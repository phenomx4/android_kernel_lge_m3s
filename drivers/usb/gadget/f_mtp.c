/*
* f_mtp.c -- USB MTP gadget driver
* LG USB gadget driver for MTP sync
* This program is the modified version of free software for LG USB gadget driver
* Editor : jaeho.cho@lge.com
* 
* Copyright (C) 2008 by LGE
*
* This software is licensed under the terms of the GNU General Public
* License version 2, as published by the Free Software Foundation, and
* may be copied, distributed, and modified under those terms.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
* GNU General Public License for more details.
*
*/
/* #define VERBOSE_DEBUG */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/poll.h>
#include <linux/delay.h>
#include <linux/wait.h>
#include <linux/err.h>
#include <linux/interrupt.h>

#include <linux/types.h>
#include <linux/device.h>
#include <linux/miscdevice.h>
#include <linux/proc_fs.h>

#include <linux/usb/ch9.h>
#ifdef CONFIG_LGE_USB_GADGET_FUNC_BIND_ONLY_INIT
#include <linux/usb/android_composite.h>
#else
#include <linux/usb/composite.h>
#endif
#include <linux/usb/gadget.h>
#include <linux/usb/f_mtp.h>

#define BULK_BUFFER_SIZE           16384
#define INTR_BUFFER_SIZE           28

/* String IDs */
#define INTERFACE_STRING_INDEX	0

/* values for mtp_dev.state */
#define STATE_OFFLINE               0   /* initial state, disconnected */
#define STATE_READY                 1   /* ready for userspace calls */
#define STATE_BUSY                  2   /* processing userspace calls */
#define STATE_CANCELED              3   /* transaction canceled by host */
#define STATE_ERROR                 4   /* error from completion routine */

/* number of tx and rx requests to allocate */
#define TX_REQ_MAX 4
#define RX_REQ_MAX 2

/* ID for Microsoft MTP OS String */
#define MTP_OS_STRING_ID   0xEE


#include "gadget_chips.h"

/*
#define DEBUG
*/

#define mtp_err(fmt, arg...)	printk(KERN_ERR "%s(): " fmt, __func__, ##arg)
#ifdef DEBUG
#ifdef CONFIG_LGE_USB_GADGET_MTP_DRIVER
#define lg_mtp_debug(fmt, arg...)	printk(KERN_ERR "%s(): " fmt, __func__, ##arg)
#define mtp_debug(fmt, arg...)
#else
#define mtp_debug(fmt, arg...)	printk(KERN_ERR "%s(): " fmt, __func__, ##arg)
#endif
#else
#define mtp_debug(fmt, arg...)
#ifdef CONFIG_LGE_USB_GADGET_MTP_DRIVER
#define lg_mtp_debug(fmt, arg...)
#endif
#endif

#define BULK_BUFFER_SIZE    16384//8192
#define MIN(a, b)	((a < b) ? a : b)

/*
 *
 */

/* static strings, in UTF-8 */
#define STRING_INTERFACE	0
#define STRING_MTP      	1
*/
struct mtp_dev {
	struct usb_function function;
	struct usb_composite_dev *cdev;
	spinlock_t lock;

	/* appear as MTP or PTP when enumerating */
	int interface_mode;

	struct usb_ep *ep_in;
	struct usb_ep *ep_out;
	struct usb_ep *ep_intr;

	int state;

	/* synchronize access to our device file */
	atomic_t open_excl;
	/* to enforce only one ioctl at a time */
	atomic_t ioctl_excl;

	struct list_head tx_idle;

	wait_queue_head_t read_wq;
	wait_queue_head_t write_wq;
	wait_queue_head_t intr_wq;
	struct usb_request *rx_req[RX_REQ_MAX];
	struct usb_request *intr_req;
	int rx_done;
	/* true if interrupt endpoint is busy */
	int intr_busy;

	/* for processing MTP_SEND_FILE and MTP_RECEIVE_FILE
	 * ioctls on a work queue
	 */
	struct workqueue_struct *wq;
	struct work_struct send_file_work;
	struct work_struct receive_file_work;
	struct file *xfer_file;
	loff_t xfer_file_offset;
	int64_t xfer_file_length;
	int xfer_result;
};


static struct usb_string mtp_string_defs[] = {
	[STRING_INTERFACE].s = "LG MTP Interface",
    [STRING_MTP].s = "MSFT100\376",
	{  /* ZEROES END LIST */ },
};

static struct usb_gadget_strings mtp_string_table = {
	.language =		0x0409,	/* en-us */
	.strings =		mtp_string_defs,
};

static struct usb_gadget_strings *mtp_strings[] = {
	&mtp_string_table,
	NULL,
};

/* There is only one interface. */
static struct usb_interface_descriptor intf_desc = {
	.bLength = sizeof intf_desc,
	.bDescriptorType = USB_DT_INTERFACE,
	.bInterfaceNumber       = 0,
	.bNumEndpoints = 3,
	.bInterfaceClass = 0x06,
	.bInterfaceSubClass = 0x01,
	.bInterfaceProtocol = 0x01,
};

static struct usb_endpoint_descriptor fs_bulk_in_desc = {
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,
	.bEndpointAddress = USB_DIR_IN,
	.bmAttributes = USB_ENDPOINT_XFER_BULK,
};

static struct usb_endpoint_descriptor fs_bulk_out_desc = {
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,
	.bEndpointAddress = USB_DIR_OUT,
	.bmAttributes = USB_ENDPOINT_XFER_BULK,
};

static struct usb_endpoint_descriptor fs_intr_in_desc = {
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,
	.bEndpointAddress = USB_DIR_IN,
	.bmAttributes = USB_ENDPOINT_XFER_INT,
	.wMaxPacketSize = __constant_cpu_to_le16(64),
	.bInterval = 10,
};

static struct usb_descriptor_header *fs_mtp_descs[] = {
	(struct usb_descriptor_header *) &intf_desc,
	(struct usb_descriptor_header *) &fs_bulk_out_desc,
	(struct usb_descriptor_header *) &fs_bulk_in_desc,
	(struct usb_descriptor_header *) &fs_intr_in_desc,
	NULL,
};

static struct usb_endpoint_descriptor hs_bulk_in_desc = {
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,
	.bmAttributes = USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize = __constant_cpu_to_le16(512),
	.bInterval = 0,
};

static struct usb_endpoint_descriptor hs_bulk_out_desc = {
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,
	.bmAttributes = USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize = __constant_cpu_to_le16(512),
	.bInterval = 0,
};

static struct usb_endpoint_descriptor hs_intr_in_desc = {
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,
	.bmAttributes = USB_ENDPOINT_XFER_INT,
	.wMaxPacketSize = __constant_cpu_to_le16(64),
	.bInterval = 10,
};

static struct usb_descriptor_header *hs_mtp_descs[] = {
	(struct usb_descriptor_header *) &intf_desc,
	(struct usb_descriptor_header *) &hs_bulk_out_desc,
	(struct usb_descriptor_header *) &hs_bulk_in_desc,
	(struct usb_descriptor_header *) &hs_intr_in_desc,
	NULL,
};

/* used when MTP function is disabled */
static struct usb_descriptor_header *null_mtp_descs[] = {
	NULL,
};

#define MAX_BULK_RX_REQ_NUM 8
#define MAX_BULK_TX_REQ_NUM 8
#define MAX_CTL_RX_REQ_NUM	8

/*---------------------------------------------------------------------------*/
struct usb_mtp_context {
	struct usb_function function;
	struct usb_composite_dev *cdev;

	spinlock_t lock;  /* For RX/TX/INT list */

	struct usb_ep *bulk_in;
	struct usb_ep *bulk_out;
	struct usb_ep *intr_in;

	struct list_head rx_reqs;
	struct list_head rx_done_reqs;
	struct list_head tx_reqs;
	struct list_head ctl_rx_reqs;
	struct list_head ctl_rx_done_reqs;

	int online;
	int error;
	int cancel;
	int ctl_cancel;
	int intr_in_busy;

	wait_queue_head_t rx_wq;
	wait_queue_head_t tx_wq;
	wait_queue_head_t ctl_rx_wq;
	wait_queue_head_t ctl_tx_wq;

	struct usb_request *int_tx_req;
	struct usb_request *ctl_tx_req;

	/* the request we're currently reading from */
	struct usb_request *cur_read_req;
	/* buffer to point to available data in the current request */
	unsigned char *read_buf;
	/* available data length */
	int data_len;
};

static struct usb_mtp_context g_usb_mtp_context;

/* record all usb requests for bulk out */
static struct usb_request *pending_reqs[MAX_BULK_RX_REQ_NUM];
#define MTP_CANCEL_REQ_DATA_SIZE		6

struct ctl_req_wrapper {
	int header;
	struct usb_ctrlrequest creq;
	struct list_head	list;
	char cancel_data[MTP_CANCEL_REQ_DATA_SIZE];
};

