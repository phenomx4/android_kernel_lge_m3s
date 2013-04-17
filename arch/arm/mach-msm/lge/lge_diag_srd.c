#include <linux/module.h>
#include <mach/lge_diagcmd.h>
#include <linux/input.h>
#include <linux/syscalls.h>
#include <linux/slab.h>

#include <mach/lge_diag_communication.h>
#include <mach/lge_diag_srd.h>
#include <mach/userDataBackUpDiag.h>
#include <../smd_private.h>
#include <linux/delay.h>

#ifndef SKW_TEST
#include <linux/fcntl.h> 
#include <linux/fs.h>
#include <linux/uaccess.h>
#endif

#ifdef CONFIG_LGE_DIAG_SRD  //kabjoo.choi

#define SIZE_OF_SHARD_RAM  0x20000  //128K : check write_size at LGF_SRD() after NV Backup list fixed
#define USE_SHARD_RAM
//#define SRD_BACKUP_DIABLE 

static struct diagcmd_dev *diagpdev;

extern PACK(void *) LGF_SRD (PACK (void *)req_pkt_ptr, uint16 pkg_len);
extern void diag_SRD_Init(srd_req_type * req_pkt, srd_rsp_type * rsp_pkt);
extern void diag_userDataBackUp_entrySet(srd_req_type * req_pkt, srd_rsp_type * rsp_pkt, script_process_type MODEM_MDM );
extern boolean writeBackUpNVdata( char * ram_start_address , unsigned int size);
extern void diag_userDataBackUp_data(srd_req_type *req_pkt, srd_rsp_type *rsp_pkt);	//CSFB SRD

extern int lge_erase_block(int secnum, size_t size);
extern int lge_write_block(int secnum, unsigned char *buf, size_t size);
extern int lge_read_block(int secnum, unsigned char *buf, size_t size);

extern PACK(void *) diagpkt_alloc (diagpkt_cmd_code_type code, unsigned int length);
extern PACK(void *) diagpkt_free (PACK(void *)pkt);

extern unsigned int srd_bytes_pos_in_emmc ;
unsigned char * load_srd_kernel_base;	//CSFB SRD

#ifdef USE_SHARD_RAM
extern void remote_rpc_srd_cmmand(void*pReq, void* pRsp );
extern void *smem_alloc(unsigned id, unsigned size);

unsigned char * load_srd_shard_base;
#endif


