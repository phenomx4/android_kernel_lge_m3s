EXTRA_CFLAGS += -Iarch/arm/mach-msm

obj-$(CONFIG_MACH_LGE_M3S) += devices_lge.o
ifeq ($(CONFIG_LGE_AUDIO),y)
obj-$(CONFIG_MACH_LGE_M3S) += board-m3s-sound.o
obj-$(CONFIG_LGE_HEADSET_2WIRE_DETECT) += lge_gpio_h2w.o
obj-$(CONFIG_LGE_PMIC_VIBRATOR) += lge_pmic_vibrator.o
obj-$(CONFIG_LGE_AMP_TPA2055D3) += lge_tpa2055-amp.o

endif
obj-$(CONFIG_MACH_LGE_M3S) += board-m3s-panel.o
obj-$(CONFIG_MACH_LGE_M3S) += board-m3s-input.o

#LGE_CHANGE_S CAMERA FIRMWARE UPDATE (jongkwon.chae@lge.com)
obj-$(CONFIG_MACH_LGE_M3S) += board-m3s-camera.o
#LGE_CHANGE_E CAMERA FIRMWARE UPDATE (jongkwon.chae@lge.com)


#diag
#mtc
obj-$(CONFIG_LGE_DIAG_MTC) += lge_diag_mtc.o
#wmc
obj-$(CONFIG_LGE_DIAG_WMC) += lge_diag_wmc.o
#testmode
obj-$(CONFIG_LGE_DIAG_TESTMODE) += lge_diag_testmode.o
obj-$(CONFIG_LGE_DIAG_TESTMODE) += lge_diag_testmode_sysfs.o
obj-$(CONFIG_LGE_DIAG_TESTMODE) += lge_diag_class.o
obj-$(CONFIG_LGE_DIAG_TESTMODE) += lge_diag_communication.o
obj-$(CONFIG_LGE_DIAG_TESTMODE) += lge_diag_event_log.o
#wifi
obj-$(CONFIG_LGE_DIAG_WIFI) += lge_diag_wifi.o
#key press
obj-$(CONFIG_LGE_DIAG_KEYPRESS) += lge_diag_keypress.o
#screen shot
obj-$(CONFIG_LGE_DIAG_SCREENSHOT) += lge_diag_screen_shot.o
#screen section shot
obj-$(CONFIG_LGE_DIAG_SCREENSHOT) += lge_diag_screen_section_shot.o
#UDM
obj-$(CONFIG_LGE_DIAG_UDM) += lge_diag_udm.o
#icd
obj-$(CONFIG_LGE_DIAG_ICD) += lge_diag_icd.o
#srd
obj-$(CONFIG_LGE_DIAG_SRD) += lge_diag_srd.o
obj-$(CONFIG_LGE_DIAG_SRD) += userDataBackUpDiag.o

#ats
obj-$(CONFIG_LGE_SUPPORT_AT_CMD) += lge_ats.o
obj-$(CONFIG_LGE_SUPPORT_AT_CMD) += lge_ats_cmd.o
obj-$(CONFIG_LGE_SUPPORT_AT_CMD) += lge_ats_eta.o
obj-$(CONFIG_LGE_SUPPORT_AT_CMD) += lge_ats_mmc.o
obj-$(CONFIG_LGE_SUPPORT_AT_CMD) += lge_ats_event_log.o
obj-$(CONFIG_LGE_SUPPORT_AT_CMD) += atcmd_virtual_kbd.o
obj-$(CONFIG_LGE_ATS_UEVENT_DEVICE) += lge_ats_uevent.o
obj-$(CONFIG_LGE_ATS_INPUT_DEVICE) += lge_ats_input.o

#etc
obj-$(CONFIG_LGE_EMMC_SUPPORT) += lge_emmc_direct_access.o
obj-$(CONFIG_MACH_LGE_M3S) += lge_ftmmode.o
obj-$(CONFIG_LGE_SUPPORT_RAPI) += lge_rapi_client.o
obj-y += lge_audio_misc_ctrl.o

obj-$(CONFIG_LGE_ERS) += lge_handle_panic.o
obj-$(CONFIG_LGE_SUSPEND_AUTOTEST) += lge_suspend_autotest.o
obj-$(CONFIG_LGE_ERS) += lge_ers.o