struct ctl_req_wrapper ctl_reqs[MAX_CTL_RX_REQ_NUM];
struct ctl_req_wrapper *cur_creq;
int ctl_tx_done;

#ifdef CONFIG_LGE_USB_GADGET_MTP_DRIVER
#define MTP_ONLINE 0x00
#define MTP_OFFLINE 0x01
#define MTP_UNKOWN 0x02
#define MTP_NO_INIT_STATUS 0x03
#define MTP_CLASS_CANCEL_REQ			0x64
#define MTP_CLASS_GET_EXTEND_EVEVT_DATA	0x65
#define MTP_CLASS_RESET_REQ				0x66
#define MTP_CLASS_GET_DEVICE_STATUS		0x67

__u8 g_bRequest= MTP_NO_INIT_STATUS;

#define USB_MTP_IOC_MAGIC 0xFF

#define USB_MTP_FUNC_IOC_CANCEL_REQUEST_SET _IOW(USB_MTP_IOC_MAGIC, 0x20, int)
#define USB_MTP_FUNC_IOC_CANCEL_REQUEST_GET _IOW(USB_MTP_IOC_MAGIC, 0x21, int)
#define USB_MTP_FUNC_IOC_GET_EXTENDED_EVENT_DATA_SET    _IOW(USB_MTP_IOC_MAGIC, 0x22, int)
#define USB_MTP_FUNC_IOC_GET_EXTENDED_EVENT_DATA_GET    _IOW(USB_MTP_IOC_MAGIC, 0x23, int)
#define USB_MTP_FUNC_IOC_DEVICE_RESET_REQUEST_SET   _IOW(USB_MTP_IOC_MAGIC, 0x24, int)
#define USB_MTP_FUNC_IOC_DEVICE_RESET_REQUEST_GET   _IOW(USB_MTP_IOC_MAGIC, 0x25, int)
#define USB_MTP_FUNC_IOC_GET_DEVICE_STATUS_SET  _IOW(USB_MTP_IOC_MAGIC, 0x26, int)
#define USB_MTP_FUNC_IOC_GET_DEVICE_STATUS_GET  _IOW(USB_MTP_IOC_MAGIC, 0x27, int)
#define USB_MTP_FUNC_IOC_GET_ONLINE_STATUS_GET  _IOW(USB_MTP_IOC_MAGIC, 0x28, int)
#define USB_MTP_FUNC_IOC_CONTROL_REQUEST_GET _IOW(USB_MTP_IOC_MAGIC, 0x29, int)
#endif

#ifdef CONFIG_LGE_USB_GADGET_MTP_DRIVER
static int cancel_noti = 0;
extern const u16 lg_mtp_pid;
#endif
/*-------------------------------------------------------------------------*/

static struct usb_request *req_new(struct usb_ep *ep, int size)
{
	struct usb_request *req = usb_ep_alloc_request(ep, GFP_KERNEL);
	if (!req)
		return NULL;

	/* now allocate buffers for the requests */
	req->buf = kmalloc(size, GFP_KERNEL);
	if (!req->buf) {
		usb_ep_free_request(ep, req);
		return NULL;
	}

	return req;
}

static void req_free(struct usb_request *req, struct usb_ep *ep)
{
	if (req) {
		kfree(req->buf);
		usb_ep_free_request(ep, req);
	}
}

/* add a request to the tail of a list */
static void req_put(struct list_head *head, struct usb_request *req)
{
	unsigned long flags;

	spin_lock_irqsave(&g_usb_mtp_context.lock, flags);
	list_add_tail(&req->list, head);
	spin_unlock_irqrestore(&g_usb_mtp_context.lock, flags);
}

/* remove a request from the head of a list */
static struct usb_request *req_get(struct list_head *head)
{
	unsigned long flags;
	struct usb_request *req;

	spin_lock_irqsave(&g_usb_mtp_context.lock, flags);
	if (list_empty(head)) {
		req = 0;
	} else {
		req = list_first_entry(head, struct usb_request, list);
		list_del(&req->list);
	}
	spin_unlock_irqrestore(&g_usb_mtp_context.lock, flags);
	return req;
}

/* add a mtp control request to the tail of a list */
static void ctl_req_put(struct list_head *head, struct ctl_req_wrapper *req)
{
	unsigned long flags;

	spin_lock_irqsave(&g_usb_mtp_context.lock, flags);
	list_add_tail(&req->list, head);
	spin_unlock_irqrestore(&g_usb_mtp_context.lock, flags);
}

/* remove a request from the head of a list */
static struct ctl_req_wrapper *ctl_req_get(struct list_head *head)
{
	unsigned long flags;
	struct ctl_req_wrapper *req;

	spin_lock_irqsave(&g_usb_mtp_context.lock, flags);
	if (list_empty(head)) {
		req = 0;
	} else {
		req = list_first_entry(head, struct ctl_req_wrapper, list);
		list_del(&req->list);
	}
	spin_unlock_irqrestore(&g_usb_mtp_context.lock, flags);
	return req;
}
/*-------------------------------------------------------------------------*/

static void mtp_in_complete(struct usb_ep *ep, struct usb_request *req)
{
	mtp_debug("status is %d %p %d\n", req->status, req, req->actual);
	if (req->status == -ECONNRESET)
		usb_ep_fifo_flush(ep);

	if (req->status != 0) {
		g_usb_mtp_context.error = 1;
		mtp_err("status is %d %p len=%d\n",
		req->status, req, req->actual);
	}

	req_put(&g_usb_mtp_context.tx_reqs, req);
	wake_up(&g_usb_mtp_context.tx_wq);
}

static void mtp_out_complete(struct usb_ep *ep, struct usb_request *req)
{
	mtp_debug("status is %d %p %d\n", req->status, req, req->actual);
	if (req->status == 0) {
		req_put(&g_usb_mtp_context.rx_done_reqs, req);
	} else {
		mtp_err("status is %d %p len=%d\n",
		req->status, req, req->actual);
		g_usb_mtp_context.error = 1;
		if (req->status == -ECONNRESET)
		{
#ifdef CONFIG_LGE_USB_GADGET_MTP_DRIVER
		  lg_mtp_debug("LG_FW : BULK OUT DATA Flush!!\n");
#endif
		  usb_ep_fifo_flush(ep);
		}
		req_put(&g_usb_mtp_context.rx_reqs, req);
	}
	wake_up(&g_usb_mtp_context.rx_wq);
}

static void mtp_int_complete(struct usb_ep *ep, struct usb_request *req)
{
	mtp_debug("status is %d %d\n", req->status, req->actual);

	if (req->status == -ECONNRESET)
		usb_ep_fifo_flush(ep);

	if (req->status != 0)
		mtp_err("status is %d %p len=%d\n",
		req->status, req, req->actual);

	g_usb_mtp_context.intr_in_busy = 0;
	return;
}