PACK (void *)LGF_SRD (
			PACK (void	*)req_pkt_ptr,	/* pointer to request packet  */
			uint16		pkt_len )		      /* length of request packet   */
{
  srd_req_type *req_ptr = (srd_req_type *) req_pkt_ptr;
  srd_rsp_type *rsp_ptr;
  int write_size=0 , mtd_op_result=0;
  unsigned int rsp_len;

  diagpdev = diagcmd_get_dev();

  rsp_len = sizeof(srd_rsp_type);
  rsp_ptr = (srd_rsp_type *)diagpkt_alloc(DIAG_SRD_F, rsp_len);


  // DIAG_TEST_MODE_F_rsp_type union type is greater than the actual size, decrease it in case sensitive items

  printk(KERN_INFO "[SRD] cmd_code : [0x%X], sub_cmd : [0x%X] --> goto MODEM through oem rapi\n",req_ptr->header.cmd_code, req_ptr->header.sub_cmd);
  printk(KERN_INFO "[SRD] backup : [0x%X], class : [0x%X] --> goto MODEM through oem rapi\n",req_ptr->req_data.do_dl_entry.backup_used, req_ptr->req_data.do_dl_entry.backup_used);

#ifdef SRD_BACKUP_DIABLE
  /* DISABLE SRD BACKUP FOR NpstDLL NV BACKUP */
#else
  switch(req_ptr->header.sub_cmd)
  {
      case  SRD_INIT_OPERATION:       
        diag_SRD_Init(req_ptr,rsp_ptr);             
        break;
        
      case USERDATA_BACKUP_REQUEST:
        printk(KERN_WARNING "USERDATA_BACKUP_REQUEST");
        //CSFB SRD remote 제거 -> diag_userDataBackUp_data 에서 처리함
        //userDataBackUpStart() 여기서 ... shared ram 저장 하도록. .. 
        #ifdef USE_SHARD_RAM
        remote_rpc_srd_cmmand(req_ptr, rsp_ptr);  //userDataBackUpStart() 여기서 ... shared ram 저장 하도록. .. 
        diag_userDataBackUp_entrySet(req_ptr,rsp_ptr,0);  //write info data ,  after rpc respons include write_sector_counter  

        rsp_ptr->header.dwsize = rsp_len;

        //todo ..  rsp_prt->header.write_sector_counter,  how about checking  no active nv item  ; 
        // write ram data to emmc misc partition  as many as retruned setor counters 
         load_srd_shard_base=smem_alloc(SMEM_ID_VENDOR2, SIZE_OF_SHARD_RAM);  //384K byte 
        
         if (load_srd_shard_base ==NULL)
         {
           ((srd_rsp_type*)rsp_ptr)->header.err_code = UDBU_ERROR_CANNOT_COMPLETE;  
           break;
           // return rsp_ptr;
         }          
          
         write_size= rsp_ptr->rsp_data.write_sector_counter *256;  //return nv backup counters  

         if( write_size >SIZE_OF_SHARD_RAM)
         {
           printk(KERN_ERR "[SRD BACKUP] Overrun Max size : %d > %d (cnt:%d)",write_size,SIZE_OF_SHARD_RAM,
             rsp_ptr->rsp_data.write_sector_counter);
           ((srd_rsp_type*)rsp_ptr)->header.err_code = UDBU_ERROR_CANNOT_COMPLETE;  //hue..
            break;
         }
         else
         {
           printk(KERN_INFO "[SRD BACKUP] Used shared mem size : %d (Max:%d cnt:%d)",write_size,SIZE_OF_SHARD_RAM,
            rsp_ptr->rsp_data.write_sector_counter); 
         }

         load_srd_kernel_base=kmalloc((size_t)write_size, GFP_KERNEL);
//nuribom_ninedragon[START]
		 if(load_srd_kernel_base)
			 memcpy(load_srd_kernel_base,load_srd_shard_base,write_size); 
		  else
		  {
			printk(KERN_ERR "[SRD BACKUP] kmalloc cannot alloc : %d )",write_size);
			((srd_rsp_type*)rsp_ptr)->header.err_code = UDBU_ERROR_CANNOT_COMPLETE;  //hue..
			 break;
		  }
//memcpy(load_srd_kernel_base,load_srd_shard_base,write_size); 
//nuribom_ninedragon[END]

         //srd_bytes_pos_in_emmc+512 means that info data already writed at emmc first sector 
         mtd_op_result = lge_write_block(srd_bytes_pos_in_emmc+512, load_srd_kernel_base, write_size);  //512 info data 

        
         if(mtd_op_result!= write_size)
         {
           ((srd_rsp_type*)rsp_ptr)->header.err_code = UDBU_ERROR_CANNOT_COMPLETE;  
           kfree(load_srd_kernel_base);
           break;
           //return rsp_ptr;
                    
         }
         kfree(load_srd_kernel_base);
         
         #else // USE_SHARD_RAM
         diag_userDataBackUp_entrySet(req_ptr,rsp_ptr,0);  //write info data ,  after rpc respons include write_sector_counter  
 
         //CSFB SRD
         // Shared RAM 을 사용하지 않고 mach/LS696_nv_backup_list.h 에 있는 아이템을 하나씩 RPC 로 읽어와서 커널 RAM에 저장 
         load_srd_kernel_base=kmalloc(SIZE_OF_SHARD_RAM, GFP_KERNEL);  //384K byte 
         diag_userDataBackUp_data(req_ptr, rsp_ptr);
         
         //todo ..  rsp_prt->header.write_sector_counter,  how about checking  no active nv item  ; 
         // write ram data to emmc misc partition  as many as retruned setor counters 
          
         if (load_srd_kernel_base ==NULL)
         {
           printk(KERN_ERR "[SRD BACKUP] UDBU_ERROR_CANNOT_COMPLETE : load_srd_baseis NULL");
           ((srd_rsp_type*)rsp_ptr)->header.err_code = UDBU_ERROR_CANNOT_COMPLETE;  
           break;
           // return rsp_ptr;
         }          
           
         write_size= rsp_ptr->rsp_data.write_sector_counter *256;  //return nv backup counters  
 
         if( write_size >SIZE_OF_SHARD_RAM)
         {
           printk(KERN_ERR "[SRD BACKUP] UDBU_ERROR_CANNOT_COMPLETE : write_size %d >SIZE_OF_SHARD_RAM %d", write_size, SIZE_OF_SHARD_RAM);
           ((srd_rsp_type*)rsp_ptr)->header.err_code = UDBU_ERROR_CANNOT_COMPLETE;  //hue..
           break;
         }
 
          //srd_bytes_pos_in_emmc+512 means that info data already writed at emmc first sector 
         mtd_op_result = lge_write_block(srd_bytes_pos_in_emmc+512, load_srd_kernel_base, write_size);  //512 info data
 
         
         if(mtd_op_result!= write_size)
         {
           printk(KERN_ERR "[SRD BACKUP] UDBU_ERROR_CANNOT_COMPLETE : mtd_op_result!= write_size");
           ((srd_rsp_type*)rsp_ptr)->header.err_code = UDBU_ERROR_CANNOT_COMPLETE;  
           kfree(load_srd_kernel_base); //CSFB SRD
           break;
           
         }
         kfree(load_srd_kernel_base); //CSFB SRD
         #endif         
         break;

      case USERDATA_BACKUP_REQUEST_MDM:
        break;

      case GET_DOWNLOAD_INFO :
        break;

      case EXTRA_NV_OPERATION :
      #ifdef LG_FW_SRD_EXTRA_NV       
        diag_extraNv_entrySet(req_ptr,rsp_ptr);
      #endif
        break;
        
      case PRL_OPERATION :
      #ifdef LG_FW_SRD_PRL        
        diag_PRL_entrySet(req_ptr,rsp_ptr);
      #endif
        break;
        
      default :
          rsp_ptr =NULL; //(void *) diagpkt_err_rsp (DIAG_BAD_PARM_F, req_ptr, pkt_len);
        break;    
  }
#endif

  if (!rsp_ptr)
    return 0;

  return (rsp_ptr);
}
EXPORT_SYMBOL(LGF_SRD);

#endif // CONFIG_LGE_DIAG_SRD