static ssize_t mtp_read(struct file *fp, char __user *buf,
				size_t count, loff_t *pos)
{
	struct usb_request *req = 0;
#if 0 // 414003 patch
	int xfer, rc = count;
	int ret;

	while (count > 0) {
		mtp_debug("count=%d\n", count);
		if (g_usb_mtp_context.error) {
			return -EIO;
		}
		/* we will block until we're online */
		ret = wait_event_interruptible(g_usb_mtp_context.rx_wq,
#ifdef CONFIG_LGE_USB_GADGET_MTP_DRIVER
			((g_usb_mtp_context.online || g_usb_mtp_context.cancel) && !cancel_noti));
#else
			(g_usb_mtp_context.online || g_usb_mtp_context.cancel));
#endif
		if (g_usb_mtp_context.cancel) {
			mtp_debug("cancel return in mtp_read at beginning\n");
			g_usb_mtp_context.cancel = 0;
			return -EINVAL;
#endif
	int r = count, xfer;
	int sendZLP = 0;
	int ret;

	DBG(cdev, "mtp_write(%d)\n", count);

	spin_lock_irq(&dev->lock);
	if (dev->state == STATE_CANCELED) {
		/* report cancelation to userspace */
		dev->state = STATE_READY;
		spin_unlock_irq(&dev->lock);
		return -ECANCELED;
	}
	if (dev->state == STATE_OFFLINE) {
		spin_unlock_irq(&dev->lock);
		return -ENODEV;
	}
	dev->state = STATE_BUSY;
	spin_unlock_irq(&dev->lock);

	/* we need to send a zero length packet to signal the end of transfer
	 * if the transfer size is aligned to a packet boundary.
	 */
	if ((count & (dev->ep_in->maxpacket - 1)) == 0) {
		sendZLP = 1;
	}

	while (count > 0 || sendZLP) {
		/* so we exit after sending ZLP */
		if (count == 0)
			sendZLP = 0;

		if (dev->state != STATE_BUSY) {
			DBG(cdev, "mtp_write dev->error\n");
			r = -EIO;
			break;

		}

		if (ret < 0) {
			mtp_err("wait_event_interruptible return %d\n", ret);
			rc = ret;
			break;
		}

#if 0 // 414003 patch
		/* if we have idle read requests, get them queued */
		while (1) {
			  req = req_get(&g_usb_mtp_context.rx_reqs);
			  if (!req)
				  break;
requeue_req:
			  req->length = BULK_BUFFER_SIZE;
			  mtp_debug("rx %p queue\n", req);

			  ret = usb_ep_queue(g_usb_mtp_context.bulk_out,
				  req, GFP_ATOMIC);

			  if (ret < 0) {
				  mtp_err("queue error %d\n", ret);
				  g_usb_mtp_context.error = 1;
				  req_put(&g_usb_mtp_context.rx_reqs, req);
				  return ret;
			  }
#endif
#ifdef CONFIG_LGE_USB_GADGET_MTP_DRIVER
			  lg_mtp_debug("LG_FW : MTP Enqeue Data!!\n");
#endif
		if (count > BULK_BUFFER_SIZE)
			xfer = BULK_BUFFER_SIZE;
		else
			xfer = count;
		if (xfer && copy_from_user(req->buf, buf, xfer)) {
			r = -EFAULT;
			break;

		}

		/* if we have data pending, give it to userspace */
		if (g_usb_mtp_context.data_len > 0) {
			if (g_usb_mtp_context.data_len < count)
				xfer = g_usb_mtp_context.data_len;
			else
				xfer = count;
			
#ifdef CONFIG_LGE_USB_GADGET_MTP_DRIVER
        if(cancel_noti == 0)
        {
#endif
		  if (copy_to_user(buf, g_usb_mtp_context.read_buf,
			  				xfer)) {
			rc = -EFAULT;
			break;
		  }
#ifdef CONFIG_LGE_USB_GADGET_MTP_DRIVER
		  lg_mtp_debug("LG_FW : MTP data Copy to User Do!!\n");
        }
#endif

			g_usb_mtp_context.read_buf += xfer;
			g_usb_mtp_context.data_len -= xfer;
			buf += xfer;
			count -= xfer;
			mtp_debug("xfer=%d\n", xfer);

		/* if we've emptied the buffer, release the request */
		if (g_usb_mtp_context.data_len == 0) {
#ifdef CONFIG_LGE_USB_GADGET_MTP_DRIVER
			lg_mtp_debug("LG_FW : mtp data_len  = 0\n");
#endif
			req_put(&g_usb_mtp_context.rx_reqs,
					g_usb_mtp_context.cur_read_req);
			g_usb_mtp_context.cur_read_req = 0;
#if defined(CONFIG_LGE_USB_GADGET_MTP_DRIVER)
			count = 0;
			rc = xfer;
#endif
			}
			continue;
		}

		/* wait for a request to complete */
		req = 0;
		mtp_debug("wait req finish\n");
		ret = wait_event_interruptible(g_usb_mtp_context.rx_wq,
		((req = req_get(&g_usb_mtp_context.rx_done_reqs))
			|| g_usb_mtp_context.cancel));
		mtp_debug("req finished\n");
		if (g_usb_mtp_context.cancel) {
			if (req != 0)
				req_put(&g_usb_mtp_context.rx_reqs, req);
			mtp_debug("cancel return in mtp_read at complete\n");
			g_usb_mtp_context.cancel = 0;
			return -EINVAL;
		}
		if (ret < 0) {
			mtp_err("wait_event_interruptible(2) return %d\n", ret);
			rc = ret;
			break;
		}
		if (req != 0) {
			/* if we got a 0-len one we need to put it back into
			** service.  if we made it the current read req we'd
			** be stuck forever
			*/
			if (req->actual == 0)
				goto requeue_req;

			g_usb_mtp_context.cur_read_req = req;
			g_usb_mtp_context.data_len = req->actual;
			g_usb_mtp_context.read_buf = req->buf;
#if defined(CONFIG_LGE_USB_GADGET_MTP_DRIVER)
			lg_mtp_debug("LG_FW : g_usb_mtp_context.data_len = [%d] \n", g_usb_mtp_context.data_len);
#endif
			mtp_debug("rx %p done actual=%d\n", req, req->actual);
		}
	}

	mtp_debug("mtp_read returning %d\n", rc);
	return rc;
}

static ssize_t mtp_write(struct file *fp, const char __user *buf,
				 size_t count, loff_t *pos)
{
	struct usb_request *req;
	int rc = count, xfer;
	int ret;

	while (count > 0) {
		mtp_debug("count=%d\n", count);
		if (g_usb_mtp_context.error) {
			return -EIO;
		}
		/* get an idle tx request to use */
		ret = wait_event_interruptible(g_usb_mtp_context.tx_wq,
			(g_usb_mtp_context.online || g_usb_mtp_context.cancel));

		if (g_usb_mtp_context.cancel) {
			mtp_debug("cancel return in mtp_write at beginning\n");
			g_usb_mtp_context.cancel = 0;
			return -EINVAL;
		}
		if (ret < 0) {
			mtp_err("wait_event_interruptible return %d\n", ret);
			rc = ret;
			break;
		}

		req = 0;
		mtp_debug("get tx req\n");
		ret = wait_event_interruptible(g_usb_mtp_context.tx_wq,
			((req = req_get(&g_usb_mtp_context.tx_reqs))
			 || g_usb_mtp_context.cancel));

		mtp_debug("got tx req\n");
		if (g_usb_mtp_context.cancel) {
			mtp_debug("cancel return in mtp_write get req\n");
			if (req != 0)
				req_put(&g_usb_mtp_context.tx_reqs, req);
			g_usb_mtp_context.cancel = 0;
			return -EINVAL;
		}
		if (ret < 0) {
			mtp_err("wait_event_interruptible return(2) %d\n", ret);
			rc = ret;
			break;
		}

		if (req != 0) {
			if (count > BULK_BUFFER_SIZE)
				xfer = BULK_BUFFER_SIZE;
			else
				xfer = count;
			if (copy_from_user(req->buf, buf, xfer)) {
				req_put(&g_usb_mtp_context.tx_reqs, req);
				rc = -EFAULT;
				break;
			}

			req->length = xfer;
			ret = usb_ep_queue(g_usb_mtp_context.bulk_in,
				req, GFP_ATOMIC);
			if (ret < 0) {
				mtp_err("error %d\n", ret);
				g_usb_mtp_context.error = 1;
				req_put(&g_usb_mtp_context.tx_reqs, req);
				rc = ret;
				break;
			}

			buf += xfer;
			count -= xfer;
			mtp_debug("xfer=%d\n", xfer);
		}
	}

	mtp_debug("mtp_write returning %d\n", rc);
	return rc;
}
/* read from a local file and write to USB */
static void send_file_work(struct work_struct *data) {
	struct mtp_dev	*dev = container_of(data, struct mtp_dev, send_file_work);
	struct usb_composite_dev *cdev = dev->cdev;
	struct usb_request *req = 0;
	struct file *filp;
	loff_t offset;
	int64_t count;
	int xfer, ret;
	int r = 0;
	int sendZLP = 0;

	/* read our parameters */
	smp_rmb();
	filp = dev->xfer_file;
	offset = dev->xfer_file_offset;
	count = dev->xfer_file_length;

	DBG(cdev, "send_file_work(%lld %lld)\n", offset, count);

	/* we need to send a zero length packet to signal the end of transfer
	 * if the transfer size is aligned to a packet boundary.
	 */
	if ((dev->xfer_file_length & (dev->ep_in->maxpacket - 1)) == 0) {
		sendZLP = 1;
	}

	while (count > 0 || sendZLP) {
		/* so we exit after sending ZLP */
		if (count == 0)
			sendZLP = 0;

		/* get an idle tx request to use */
		req = 0;
		ret = wait_event_interruptible(dev->write_wq,
			(req = req_get(dev, &dev->tx_idle))
			|| dev->state != STATE_BUSY);
		if (dev->state == STATE_CANCELED) {
			r = -ECANCELED;
			break;
		}
		if (!req) {
			r = ret;
			break;

		}
		/* get an idle tx request to use */
		ret = wait_event_interruptible(g_usb_mtp_context.tx_wq,
			(g_usb_mtp_context.online || g_usb_mtp_context.cancel));

		if (g_usb_mtp_context.cancel) {
			mtp_debug("cancel return in mtp_write at beginning\n");
			g_usb_mtp_context.cancel = 0;
			return -EINVAL;
		}
		if (ret < 0) {
			mtp_err("wait_event_interruptible return %d\n", ret);
			rc = ret;
			break;
		}

		req = 0;
		mtp_debug("get tx req\n");
		ret = wait_event_interruptible(g_usb_mtp_context.tx_wq,
			((req = req_get(&g_usb_mtp_context.tx_reqs))
			 || g_usb_mtp_context.cancel));

		mtp_debug("got tx req\n");
		if (g_usb_mtp_context.cancel) {
			mtp_debug("cancel return in mtp_write get req\n");
			if (req != 0)
				req_put(&g_usb_mtp_context.tx_reqs, req);
			g_usb_mtp_context.cancel = 0;
			return -EINVAL;
		}
		if (ret < 0) {
			DBG(cdev, "send_file_work: xfer error %d\n", ret);
			dev->state = STATE_ERROR;
			r = -EIO;
			break;
		}

		count -= xfer;

		/* zero this so we don't try to free it on error exit */
		req = 0;
	}

	if (req)
		req_put(dev, &dev->tx_idle, req);

	DBG(cdev, "send_file_work returning %d\n", r);
	/* write the result */
	dev->xfer_result = r;
	smp_wmb();
}

/* read from USB and write to a local file */
static void receive_file_work(struct work_struct *data)
{
	struct mtp_dev	*dev = container_of(data, struct mtp_dev, receive_file_work);
	struct usb_composite_dev *cdev = dev->cdev;
	struct usb_request *read_req = NULL, *write_req = NULL;
	struct file *filp;
	loff_t offset;
	int64_t count;
	int ret, cur_buf = 0;
	int r = 0;

	/* read our parameters */
	smp_rmb();
	filp = dev->xfer_file;
	offset = dev->xfer_file_offset;
	count = dev->xfer_file_length;

	DBG(cdev, "receive_file_work(%lld)\n", count);

	while (count > 0 || write_req) {
		if (count > 0) {
			/* queue a request */
			read_req = dev->rx_req[cur_buf];
			cur_buf = (cur_buf + 1) % RX_REQ_MAX;

			read_req->length = (count > BULK_BUFFER_SIZE
					? BULK_BUFFER_SIZE : count);
			dev->rx_done = 0;
			ret = usb_ep_queue(dev->ep_out, read_req, GFP_KERNEL);
			if (ret < 0) {
				r = -EIO;
				dev->state = STATE_ERROR;
				break;
			}
		}


			req->length = xfer;
			ret = usb_ep_queue(g_usb_mtp_context.bulk_in,
				req, GFP_ATOMIC);
			if (ret < 0) {
				mtp_err("error %d\n", ret);
				g_usb_mtp_context.error = 1;
				req_put(&g_usb_mtp_context.tx_reqs, req);
				rc = ret;
				break;
			}

		if (read_req) {
			/* wait for our last read to complete */
			ret = wait_event_interruptible(dev->read_wq,
				dev->rx_done || dev->state != STATE_BUSY);
			if (dev->state == STATE_CANCELED) {
				r = -ECANCELED;
				if (!dev->rx_done)
					usb_ep_dequeue(dev->ep_out, read_req);
				break;
			}
			/* if xfer_file_length is 0xFFFFFFFF, then we read until
			 * we get a zero length packet
			 */
			if (count != 0xFFFFFFFF)
				count -= read_req->actual;
			if (read_req->actual < read_req->length) {
				/* short packet is used to signal EOF for sizes > 4 gig */
				DBG(cdev, "got short packet\n");
				count = 0;
			}

			write_req = read_req;
			read_req = NULL;
		}
	}

	DBG(cdev, "receive_file_work returning %d\n", r);
	/* write the result */
	dev->xfer_result = r;
	smp_wmb();
}

/* ioctl related */
#define MTP_EVENT_SIZE   28
struct mtp_event_data {
    unsigned char data[MTP_EVENT_SIZE];
};
#ifdef CONFIG_LGE_USB_GADGET_MTP_DRIVER
extern int mtp_get_usb_state(void);

extern int mtp_enable_flag;
extern u16 product_id;
static int mtp_ioctl(struct inode *inode, struct file *file,
		unsigned int cmd, unsigned long arg)
{   
  u16 g_mtp_get_status;    
  struct usb_request	*req =g_usb_mtp_context.cdev->req;
  struct usb_ep *ep0;
  long ret;
  int usbconnect = 0;

/* LGE_CHANGES_S [younsuk.song@lge.com] 2010-09-18, Depense code for Null pointer access */

  /* When mtp_enable_open()/release()(in android.c) is invoked,
   * mtp_enable_flag is set/cleared. If enable flag is false(mtp is off),
   * we cut off the user's ioctl request.
   */
  if (!mtp_enable_flag)
	  return -ENODEV;

  if (product_id != lg_mtp_pid) {
	  printk(KERN_INFO "not MTP pid\n");
	  return -EFAULT;
  }

  if (g_usb_mtp_context.cdev->gadget == NULL) {
	  return -EFAULT;
  }

  ep0 = g_usb_mtp_context.cdev->gadget->ep0;

/* LGE_CHANGES_E [younsuk.song@lge.com]  */


  switch (cmd)  
  {      	       
	case USB_MTP_FUNC_IOC_CONTROL_REQUEST_GET:
		
      if(g_bRequest==MTP_CLASS_CANCEL_REQ||
        g_bRequest==MTP_CLASS_RESET_REQ||
        g_bRequest==MTP_CLASS_GET_DEVICE_STATUS
        )
      {
         mtp_debug("USB_MTP_FUNC_IOC_CONTROL_REQUEST_GET status = %d\n", g_bRequest);
         ret = copy_to_user ((void __user *)arg, &g_bRequest, sizeof(g_bRequest));
		 
		 if(g_bRequest == MTP_CLASS_CANCEL_REQ)
		 {
		   lg_mtp_debug("LG_FW : MTP CANCEL Request Device => App !!\n");
		   cancel_noti = 1;
		 }
		 else if(g_bRequest == MTP_CLASS_GET_DEVICE_STATUS)
		 {
		   lg_mtp_debug("LG_FW : MTP GET DEVICE Request Device => App!!\n");
		 }
      }
      else
      {
         mtp_debug("USB_MTP_FUNC_IOC_OTHER_CONTROL_REQUEST_GET status = %d\n", g_bRequest);
         usbconnect = mtp_get_usb_state();
         if(usbconnect == 0)
         {
           g_bRequest = MTP_OFFLINE; //offline
           ret = copy_to_user ((void __user *)arg, &g_bRequest, sizeof(g_bRequest));
         }
         else
         {
           if(g_usb_mtp_context.online == 1)
           {
              g_bRequest = MTP_ONLINE;//online
              ret = copy_to_user ((void __user *)arg, &g_bRequest, sizeof(g_bRequest));
           }
           else
           {
              g_bRequest = MTP_UNKOWN; //unkown
              ret = copy_to_user ((void __user *)arg, &g_bRequest, sizeof(g_bRequest));
           }
         }
      }
	  
	   g_bRequest = MTP_NO_INIT_STATUS;
	   
      if(ret >= 0)
        return ret;
      else
        return -EFAULT;   
	  break;
		
	case USB_MTP_FUNC_IOC_GET_DEVICE_STATUS_SET:
		 mtp_debug("USB_MTP_FUNC_IOC_GET_DEVICE_STATUS_SET status = %d\n", g_bRequest);
         ret = copy_from_user (&g_mtp_get_status, (void __user *)arg, sizeof(g_mtp_get_status));
         if(ret < 0)
           return -EFAULT;

         if(req == NULL)
         {
           mtp_debug("LG_FW :: req is NULL");
           return -EFAULT;
         }
		 lg_mtp_debug("LG_FW : MTP SET DEVICE STATUS App => Device [0x%x]!!\n",arg);
         *((u16 *)(req->buf)) = 0x0004;
         *((u16 *)(req->buf + 2)) = arg;
		 req->zero = 0;
		 req->length = 6;
         usb_ep_queue(ep0,req, GFP_ATOMIC);
		 
		 if(arg == 0x2001)
		 {
		   cancel_noti = 0;
		 }
		 break;
	                          
    default :
      mtp_debug("Invalid IOCTL  Processed!!\n");
      break; 
 	}  
  
	return 0;  
}
#endif

/* file operations for MTP device /dev/mtp */
static const struct file_operations mtp_fops = {
	.owner = THIS_MODULE,
	.read = mtp_read,
	.write = mtp_write,
};

static struct miscdevice mtp_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "mtp",
	.fops = &mtp_fops,
};

/* file operations for MTP device /dev/mtp_csr */
static const struct file_operations mtp_csr_fops = {
	.owner = THIS_MODULE,
	.ioctl = mtp_ioctl,
};
static struct miscdevice mtp_csr_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "mtp_csr",
	.fops = &mtp_csr_fops,
};

/* mtpctl related */
#define MTP_CTL_CLASS_REQ    1
#define MTP_CTL_CLASS_REPLY  2

struct mtp_ctl_msg_header {
    int msg_len;
    int msg_id;
};

#define MTP_CTL_MSG_HEADER_SIZE   (sizeof(struct mtp_ctl_msg_header))
#define MTP_CTL_MSG_SIZE	(MTP_CTL_MSG_HEADER_SIZE +\
			 sizeof(struct usb_ctrlrequest))

#ifdef CONFIG_LGE_USB_GADGET_MTP_DRIVER
#else
static void mtp_ctl_read_complete(struct usb_ep *ep, struct usb_request *req)
{
	struct ctl_req_wrapper	*ctl_req = ep->driver_data;

	mtp_debug("mtp_ctl_read_complete --> %d, %d/%d\n",
	req->status, req->actual, req->length);

/* 414003 patch
	if (req->status == 0)
		memcpy(ctl_req->cancel_data, req->buf, req->actual);

	wake_up(&g_usb_mtp_context.ctl_rx_wq);
*/
	/* wait for a request to complete */
	ret = wait_event_interruptible(dev->intr_wq, !dev->intr_busy || dev->state == STATE_OFFLINE);
	if (ret < 0)
		return ret;
	if (dev->state == STATE_OFFLINE)
		return -ENODEV;
	req = dev->intr_req;
	if (copy_from_user(req->buf, (void __user *)event->data, length))
		return -EFAULT;
	req->length = length;
	dev->intr_busy = 1;
	ret = usb_ep_queue(dev->ep_intr, req, GFP_KERNEL);
	if (ret)
		dev->intr_busy = 0;

	return ret;

}
#endif

static void mtp_ctl_write_complete(struct usb_ep *ep, struct usb_request *req)
{
	struct mtp_dev *dev = fp->private_data;
	struct file *filp = NULL;
	int ret = -EINVAL;

	if (_lock(&dev->ioctl_excl))
		return -EBUSY;

	switch (code) {
	case MTP_SEND_FILE:
	case MTP_RECEIVE_FILE:
	{
		struct mtp_file_range	mfr;
		struct work_struct *work;

		spin_lock_irq(&dev->lock);
		if (dev->state == STATE_CANCELED) {
			/* report cancelation to userspace */
			dev->state = STATE_READY;
			spin_unlock_irq(&dev->lock);
			ret = -ECANCELED;
			goto out;
		}
		if (dev->state == STATE_OFFLINE) {
			spin_unlock_irq(&dev->lock);
			ret = -ENODEV;
			goto out;
		}
		dev->state = STATE_BUSY;
		spin_unlock_irq(&dev->lock);

		if (copy_from_user(&mfr, (void __user *)value, sizeof(mfr))) {
			ret = -EFAULT;
			goto fail;
		}
		/* hold a reference to the file while we are working with it */
		filp = fget(mfr.fd);
		if (!filp) {
			ret = -EBADF;
			goto fail;
		}

		/* write the parameters */
		dev->xfer_file = filp;
		dev->xfer_file_offset = mfr.offset;
		dev->xfer_file_length = mfr.length;
		smp_wmb();

		if (code == MTP_SEND_FILE)
			work = &dev->send_file_work;
		else
			work = &dev->receive_file_work;

		/* We do the file transfer on a work queue so it will run
		 * in kernel context, which is necessary for vfs_read and
		 * vfs_write to use our buffers in the kernel address space.
		 */
		queue_work(dev->wq, work);
		/* wait for operation to complete */
		flush_workqueue(dev->wq);
		fput(filp);

		/* read the result */
		smp_rmb();
		ret = dev->xfer_result;
		break;
	}
	case MTP_SET_INTERFACE_MODE:
		if (value == MTP_INTERFACE_MODE_MTP ||
			value == MTP_INTERFACE_MODE_PTP) {
			dev->interface_mode = value;
			if (value == MTP_INTERFACE_MODE_PTP) {
				dev->function.descriptors = fs_ptp_descs;
				dev->function.hs_descriptors = hs_ptp_descs;
			} else {
				dev->function.descriptors = fs_mtp_descs;
				dev->function.hs_descriptors = hs_mtp_descs;
			}
			ret = 0;
		}
		break;
	case MTP_SEND_EVENT:
	{
		struct mtp_event	event;
		/* return here so we don't change dev->state below,
		 * which would interfere with bulk transfer state.
		 */
		if (copy_from_user(&event, (void __user *)value, sizeof(event)))
			ret = -EFAULT;
		else
			ret = mtp_send_event(dev, &event);
		goto out;

	}

	msg.msg_id = MTP_CTL_CLASS_REQ;
	msg.msg_len = MTP_CTL_MSG_SIZE;
	if (cur_creq->creq.bRequest == MTP_CLASS_CANCEL_REQ)
		msg.msg_len = MTP_CTL_MSG_SIZE + MTP_CANCEL_REQ_DATA_SIZE;

	if (cur_creq->header == 1) {
		cur_creq->header = 0;
		if (copy_to_user(buf, &msg, MTP_CTL_MSG_HEADER_SIZE))
			goto ctl_read_fail;
		ret = MTP_CTL_MSG_HEADER_SIZE;
		mtp_debug("msg header return %d\n", ret);
	} else {
		if (copy_to_user(buf, &cur_creq->creq, size))
			goto ctl_read_fail;
		ret = size;
		if (cur_creq->creq.bRequest == MTP_CLASS_CANCEL_REQ) {
			if (copy_to_user(buf + size, &cur_creq->cancel_data,
				MTP_CANCEL_REQ_DATA_SIZE))
				goto ctl_read_fail;
			ret += MTP_CANCEL_REQ_DATA_SIZE;
		}
		mtp_debug("prepare %d %x\n", ret, cur_creq->creq.bRequest);
		ctl_req_put(&g_usb_mtp_context.ctl_rx_reqs, cur_creq);
		cur_creq = NULL;
	}

fail:
	spin_lock_irq(&dev->lock);
	if (dev->state == STATE_CANCELED)
		ret = -ECANCELED;
	else if (dev->state != STATE_OFFLINE)
		dev->state = STATE_READY;
	spin_unlock_irq(&dev->lock);
out:
	_unlock(&dev->ioctl_excl);
	DBG(dev->cdev, "ioctl returning %d\n", ret);
	return ret;

}
/* 414003 patch
	mtp_debug("mtp_ctl_write_complete --> %d, %d/%d\n",
	req->status, req->actual, req->length);

	ctl_tx_done = 1;

	wake_up(&g_usb_mtp_context.ctl_tx_wq);
}
*/
static ssize_t mtp_ctl_read(struct file *file, char *buf,
	size_t count, loff_t *pos)
{
    int ret, size = sizeof(struct usb_ctrlrequest);
	struct mtp_ctl_msg_header msg;

	mtp_debug("count=%d\n", count);

	if (!g_usb_mtp_context.online)
		return -EINVAL;

	if (!cur_creq) {
		ret = wait_event_interruptible(g_usb_mtp_context.ctl_rx_wq,
		((cur_creq = ctl_req_get(&g_usb_mtp_context.ctl_rx_done_reqs))
			|| g_usb_mtp_context.ctl_cancel));
		if (g_usb_mtp_context.ctl_cancel) {
			mtp_debug("ctl_cancel return in mtp_ctl_read\n");
			if (cur_creq)
				ctl_req_put(&g_usb_mtp_context.ctl_rx_reqs,
				cur_creq);
			g_usb_mtp_context.ctl_cancel = 0;
			return -EINVAL;
		}
		if (ret < 0) {
			mtp_err("wait_event_interruptible return %d\n", ret);
			return ret;
		}
	}

	msg.msg_id = MTP_CTL_CLASS_REQ;
	msg.msg_len = MTP_CTL_MSG_SIZE;
	if (cur_creq->creq.bRequest == MTP_CLASS_CANCEL_REQ)
		msg.msg_len = MTP_CTL_MSG_SIZE + MTP_CANCEL_REQ_DATA_SIZE;

	if (cur_creq->header == 1) {
		cur_creq->header = 0;
		if (copy_to_user(buf, &msg, MTP_CTL_MSG_HEADER_SIZE))
			goto ctl_read_fail;
		ret = MTP_CTL_MSG_HEADER_SIZE;
		mtp_debug("msg header return %d\n", ret);
	} else {
		if (copy_to_user(buf, &cur_creq->creq, size))
			goto ctl_read_fail;
		ret = size;
		if (cur_creq->creq.bRequest == MTP_CLASS_CANCEL_REQ) {
			if (copy_to_user(buf + size, &cur_creq->cancel_data,
				MTP_CANCEL_REQ_DATA_SIZE))
				goto ctl_read_fail;
			ret += MTP_CANCEL_REQ_DATA_SIZE;
		}
		mtp_debug("prepare %d %x\n", ret, cur_creq->creq.bRequest);
		ctl_req_put(&g_usb_mtp_context.ctl_rx_reqs, cur_creq);
		cur_creq = NULL;
	}

	mtp_debug("return %d\n", ret);
    return ret;

ctl_read_fail:
	ctl_req_put(&g_usb_mtp_context.ctl_rx_reqs, cur_creq);
	cur_creq = NULL;
	mtp_debug("return -EFAULT\n");
    return -EFAULT;
}

static ssize_t mtp_ctl_write(struct file *file, const char *buf,
	size_t count, loff_t *pos)
{
    struct mtp_ctl_msg_header msg;
	struct usb_request *req = NULL;
	struct usb_ep *ep0;
	int ret;

	mtp_debug("count=%d\n", count);

#if 0 // 414003 patch
	ret = wait_event_interruptible(g_usb_mtp_context.ctl_tx_wq,
		(g_usb_mtp_context.online || g_usb_mtp_context.ctl_cancel));
	if (g_usb_mtp_context.ctl_cancel) {
		mtp_debug("ctl_cancel return in mtp_ctl_write 1\n");
		g_usb_mtp_context.ctl_cancel = 0;
		return -EINVAL;
	}
	if (ret < 0)
		return ret;

	ep0 = g_usb_mtp_context.cdev->gadget->ep0;
    if (count > ep0->maxpacket || count < MTP_CTL_MSG_HEADER_SIZE) {
		mtp_err("size invalid\n");
		return -ENOMEM;
    }

    /* msg info */
    if (copy_from_user(&msg, buf, MTP_CTL_MSG_HEADER_SIZE))
		return -EINVAL;
#endif
	/* clear any error condition */
	if (_mtp_dev->state != STATE_OFFLINE)
		_mtp_dev->state = STATE_READY;


    mtp_debug("msg len = %d, msg id = %d", msg.msg_len, msg.msg_id);
    if (msg.msg_id != MTP_CTL_CLASS_REPLY) {
		mtp_err("invalid id %d", msg.msg_id);
		return -EINVAL;
    }

#if 0 // 414003 patch
    /* sending the data */
	req = g_usb_mtp_context.ctl_tx_req;
	if (!req)
		return -ENOMEM;
    req->length = count - MTP_CTL_MSG_HEADER_SIZE;
	req->complete = mtp_ctl_write_complete;
    if (copy_from_user(req->buf,
		(u8 *)buf + MTP_CTL_MSG_HEADER_SIZE, req->length)) {
		return -EINVAL;
	}
	ctl_tx_done = 0;
	if (usb_ep_queue(ep0, req, GFP_ATOMIC)) {
		req->status = 0;
		mtp_ctl_write_complete(ep0, req);
		return -EIO;
	}
	ret = wait_event_interruptible(g_usb_mtp_context.ctl_tx_wq,
		(ctl_tx_done || g_usb_mtp_context.ctl_cancel));
	ctl_tx_done = 0;
	if (g_usb_mtp_context.ctl_cancel) {
		mtp_debug("ctl_cancel return in mtp_ctl_write\n");
		g_usb_mtp_context.ctl_cancel = 0;
		return -EINVAL;
	}
	if (ret < 0)
		return ret;

	mtp_debug("return count=%d\n", count);
    return count;
#endif
}
static int mtp_release(struct inode *ip, struct file *fp)
{
	printk(KERN_INFO "mtp_release\n");

	_unlock(&_mtp_dev->open_excl);
	return 0;

}

static const struct file_operations mtp_ctl_fops = {
     .read = mtp_ctl_read,
     .write = mtp_ctl_write,
};

static void
mtp_function_unbind(struct usb_configuration *c, struct usb_function *f)
{
	struct usb_request *req;
	int n;

	for (n = 0; n < MAX_BULK_RX_REQ_NUM; n++)
		pending_reqs[n] = NULL;

	while ((req = req_get(&g_usb_mtp_context.rx_reqs)))
		req_free(req, g_usb_mtp_context.bulk_out);
	while ((req = req_get(&g_usb_mtp_context.rx_done_reqs)))
		req_free(req, g_usb_mtp_context.bulk_out);
	while ((req = req_get(&g_usb_mtp_context.tx_reqs)))
		req_free(req, g_usb_mtp_context.bulk_in);

	req_free(g_usb_mtp_context.int_tx_req, g_usb_mtp_context.intr_in);
	req_free(g_usb_mtp_context.ctl_tx_req,
	g_usb_mtp_context.cdev->gadget->ep0);
	g_usb_mtp_context.intr_in_busy = 0;
#if !defined(CONFIG_LGE_USB_GADGET_MTP_DRIVER)
	misc_deregister(&mtp_device);
    remove_proc_entry("mtpctl", NULL);
#endif
}

static int
mtp_function_bind(struct usb_configuration *c, struct usb_function *f)
{
	int n, rc, id;
	struct usb_ep *ep;
	struct usb_request *req;
#if !defined(CONFIG_LGE_USB_GADGET_MTP_DRIVER)
    struct proc_dir_entry *mtp_proc = NULL;
#endif
//jaeho.cho 2010.11.30
	printk(KERN_ERR "LG_FW :: %s()\n", __func__);

	g_usb_mtp_context.cdev = c->cdev;
	/* allocate interface ID(s) */
	id = usb_interface_id(c, f);
	if (id < 0)
		return id;
	intf_desc.bInterfaceNumber = id;

	/* Find all the endpoints we will use */
	ep = usb_ep_autoconfig(g_usb_mtp_context.cdev->gadget,
						&fs_bulk_in_desc);
	if (!ep) {
		mtp_err("auto-configure hs_bulk_in_desc error\n");
		goto autoconf_fail;
	}
	ep->driver_data = &g_usb_mtp_context;
	g_usb_mtp_context.bulk_in = ep;

	ep = usb_ep_autoconfig(g_usb_mtp_context.cdev->gadget,
						&fs_bulk_out_desc);
	if (!ep) {
		mtp_err("auto-configure hs_bulk_out_desc error\n");
		goto autoconf_fail;
	}
	ep->driver_data = &g_usb_mtp_context;
	g_usb_mtp_context.bulk_out = ep;

	ep = usb_ep_autoconfig(g_usb_mtp_context.cdev->gadget,
						&fs_intr_in_desc);
	if (!ep) {
		mtp_err("auto-configure hs_intr_in_desc error\n");
		goto autoconf_fail;
	}
	ep->driver_data = &g_usb_mtp_context;
	g_usb_mtp_context.intr_in = ep;

	if (gadget_is_dualspeed(g_usb_mtp_context.cdev->gadget)) {
		/* Assume endpoint addresses are the same for both speeds */
		hs_bulk_in_desc.bEndpointAddress =
		    fs_bulk_in_desc.bEndpointAddress;
		hs_bulk_out_desc.bEndpointAddress =
		    fs_bulk_out_desc.bEndpointAddress;
		hs_intr_in_desc.bEndpointAddress =
		    fs_intr_in_desc.bEndpointAddress;
	}

	rc = -ENOMEM;

	for (n = 0; n < MAX_BULK_RX_REQ_NUM; n++) {
		req = req_new(g_usb_mtp_context.bulk_out, BULK_BUFFER_SIZE);
		if (!req)
			goto autoconf_fail;

		pending_reqs[n] = req;

		req->complete = mtp_out_complete;
		req_put(&g_usb_mtp_context.rx_reqs, req);
	}
	for (n = 0; n < MAX_BULK_TX_REQ_NUM; n++) {
		req = req_new(g_usb_mtp_context.bulk_in, BULK_BUFFER_SIZE);
		if (!req)
			goto autoconf_fail;

		req->complete = mtp_in_complete;
		req_put(&g_usb_mtp_context.tx_reqs, req);
	}

	for (n = 0; n < MAX_CTL_RX_REQ_NUM; n++)
		ctl_req_put(&g_usb_mtp_context.ctl_rx_reqs, &ctl_reqs[n]);

	g_usb_mtp_context.int_tx_req =
		req_new(g_usb_mtp_context.intr_in, BULK_BUFFER_SIZE);
	if (!g_usb_mtp_context.int_tx_req)
		goto autoconf_fail;
	g_usb_mtp_context.intr_in_busy = 0;
	g_usb_mtp_context.int_tx_req->complete = mtp_int_complete;

	g_usb_mtp_context.ctl_tx_req =
		req_new(g_usb_mtp_context.cdev->gadget->ep0, 512);
	if (!g_usb_mtp_context.ctl_tx_req)
		goto autoconf_fail;
#if !defined(CONFIG_LGE_USB_GADGET_MTP_DRIVER)
	misc_register(&mtp_device);

	mtp_proc = create_proc_entry("mtpctl", 0666, 0);
	if (!mtp_proc) {
		mtp_err("creating /proc/mtpctl failed\n");
		goto autoconf_fail;
    }
    mtp_proc->proc_fops = &mtp_ctl_fops;
#endif
	return 0;

autoconf_fail:
	rc = -ENOTSUPP;
	mtp_function_unbind(c, f);
	return rc;
}

static void mtp_function_disable(struct usb_function *f)
{
	printk(KERN_DEBUG "%s(): disabled\n", __func__);
	g_usb_mtp_context.online = 0;
	g_usb_mtp_context.cancel = 1;
	g_usb_mtp_context.ctl_cancel = 1;
	g_usb_mtp_context.error = 1;

	usb_ep_fifo_flush(g_usb_mtp_context.bulk_in);
	usb_ep_fifo_flush(g_usb_mtp_context.bulk_out);
	usb_ep_fifo_flush(g_usb_mtp_context.intr_in);
	usb_ep_disable(g_usb_mtp_context.bulk_in);
	usb_ep_disable(g_usb_mtp_context.bulk_out);
	usb_ep_disable(g_usb_mtp_context.intr_in);

	g_usb_mtp_context.cur_read_req = 0;
	g_usb_mtp_context.read_buf = 0;
	g_usb_mtp_context.data_len = 0;
	/* readers may be blocked waiting for us to go online */
	wake_up(&g_usb_mtp_context.rx_wq);
	wake_up(&g_usb_mtp_context.tx_wq);
	wake_up(&g_usb_mtp_context.ctl_rx_wq);
	wake_up(&g_usb_mtp_context.ctl_tx_wq);
}

static void start_out_receive(void)
{
/* 414003 patch
	struct usb_request *req;
	int ret;
*/
	struct mtp_dev	*dev = func_to_dev(f);
	struct usb_composite_dev *cdev = dev->cdev;
	int	value = -EOPNOTSUPP;
	u16	w_index = le16_to_cpu(ctrl->wIndex);
	u16	w_value = le16_to_cpu(ctrl->wValue);
	u16	w_length = le16_to_cpu(ctrl->wLength);
	unsigned long	flags;

	/* do nothing if we are disabled */
	if (dev->function.disabled)
		return value;

	VDBG(cdev, "mtp_function_setup "
			"%02x.%02x v%04x i%04x l%u\n",
			ctrl->bRequestType, ctrl->bRequest,
			w_value, w_index, w_length);

	/* Handle MTP OS string */
	if (dev->interface_mode == MTP_INTERFACE_MODE_MTP
			&& ctrl->bRequestType ==
			(USB_DIR_IN | USB_TYPE_STANDARD | USB_RECIP_DEVICE)
			&& ctrl->bRequest == USB_REQ_GET_DESCRIPTOR
			&& (w_value >> 8) == USB_DT_STRING
			&& (w_value & 0xFF) == MTP_OS_STRING_ID) {
		value = (w_length < sizeof(mtp_os_string)
				? w_length : sizeof(mtp_os_string));
		memcpy(cdev->req->buf, mtp_os_string, value);
		/* return here since composite.c will send for us */
		return value;
	}
	if ((ctrl->bRequestType & USB_TYPE_MASK) == USB_TYPE_VENDOR) {
		/* Handle MTP OS descriptor */
		DBG(cdev, "vendor request: %d index: %d value: %d length: %d\n",
			ctrl->bRequest, w_index, w_value, w_length);

		if (dev->interface_mode == MTP_INTERFACE_MODE_MTP
				&& ctrl->bRequest == 1
				&& (ctrl->bRequestType & USB_DIR_IN)
				&& (w_index == 4 || w_index == 5)) {
			value = (w_length < sizeof(mtp_ext_config_desc) ?
					w_length : sizeof(mtp_ext_config_desc));
			memcpy(cdev->req->buf, &mtp_ext_config_desc, value);
		}
	}
	if ((ctrl->bRequestType & USB_TYPE_MASK) == USB_TYPE_CLASS) {
		DBG(cdev, "class request: %d index: %d value: %d length: %d\n",
			ctrl->bRequest, w_index, w_value, w_length);

		if (ctrl->bRequest == MTP_REQ_CANCEL && w_index == 0
				&& w_value == 0) {
			DBG(cdev, "MTP_REQ_CANCEL\n");

			spin_lock_irqsave(&dev->lock, flags);
			if (dev->state == STATE_BUSY) {
				dev->state = STATE_CANCELED;
				wake_up(&dev->read_wq);
				wake_up(&dev->write_wq);
			}
			spin_unlock_irqrestore(&dev->lock, flags);

			/* We need to queue a request to read the remaining
			 *  bytes, but we don't actually need to look at
			 * the contents.
			 */
			value = w_length;
		} else if (ctrl->bRequest == MTP_REQ_GET_DEVICE_STATUS
				&& w_index == 0 && w_value == 0) {
			struct mtp_device_status *status = cdev->req->buf;
			status->wLength =
				__constant_cpu_to_le16(sizeof(*status));

			DBG(cdev, "MTP_REQ_GET_DEVICE_STATUS\n");
			spin_lock_irqsave(&dev->lock, flags);
			/* device status is "busy" until we report
			 * the cancelation to userspace
			 */
			if (dev->state == STATE_CANCELED)
				status->wCode =
					__cpu_to_le16(MTP_RESPONSE_DEVICE_BUSY);
			else
				status->wCode =
					__cpu_to_le16(MTP_RESPONSE_OK);
			spin_unlock_irqrestore(&dev->lock, flags);
			value = sizeof(*status);
		}
	}


	/* if we have idle read requests, get them queued */
	while ((req = req_get(&g_usb_mtp_context.rx_reqs))) {
		req->length = BULK_BUFFER_SIZE;
		ret = usb_ep_queue(g_usb_mtp_context.bulk_out, req, GFP_ATOMIC);
		if (ret < 0) {
			mtp_err("error %d\n", ret);
			g_usb_mtp_context.error = 1;
			req_put(&g_usb_mtp_context.rx_reqs, req);
		}
	}
}

static int mtp_function_set_alt(struct usb_function *f,
		unsigned intf, unsigned alt)
{
	int ret;
//jaeho.cho 2010.11.30
	printk(KERN_ERR "LG_FW :: %s()\n", __func__);

	printk(KERN_DEBUG "%s intf=%d alt=%d\n", __func__, intf, alt);
	ret = usb_ep_enable(g_usb_mtp_context.bulk_in,
			ep_choose(g_usb_mtp_context.cdev->gadget,
				&hs_bulk_in_desc,
				&fs_bulk_in_desc));
	if (ret)
		return ret;
	ret = usb_ep_enable(g_usb_mtp_context.bulk_out,
			ep_choose(g_usb_mtp_context.cdev->gadget,
				&hs_bulk_out_desc,
				&fs_bulk_out_desc));
	if (ret) {
		usb_ep_disable(g_usb_mtp_context.bulk_in);
		return ret;
	}

	ret = usb_ep_enable(g_usb_mtp_context.intr_in,
			ep_choose(g_usb_mtp_context.cdev->gadget,
				&hs_intr_in_desc,
				&fs_intr_in_desc));
	if (ret) {
		usb_ep_disable(g_usb_mtp_context.bulk_in);
		usb_ep_disable(g_usb_mtp_context.bulk_out);
		return ret;
	}

	g_usb_mtp_context.cur_read_req = 0;
	g_usb_mtp_context.read_buf = 0;
	g_usb_mtp_context.data_len = 0;

	/* we're online -- get all rx requests queued */
	start_out_receive();

	g_usb_mtp_context.online = 1;
	g_usb_mtp_context.cancel = 0;
	g_usb_mtp_context.ctl_cancel = 0;
	g_usb_mtp_context.error = 0;

	/* readers may be blocked waiting for us to go online */
	wake_up(&g_usb_mtp_context.rx_wq);
	return 0;
}

#define MTP_MOD_VENDOR_CODE   0x1C
static int  mtp_ext_id = 4;
static unsigned char mtp_ext_desc[] =
"\050\000\000\000\000\001\004\000\001\000\000\000\000\000\000\000\000\001"
"\115\124\120\000\000\000\000\000\060\060\000\000\000\000\000\000\000\000"
"\000\000\000\000";
static int  mtp_ext_str_idx = 238;

static int mtp_function_setup(struct usb_function *f,
					const struct usb_ctrlrequest *ctrl)
{
	int	value = -EOPNOTSUPP;
	u16     wIndex = le16_to_cpu(ctrl->wIndex);
	u16     wLength = le16_to_cpu(ctrl->wLength);
	struct usb_composite_dev *cdev = f->config->cdev;
	struct usb_request	*req = cdev->req;

//    int result = -EOPNOTSUPP;

	mtp_debug("bRequestType=0x%x bRequest=0x%x wIndex=0x%x wLength=0x%x\n",
		ctrl->bRequestType, ctrl->bRequest, wIndex, wLength);

	switch (ctrl->bRequestType & USB_TYPE_MASK) {
	case USB_TYPE_VENDOR:
		switch (ctrl->bRequest) {
		case MTP_MOD_VENDOR_CODE:
			if (wIndex == mtp_ext_id) {
				memcpy(req->buf, mtp_ext_desc,
						sizeof(mtp_ext_desc));
				if (wLength < mtp_ext_desc[0])
					value = wLength;
				else
					value = mtp_ext_desc[0];

				req->zero = 0;
				req->length = value;
				if (usb_ep_queue(cdev->gadget->ep0, req,
					GFP_ATOMIC))
					mtp_err("ep0 in queue failed\n");
			}
			break;
		default:
			break;
		}
		break;
	case USB_TYPE_CLASS:
		switch (ctrl->bRequest) {
		case MTP_CLASS_CANCEL_REQ:
		case MTP_CLASS_GET_EXTEND_EVEVT_DATA:
		case MTP_CLASS_RESET_REQ:
		case MTP_CLASS_GET_DEVICE_STATUS:
#ifdef CONFIG_LGE_USB_GADGET_MTP_DRIVER
         g_bRequest = ctrl->bRequest;

		 if(g_bRequest == MTP_CLASS_CANCEL_REQ)
		 {
		   lg_mtp_debug("LG_FW : MTP CANCEL Request PC => Device!!\n");
		   cancel_noti = 1;
		 }
		 else if(g_bRequest == MTP_CLASS_GET_DEVICE_STATUS)
		 {
		   lg_mtp_debug("LG_FW : MTP GET DEVICE Request PC => Device!!\n");
		 }
#endif
			mtp_debug("ctl request=0x%x\n", ctrl->bRequest);

			value = 0;
			if ((ctrl->bRequest  == MTP_CLASS_CANCEL_REQ)
				&& wLength == MTP_CANCEL_REQ_DATA_SIZE) {
				value = wLength;
				req->zero = 0;
				req->length = wLength;

                lg_mtp_debug("LG_FW : MTP Cancel Request Length [%d] \n", wLength);
				if (usb_ep_queue(cdev->gadget->ep0,
						req, GFP_ATOMIC)) {
					mtp_err("ep0 out queue failed\n");
				}
			} 
			break;

		default:
			break;
		}
	}

	mtp_debug("return value=%d\n", value);
	return value;

}

#ifdef CONFIG_LGE_USB_GADGET_FUNC_BIND_ONLY_INIT
int  mtp_function_add(struct usb_configuration *c)
#else
int  mtp_function_add(struct usb_composite_dev *cdev,
	struct usb_configuration *c)
#endif
{
/* 414003 patch
	int ret = 0;
	int status;

	init_waitqueue_head(&g_usb_mtp_context.rx_wq);
	init_waitqueue_head(&g_usb_mtp_context.tx_wq);
	init_waitqueue_head(&g_usb_mtp_context.ctl_rx_wq);
	init_waitqueue_head(&g_usb_mtp_context.ctl_tx_wq);

	INIT_LIST_HEAD(&g_usb_mtp_context.rx_reqs);
	INIT_LIST_HEAD(&g_usb_mtp_context.rx_done_reqs);
	INIT_LIST_HEAD(&g_usb_mtp_context.tx_reqs);
	INIT_LIST_HEAD(&g_usb_mtp_context.ctl_rx_reqs);
	INIT_LIST_HEAD(&g_usb_mtp_context.ctl_rx_done_reqs);

	status = usb_string_id(c->cdev);
	if (status >= 0) {
		mtp_string_defs[STRING_INTERFACE].id = status;
        intf_desc.iInterface = status;
	}

	mtp_string_defs[STRING_MTP].id = mtp_ext_str_idx;

#ifdef CONFIG_LGE_USB_GADGET_FUNC_BIND_ONLY_INIT
    g_usb_mtp_context.cdev = c->cdev;
#else
	g_usb_mtp_context.cdev = cdev;
#endif
	g_usb_mtp_context.function.name = "mtp";
	g_usb_mtp_context.function.descriptors = fs_mtp_descs;
	g_usb_mtp_context.function.hs_descriptors = hs_mtp_descs;
	g_usb_mtp_context.function.strings = mtp_strings;
	g_usb_mtp_context.function.bind = mtp_function_bind;
	g_usb_mtp_context.function.unbind = mtp_function_unbind;
	g_usb_mtp_context.function.setup = mtp_function_setup;
	g_usb_mtp_context.function.set_alt = mtp_function_set_alt;
	g_usb_mtp_context.function.disable = mtp_function_disable;

#ifndef CONFIG_LGE_USB_GADGET_FUNC_BIND_ONLY_INIT
	ret = mtp_function_init();
*/
	struct mtp_dev *dev;
	int ret = 0;

	printk(KERN_INFO "mtp_bind_config\n");

	dev = kzalloc(sizeof(*dev), GFP_KERNEL);
	if (!dev)
		return -ENOMEM;

	/* allocate a string ID for our interface */
	if (mtp_string_defs[INTERFACE_STRING_INDEX].id == 0) {
		ret = usb_string_id(c->cdev);
		if (ret < 0)
			return ret;
		mtp_string_defs[INTERFACE_STRING_INDEX].id = ret;
		mtp_interface_desc.iInterface = ret;
	}

	spin_lock_init(&dev->lock);
	init_waitqueue_head(&dev->read_wq);
	init_waitqueue_head(&dev->write_wq);
	init_waitqueue_head(&dev->intr_wq);
	atomic_set(&dev->open_excl, 0);
	atomic_set(&dev->ioctl_excl, 0);
	INIT_LIST_HEAD(&dev->tx_idle);

	dev->wq = create_singlethread_workqueue("f_mtp");
	if (!dev->wq)
		goto err1;
	INIT_WORK(&dev->send_file_work, send_file_work);
	INIT_WORK(&dev->receive_file_work, receive_file_work);

	dev->cdev = c->cdev;
	dev->function.name = "mtp";
	dev->function.strings = mtp_strings,
	dev->function.descriptors = fs_mtp_descs;
	dev->function.hs_descriptors = hs_mtp_descs;
	dev->function.bind = mtp_function_bind;
	dev->function.unbind = mtp_function_unbind;
	dev->function.setup = mtp_function_setup;
	dev->function.set_alt = mtp_function_set_alt;
	dev->function.disable = mtp_function_disable;

	/* MTP mode by default */
	dev->interface_mode = MTP_INTERFACE_MODE_MTP;

	/* _mtp_dev must be set before calling usb_gadget_register_driver */
	_mtp_dev = dev;

	ret = misc_register(&mtp_device);

	if (ret)
		goto mtp_exit;

	ret = misc_register(&mtp_enable_device);
	if (ret)
		goto misc_deregister;
#endif

#ifdef CONFIG_LGE_USB_GADGET_FUNC_BIND_ONLY_INIT
	/* start disabled */
	g_usb_mtp_context.function.disabled = 1;
#endif

	ret = usb_add_function(c, &g_usb_mtp_context.function);
	if (ret) {
		mtp_err("MTP gadget driver failed to initialize\n");
		return ret;
	}

	return 0;

#ifndef CONFIG_LGE_USB_GADGET_FUNC_BIND_ONLY_INIT
mtp_misc_deregister:
	misc_deregister(&mtp_enable_device);
mtp_exit:
	mtp_function_exit();
err2:
	misc_deregister(&mtp_device);
err1:
	if (dev->wq)
		destroy_workqueue(dev->wq);
	kfree(dev);
	printk(KERN_ERR "mtp gadget driver failed to initialize\n");

	return ret;
#endif

}

struct usb_function *mtp_function_enable(int enable, int id)
{
	printk(KERN_DEBUG "%s enable=%d id=%d\n", __func__, enable, id);
	if (enable) {
		g_usb_mtp_context.function.descriptors = fs_mtp_descs;
		g_usb_mtp_context.function.hs_descriptors = hs_mtp_descs;
		intf_desc.bInterfaceNumber = id;
	} else {
		g_usb_mtp_context.function.descriptors = null_mtp_descs;
		g_usb_mtp_context.function.hs_descriptors = null_mtp_descs;
	}
	return &g_usb_mtp_context.function;
}

#if defined(CONFIG_LGE_USB_GADGET_MTP_DRIVER)
int mtp_function_init(void)
{
    int ret;
#if !defined(CONFIG_LGE_USB_GADGET_MTP_DRIVER)
    struct proc_dir_entry *mtp_proc = NULL;
#endif
    ret = misc_register(&mtp_device);
    if (ret)
    {
	    printk(KERN_ERR "mtp device failed to initialize\n");
    }
    ret = misc_register(&mtp_csr_device);
    
    if (ret)
    {
	    printk(KERN_ERR "mtp csr device failed to initialize\n");
    }
    
#if !defined(CONFIG_LGE_USB_GADGET_MTP_DRIVER)
    mtp_proc = create_proc_entry("mtpctl", 0666, 0);
    if (!mtp_proc) {
	   mtp_err("creating /proc/mtpctl failed\n");
         return 1;
    }
    mtp_proc->proc_fops = &mtp_ctl_fops;
#endif
    return ret;
}

void mtp_function_exit(void)
{
	misc_deregister(&mtp_device);
    misc_deregister(&mtp_csr_device);
}
#endif

#ifdef CONFIG_LGE_USB_GADGET_FUNC_BIND_ONLY_INIT
static struct android_usb_function mtp_function = {
	.name = "mtp",
	.bind_config = mtp_function_add,
};


static int __init init(void)
{
	printk(KERN_INFO "f_mtp init\n");
	android_register_function(&mtp_function);
	return 0;
}
module_init(init);
#endif
