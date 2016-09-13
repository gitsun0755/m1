#include "testframework.h"

#include "fpp_board.h"
#include "fpp_video.h"
#include "fpp_signal.h"
#include "fpp_audio.h"
#include "fpp_tsi.h"
#include "fpp_linein.h"
#include "fpp_decoder.h"
#include "fpp_demux.h"
#include "fpp_zoom.h"
#include "fpp_inner_demod.h"
#include "fpp_factory.h"
#include "fpp_system.h"
#include "test_tsi.h"

#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include "getline.h"
#include "comm.h"
//#include "test_demux.h"
//#include "test_decoder.h"
//#include "test_demod.h"
//#include "test_zoom.h"
#include "test_tsi.h"

#include "test_board.h"

#include "demux_interface.h"
#include "dtv_channel_interface.h"
#include "util_tv.h"
#include "audio_interface.h"

#include "demod_interface.h"
#include "demux_interface.h"
#include "decoder_interface.h"
#include "dtv_channel_interface.h"

#include "fpi_demod.h"
#include <stdio.h>

#define TTY_PATH "/dev/tty"
#define STTY_US "stty raw -echo -F "
#define STTY_DEF "stty -raw echo -F "
//======================================================================================


//---------------------------------------------------------------------

#define CHECK_FUNC_RETURN(correct_return,func)\
{\
    /*判断接口返回结果,如果与预期一致则无反应,否则会在测试报告中形成记录,详细见接口文档*/\
    TF_CU_ASSERT_EQUAL((func),correct_return,#func,"执行返回值错误");\
}


#define CHECK_FUNC_RETURN_CB(correct_return,func,errcb)\
{\
    int32_t ret = (int32_t)(func);\
    if(ret != (int32_t)(correct_return))\
    {\
        errcb;\
        /*判断接口返回结果,如果与预期一致则无反应,否则会在测试报告中形成记录,详细见接口文档*/\
        TF_CU_ASSERT_EQUAL(ret,correct_return,#func,#func" 执行返回值错误");\
    }\
    else\
        {}\
}

//--------TSI definition----------
#define TSI_TEST_DEBUG 


#define TSI_WAITDISPLAY_TIME_SLEEP  10      //ms
#define TSI_WAITDISPLAY_TIME_MAX    4000    //ms
#define TSI_WAITDISPLAY_CNT_MAX     (TSI_WAITDISPLAY_TIME_MAX/TSI_WAITDISPLAY_TIME_SLEEP)


#define TSI_TEST_TIME_STEP 10//seconds

//#define TSI_TEST_CABLE_FREQUENCY (259000)
#define TSI_TEST_FILE_VPID (0X191)//CCTV-1 Tian Wei Signal
#define TSI_TEST_FILE_APID (0X192)//CCTV-1 Tian Wei Signal
#define TSI_TEST_FILE_PCRPID (0X191)//CCTV-1 Tian Wei Signal
#define TSI_TEST_FILE_PMTPID (75)

#define DEMOD_LOCK_TIMEOUT_MAX	2000



#define TSI_TEST_FILE "/applications/bin/test_tsi.ts"

#ifdef TSI_TEST_DEBUG
    #define DBG_TSI_TEST(fmt,arg...) TEST_LOG_INFO("[--tsi test--]"fmt,##arg)
#else
    #define DBG_TSI_TEST(fmt,arg...)
#endif


typedef struct _ST_DTV_PLAY_INFO
{
    EN_FPP_LINEIN_TYPE_T    en_linein;
    
    ST_FPP_DECODER_INFO_T *pst_decoder_info;
    ST_FPP_ZOOM_WINDOWINFO_T *pst_window_info;
    ST_FPP_DEMUX_INFO_T *pst_demux_info;
    ST_FPP_VIDEO_FORMAT_T st_video_info;
    ST_FPP_SYSTEM_PANEL_PROPERTY_T st_panel_info;
    ST_FPP_DECODER_VCODEC_INFO_T st_video_decoder_info;
    ST_FPP_DECODER_ACODEC_INFO_T st_audio_decoder_info;

    uint32_t un32_memory_demux_id;
    uint32_t un32_video_decoder_id;
    uint32_t un32_audio_decoder_id;

    uint32_t un32_video_channel_id;
    uint32_t un32_pcr_channel_id;
    uint32_t un32_audio_channel_id;
    uint16_t un16_audio_pid;
    uint16_t un16_video_pid;
    uint16_t un16_pcr_pid;

}DTV_PLAY_INFO;


typedef struct
{
    fpi_bool decoder_av_sync;
    fpi_bool decoder_audio_status;
    fpi_bool decoder_video_status;
    fpi_bool b_record_ts;

    uint32_t un32_demuxID;
    uint32_t un32tsi_id;
	
   /*ST_FPP_DECODER_INFO_T *pst_decoder_info;
    ST_FPP_ZOOM_WINDOWINFO_T *pst_window_info;
    ST_FPP_DEMUX_INFO_T *pst_demux_info;
    ST_FPP_VIDEO_FORMAT_T st_video_info;
    ST_FPP_SYSTEM_PANEL_PROPERTY_T st_panel_info;
    ST_FPP_DECODER_VCODEC_INFO_T st_video_decoder_info;
    ST_FPP_DECODER_ACODEC_INFO_T st_audio_decoder_info;

    uint32_t un32_memory_demux_id;
    uint32_t un32_video_decoder_id;
    uint32_t un32_audio_decoder_id;

    uint32_t un32_video_channel_id;
    uint32_t un32_pcr_channel_id;
    uint32_t un32_audio_channel_id;
    uint16_t un16_audio_pid;
    uint16_t un16_video_pid;
    uint16_t un16_pcr_pid;*/

    uint8_t *pu8TsiAddr;
    uint8_t *pu8TsdAddr;
    uint32_t u32TsiSize;
    uint32_t u32BufOffset;
    uint32_t u32TsdSize;
    uint32_t u32TsdBufOffset;
    void *pTsiFileR;
    void *pTsiFileW;
    EN_FPP_TSI_PLAYBACK_MOTION eMotion;
    char file_path[128];
    uint32_t    duration;// seconds
}T_TSI_CTRL;


#define TSI_TEST_TYPE_RECORD      0
#define TSI_TEST_TYPE_PLAYBACK    1
typedef enum{
    E_TESTPB_FF_1X=1,
    E_TESTPB_FF_2X=2,
    E_TESTPB_FF_4X=3,
    E_TESTPB_FF_8X=4,
    E_TESTPB_FF_16X=5,
    E_TESTPB_FF_32X=6,
    E_TESTPB_SLOW_1_2_X=7,
    E_TESTPB_SLOW_1_4_X=8,
    E_TESTPB_SLOW_1_8_X=9,
    E_TESTPB_SLOW_1_16_X=10,
    E_TESTPB_TRICK_MODE_PAUSE_NORMAL=11,
    E_TESTPB_TRICK_MODE_NORMAL=12,
    E_TESTPB_TRICK_MODE_FAST_FORWARD_CHANGE=13,
    E_TESTPB_TRICK_MODE_SCAN=14,
    E_TESTPB_NORMAL_SPEED=15,
    
}E_PLAYBACK_TEST_MODE;




//======================================================================================
static T_TSI_CTRL          Tsi_ctrl_data;

static T_TSI_CTRL          Tsi_ctrl_data1;

static T_TSI_CTRL          Tsi_ctrl_data2;


static EN_FPP_DEMOD_DELIVERY_TYPE_T delivery_type = EN_FPP_DEMOD_DELIVERY_TYPE_DTMB;

static ST_TEST_FPP_PROGRAME_INFO current_program_info;

static DTV_PLAY_INFO play_info;
static fpi_bool b_video_status = fpi_false;
static fpi_bool b_audio_status = fpi_false;
static fpi_bool b_av_sync = fpi_false;

static ST_FPP_DEMUX_INFO_T *pDemuxInfo = NULL;
static ST_FPP_TSPATH_INFO_T *pTsPathInfo = NULL;

char testid=0;
#define T_FRE 578000//666000//474000
#define T_VID 1010//101//1536
#define T_AID 1011//102//2048
#define T_PCRID 1010//101//1536
#define T_VTYPE EN_ES_VIDEO_H264//101//1536
#define T_ATYPE  EN_ES_AUDIO_AAC

//======================================================================================
extern int common_power_rtc_get_clk(uint64_t *pun64_secs);


//===decoder common=======================================================================
fpi_error  _mutitsi_getusbpath(char* path, uint32_t length)
{
    if(NULL == path)
        return FPI_ERROR_FAIL;

   if(!get_configuration_usb_path(path, length+1))
    {
        return FPI_ERROR_FAIL;
    }


    return FPI_ERROR_SUCCESS;
}



fpi_error  _mutitsi_record_virtual_disk_write(uint32_t u32Size)
{
    uint32_t u32SizeLeft=u32Size;
    uint32_t u32ReadSize;
    uint8_t *pu8ReadPointer;
    uint32_t u32WrittenByte;
    
    //DBG_TSI_TEST("%s:%d Old read pointer is 0x%x, write size is %ld\n", __FUNCTION__, __LINE__, (int)(Tsi_ctrl_data.pu8TsiAddr+Tsi_ctrl_data.u32BufOffset),u32Size);
    if(NULL == Tsi_ctrl_data.pTsiFileW)
        return FPI_ERROR_FAIL;

    while(u32SizeLeft)
    {
        #if 1
            CHECK_FUNC_RETURN(FPI_ERROR_SUCCESS, \
                fpp_tsi_get_read_buffer_address(Tsi_ctrl_data.un32tsi_id,\
                                                 (uint32_t **)&pu8ReadPointer,&u32ReadSize));
            if(NULL == pu8ReadPointer)
                return FPI_ERROR_FAIL;
            u32ReadSize *= 4;
            if(u32ReadSize > u32SizeLeft)
                u32ReadSize = u32SizeLeft;
        #else
            u32ReadSize=(Tsi_ctrl_data.u32TsiSize-Tsi_ctrl_data.u32BufOffset)>u32SizeLeft ? u32SizeLeft:(Tsi_ctrl_data.u32TsiSize-Tsi_ctrl_data.u32BufOffset);
            pu8ReadPointer=Tsi_ctrl_data.pu8TsiAddr+Tsi_ctrl_data.u32BufOffset;
        #endif
        
        //here write disk
        if(u32ReadSize)
        {
            u32WrittenByte = fwrite(pu8ReadPointer,sizeof(uint8_t),u32ReadSize,(FILE*)Tsi_ctrl_data.pTsiFileW);
            if(u32WrittenByte != u32ReadSize)
            {
                TEST_LOG_ERROR("%s:%d : fwrite %d -> %d\n", __FUNCTION__, __LINE__, u32ReadSize, u32WrittenByte);
                return FPI_ERROR_FAIL;
            }
            
            CHECK_FUNC_RETURN(FPI_ERROR_SUCCESS, \
                fpp_tsi_confirm_data(Tsi_ctrl_data.un32tsi_id, (uint32_t)pu8ReadPointer, u32ReadSize/4));
        }
        
        Tsi_ctrl_data.u32BufOffset+=u32ReadSize;
        if(Tsi_ctrl_data.u32BufOffset >= Tsi_ctrl_data.u32TsiSize)
        {
            Tsi_ctrl_data.u32BufOffset -= Tsi_ctrl_data.u32TsiSize;
        }
        u32SizeLeft-=u32ReadSize;
    }

    //DBG_TSI_TEST("%s:%d : New read pointer is 0x%x!!\n", __FUNCTION__, __LINE__, (int)(Tsi_ctrl_data.pu8TsiAddr + Tsi_ctrl_data.u32BufOffset));
    
    return FPI_ERROR_SUCCESS;
}

fpi_error  _mutitsi_playback_virtual_disk_read(uint32_t u32Size)
{
    uint32_t u32SizeLeft=u32Size;
    uint32_t u32WriteSize;
    uint8_t *pu8WritePointer;
    uint32_t u32ReadByte;
    
    DBG_TSI_TEST("%s:%d Old write pointer is 0x%x, write size is %d\n", __FUNCTION__, __LINE__, (int)((int)Tsi_ctrl_data.pu8TsdAddr+Tsi_ctrl_data.u32TsdBufOffset),u32Size);
    if(NULL == Tsi_ctrl_data2.pTsiFileR)
        return FPI_ERROR_FAIL;
   // DBG_TSI_TEST("1111%s:%d Old write pointer is 0x%x, write size is %d\n", __FUNCTION__, __LINE__, (int)((int)Tsi_ctrl_data.pu8TsdAddr+Tsi_ctrl_data.u32TsdBufOffset),u32Size);
    while(u32SizeLeft)
    {
        #if 1
            CHECK_FUNC_RETURN(FPI_ERROR_SUCCESS, \
                fpp_tsi_get_write_buffer_address(Tsi_ctrl_data2.un32tsi_id,\
                                                 (uint32_t **)&pu8WritePointer,&u32WriteSize));
            if(NULL == pu8WritePointer)
            	{
    DBG_TSI_TEST("%s:%d fpp_tsi_get_write_buffer_address\n", __FUNCTION__, __LINE__);
                return FPI_ERROR_FAIL;

			}
			u32WriteSize *= 4;
            if(u32WriteSize > u32SizeLeft)
                u32WriteSize = u32SizeLeft;
        #else
            u32WriteSize=(Tsi_ctrl_data.u32TsdSize-Tsi_ctrl_data.u32TsdBufOffset)>u32SizeLeft ? u32SizeLeft:(Tsi_ctrl_data.u32TsdSize-Tsi_ctrl_data.u32TsdBufOffset);
            pu8WritePointer=Tsi_ctrl_data.pu8TsdAddr+Tsi_ctrl_data.u32TsdBufOffset;
        #endif
        
        //here read from disk
        if(u32WriteSize)
        {
            u32ReadByte = fread(pu8WritePointer,sizeof(uint8_t),u32WriteSize,(FILE*)Tsi_ctrl_data2.pTsiFileR);
            if(0 == u32ReadByte)
            {
                TEST_LOG_ERROR("%s:%d : fread data empty: %d -> %d\n", __FUNCTION__, __LINE__, u32WriteSize, u32ReadByte);
                return FPI_ERROR_FAIL;
            }
            u32ReadByte -= u32ReadByte%(188*2);
            CHECK_FUNC_RETURN(FPI_ERROR_SUCCESS, \
                fpp_tsi_confirm_data(Tsi_ctrl_data2.un32tsi_id, (uint32_t)pu8WritePointer, u32ReadByte/4));
        }
        Tsi_ctrl_data2.u32TsdBufOffset += u32ReadByte;//u32WriteSize
        if(Tsi_ctrl_data2.u32TsdBufOffset>=Tsi_ctrl_data2.u32TsdSize)
        {
            Tsi_ctrl_data2.u32TsdBufOffset -=Tsi_ctrl_data2.u32TsdSize;
        }
        u32SizeLeft -= u32ReadByte;
        
        if(u32ReadByte < u32WriteSize)
        {
            TEST_LOG_INFO("%s:%d : fread data is not enough: %d -> %d\n", __FUNCTION__, __LINE__, u32WriteSize, u32ReadByte);
            return FPI_ERROR_FAIL;
        }
    }

    DBG_TSI_TEST("%s:%d : New write pointer is 0x%x!!\n", __FUNCTION__, __LINE__,(int)(Tsi_ctrl_data2.pu8TsdAddr+Tsi_ctrl_data2.u32TsdBufOffset));
    
    return FPI_ERROR_SUCCESS;
}

void _mutitsi_dtv_get_program_info(EN_FPP_DEMOD_DELIVERY_TYPE_T en_delivery_type, ST_TEST_FPP_PROGRAME_INFO * pst_prog_info)
{
	ST_TEST_FPP_PROGRAME_INFO prog_info;

	memset(&prog_info,0,sizeof(ST_TEST_FPP_PROGRAME_INFO));
	DTVChannelGetCurrProgInfor(&prog_info);

       
	pst_prog_info->u32VideoPID = prog_info.u32VideoPID;
   	pst_prog_info->u32AudioPID = prog_info.u32AudioPID;
	pst_prog_info->u32PCRPID = prog_info.u32PCRPID;
	pst_prog_info->u8VideoType = prog_info.u8VideoType;
	pst_prog_info->u8AudioType = prog_info.u8AudioType;

	if(en_delivery_type == EN_FPP_DEMOD_DELIVERY_TYPE_CABLE)
	{
		pst_prog_info->u32VideoPID = 0x191;
   		pst_prog_info->u32AudioPID = 0x192;
		pst_prog_info->u32PCRPID = 0x191;
		pst_prog_info->u8VideoType = EN_ES_VIDEO_MPEG2VIDEO;
		pst_prog_info->u8AudioType = EN_ES_AUDIO_MPEG;
	}
	else if(en_delivery_type == EN_FPP_DEMOD_DELIVERY_TYPE_TERRESTRIAL)
	{
		pst_prog_info->u32VideoPID = T_VID;//0x101;
   		pst_prog_info->u32AudioPID =T_AID;// 0x102;
		pst_prog_info->u32PCRPID =T_PCRID;// 0x101;
		pst_prog_info->u8VideoType = T_VTYPE;
		pst_prog_info->u8AudioType = T_ATYPE;
	}
}


static void _mutitsi_fpp_decoder_notify_callback( uint32_t u32decoderID,EN_FPP_DECODER_MODE_T en_mode_type,void * parameter)
{
    if(u32decoderID == play_info.un32_video_decoder_id)
    {
        if(en_mode_type == EN_FPP_DECODE_VIDEO_DATA_ERRO)
        {
            b_video_status = fpi_false;
        }
        else if(en_mode_type == EN_FPP_DECODE_VIDEO_NORMAL)
        {
            b_video_status = fpi_true;
        }
        else if(en_mode_type == EN_FPP_DECODE_VIDEO_UNSUPPORT)
        {
            b_video_status = fpi_false;
        }    
        else if(en_mode_type == EN_FPP_DECODE_AV_SYNC_DONE)
        {
            b_av_sync = fpi_true;
        }    
    }
    else if(u32decoderID == play_info.un32_audio_decoder_id)
    {
        if(en_mode_type == EN_FPP_DECODE_AUDIO_DATA_ERRO)
        {
            b_audio_status = fpi_false;
        }
        else if(en_mode_type == EN_FPP_DECODE_AUDIO_NORMAL)
        {
            b_audio_status = fpi_true;
        }
        else if(en_mode_type == EN_FPP_DECODE_AUDIO_UNSUPPORT)
        {
            b_audio_status = fpi_false;
        }    
        else if(en_mode_type == EN_FPP_DECODE_AV_SYNC_DONE)
        {
            b_av_sync = fpi_true;
        }    
    }    
        
}



ST_FPP_TSPATH_INFO_T * _mutitsi_get_demod_instance(EN_FPP_DEMOD_DELIVERY_TYPE_T deliverytype)
{
	fpi_error err = FPI_ERROR_FAIL;
	uint32_t tsCount =0;	
	uint32_t index =0;	

	if(FPI_ERROR_SUCCESS != fpp_inner_demod_get_ts_count(&tsCount))
	{
		TF_CU_ASSERT_EQUAL(FPI_ERROR_FAIL,FPI_ERROR_SUCCESS,"fpp_inner_demod_get_ts_count","get demod tspath Count fail !");
		return NULL ;
	}
	
	if(pTsPathInfo == NULL)
	{
		pTsPathInfo = (ST_FPP_TSPATH_INFO_T *)malloc( tsCount * sizeof(ST_FPP_TSPATH_INFO_T));
		memset(pTsPathInfo,0,tsCount * sizeof(ST_FPP_TSPATH_INFO_T));

              
		err = fpi_demod_init();
		TF_CU_ASSERT_EQUAL(err,FPI_ERROR_SUCCESS,"fpi_demod_init","get demod tspath fpi_demod_init fail !");


		if(NULL != pTsPathInfo)
		{
			if(FPI_ERROR_SUCCESS != fpp_inner_demod_get_device_info(pTsPathInfo))
			{
				printf("get demod info fail \n");
				TF_CU_ASSERT_EQUAL(FPI_ERROR_FAIL,FPI_ERROR_SUCCESS,"fpp_inner_demod_get_device_info","fpp_inner_demod_get_device_info fail !");
			}
		}
	}	
		printf("\n\r0000:%s,%d\n\r",__FUNCTION__,__LINE__);
	for(index=0;index<tsCount;index++)
	{
		if((pTsPathInfo[index].un32_desc & 0xFFFFFF) & (1<<deliverytype))
		{
			printf("\n un32_innerdemodID =%d un32_tspath=%d un32_desc= 0x%x\n",
			pTsPathInfo[index].un32_innerdemodID,pTsPathInfo[index].un32_tspath,pTsPathInfo[index].un32_desc);
			break;
		}		
	}

	if (index == tsCount)
	{
		printf("\n\r0000:%s,%d\n\r",__FUNCTION__,__LINE__);

		TF_CU_ASSERT_EQUAL(FPI_ERROR_UNSUPPORT,FPI_ERROR_SUCCESS,"fpi_demod_init","get demod tspath fpi_demod_init fail !");
		return NULL;
	}
	else
	{
			printf("\n\r0000:%s,%d\n\r",__FUNCTION__,__LINE__);

		return &(pTsPathInfo[index]);
	}
}


static ST_FPP_DEMUX_INFO_T * _mutitsi_get_demux_instance(EN_FPP_DEMUX_SOURCE_TYPE_T  DemoSourceType)
{
	fpi_error err = FPI_ERROR_FAIL;
	int i=0;
	uint32_t dmxCount = 0;	

	if(FPI_ERROR_SUCCESS != fpp_demux_get_device_count(&dmxCount)) 	
	{
		TF_CU_ASSERT_EQUAL(FPI_ERROR_FAIL,FPI_ERROR_SUCCESS,"fpp_demux_get_device_count","获取 demux设备数量 失败");
		return NULL ;
	}
	printf("\n\r0000:%s,%d,demux count:%d\n\r",__FUNCTION__,__LINE__,dmxCount);
	if(pDemuxInfo == NULL)
	{
		pDemuxInfo = (ST_FPP_DEMUX_INFO_T *)malloc(sizeof(ST_FPP_DEMUX_INFO_T)*dmxCount);
		if(NULL == pDemuxInfo)
		{
			TF_CU_ASSERT_EQUAL(FPI_ERROR_FAIL,FPI_ERROR_SUCCESS,"malloc","malloc 失败");
			return NULL;
		}

		err = fpp_demux_get_device_info(pDemuxInfo);
		TF_CU_ASSERT_EQUAL(err,FPI_ERROR_SUCCESS,"fpp_demux_get_device_info","获取 demux设备信息 失败");
	}


	
	for(i=0;i<dmxCount;i++)
	{	
		if( (EN_FPP_DEMUX_SOURCE_TYPE_T)pDemuxInfo[i].un32_demuxDesc == DemoSourceType)
		{						
			printf("\n un32_demuxID = %d  un32_demuxDesc =%d\n",pDemuxInfo[i].un32_demuxID,pDemuxInfo[i].un32_demuxDesc);
			break;
		}
	}

	if(i == dmxCount)
	{
		TF_CU_ASSERT_EQUAL(FPI_ERROR_UNSUPPORT,FPI_ERROR_SUCCESS,"fpi_demod_init","get demod tspath fpi_demod_init fail !");
		return NULL;
	}

 	return &(pDemuxInfo[i]);	
}	


static fpi_error _mutitsi_start_tuner(uint8_t tuner_id,ST_FPP_DEMOD_DELIVERY_T st_delivery)
{
	fpi_error err = FPI_ERROR_FAIL;
	
	if(tuner_id == 0)
	{
		err = fpp_inner_demod_set_active(st_delivery.tspath,1);
		//TF_CU_ASSERT_EQUAL(err,FPI_ERROR_SUCCESS,"fpp_inner_demod_set_active", "fpp_inner_demod_set_active failed");

		err = fpp_inner_demod_change_delivery(st_delivery.tspath,st_delivery.delivery_type);
		//TF_CU_ASSERT_EQUAL(err,FPI_ERROR_SUCCESS,"fpp_inner_demod_change_delivery", "fpp_inner_demod_change_delivery failed");
			printf("\n\r0000:%s,%d  tuner 0\n\r",__FUNCTION__,__LINE__);
		err = fpp_inner_demod_start_tuning(&st_delivery, EN_FPP_DEMOD_CONNECT_MODE_SCAN);


		//TF_CU_ASSERT_EQUAL(err,FPI_ERROR_SUCCESS,"fpp_inner_demod_start_tuning", "fpp_inner_demod_start_tuning failed");
	}
	else if(tuner_id == 1)
	{
		err = fpi_demod_set_active(&st_delivery,1);
        	//TF_CU_ASSERT_EQUAL(err,FPI_ERROR_SUCCESS,"fpi_demod_set_active", "fpi_demod_set_active failed");

        	err = fpi_demod_change_delivery(&st_delivery);
		//err = fpi_demod_set_active(&st_delivery,1);
		//TF_CU_ASSERT_EQUAL(err,FPI_ERROR_SUCCESS,"fpi_demod_change_delivery", "fpi_demod_change_delivery failed");
			printf("\n\r0000:%s,%d tuner1 \n\r",__FUNCTION__,__LINE__);
        	err = fpi_demod_start_connect(&st_delivery,EN_FPP_DEMOD_CONNECT_MODE_SCAN);
			printf("\n\r0000:%s,%d tuner1,%d \n\r",__FUNCTION__,__LINE__,err);
	}
	return err;

}



static fpi_bool _mutitsi_get_lock_state(uint8_t tuner_id,ST_FPP_DEMOD_DELIVERY_T stDelivery)
{
	uint32_t u32Curtime =  getcurTime();
 	fpi_bool lock_state = fpi_false;
	uint32_t timeout;
	fpi_error err = FPI_ERROR_FAIL;

	if(tuner_id == 0)
	{
		err = fpp_inner_demod_get_info(stDelivery.tspath,EN_FPP_INNER_DEMOD_PARAMETER_GET_TIMEOUT,&timeout);
	}
	else if(tuner_id == 1)
	{
		timeout = DEMOD_LOCK_TIMEOUT_MAX;
	}
	else
	{
		timeout = 0;
	}

	printf("timeout =  %d\n",timeout);

	while(TimeDiffFromNow(u32Curtime)  < timeout)
	{
		if(tuner_id == 0)
		{
			/* The return value means that the fpi_demod_get_lock_status excutes successfully rather than the lock state is unlock.  */
			err = fpp_inner_demod_get_info(stDelivery.tspath,EN_FPP_INNER_DEMOD_PARAMETER_GET_LOCK_STATUS,&lock_state);
		}
		else if(tuner_id == 1)
		{
		    err = fpi_demod_get_lock_status(&stDelivery,&lock_state);
	
		}
		else
		{

		}
		if(lock_state == fpi_true)
			break;
	}

	if(err != FPI_ERROR_SUCCESS)
	{
		lock_state = fpi_false;
	}
	return  lock_state;			
}



static fpi_error _mutitsi_stop_tuner(uint8_t tuner_id,ST_FPP_DEMOD_DELIVERY_T stDelivery)
{
	fpi_error err = FPI_ERROR_FAIL;

	if(tuner_id == 0)
	{        
		err = fpp_inner_demod_set_active(stDelivery.tspath,fpi_false);
	    TF_CU_ASSERT_EQUAL(err,FPI_ERROR_SUCCESS,"fpp_inner_demod_set_active","set inner demod unactive failed");

		err = fpp_inner_demod_stop_tuning(stDelivery.tspath);
	    TF_CU_ASSERT_EQUAL(err,FPI_ERROR_SUCCESS,"fpp_inner_demod_stop_tuning","cancle inner demod tuning failed");
	}
	else if(tuner_id == 1)
	{
        err = fpi_demod_set_active(&stDelivery,fpi_false);
        TF_CU_ASSERT_EQUAL(err,FPI_ERROR_SUCCESS,"fpi_demod_set_active", "fpi_demod_set_active failed");

	}

	return err;
}



static fpi_error _mutitsi_demux_set_source_type(ST_FPP_DEMUX_INFO_T *pstDmuxinfo,EN_FPP_DTV_TS_PATH_T  en_tspath)
{

	EN_FPP_DEMUX_SOURCE_TYPE_T DemuxSourceType = pstDmuxinfo->un32_demuxDesc;
	uint32_t un32_demuxID = pstDmuxinfo->un32_demuxID;
	fpi_error err;
		printf("\n\r0000:%s,%d\n\r",__FUNCTION__,__LINE__);
	
	TEST_LOG_INFO("Set Demux source type to %d\n",DemuxSourceType);
	printf("Set Demux source type to %d\n",DemuxSourceType);
	err = fpp_demux_set_source_type(un32_demuxID, en_tspath, DemuxSourceType);
	TF_CU_ASSERT_EQUAL(err,FPI_ERROR_SUCCESS,"fpp_demux_set_source_type","设置 demux设备源类型 失败");	

	return err;
}


static fpi_error  _mutitsi_dtv_init(uint32_t test_type,EN_FPP_DEMOD_DELIVERY_TYPE_T en_delivery_type)
{
    fpi_error err = FPI_ERROR_SUCCESS; 
    uint32_t DecoderCount = 0;
    uint32_t WindowCount = 0;
    uint8_t un8_loop = 0;


    Tsi_ctrl_data.decoder_av_sync = fpi_false;
    Tsi_ctrl_data.decoder_audio_status = fpi_false;
    Tsi_ctrl_data.decoder_video_status = fpi_false;
    Tsi_ctrl_data.duration = TSI_TEST_FILE_RECORD_LENGTH_IN_SECONDS;

    if(TSI_TEST_TYPE_RECORD == test_type)
    {
		play_info.un16_video_pid = current_program_info.u32VideoPID;
		play_info.un16_audio_pid = current_program_info.u32AudioPID;
		play_info.un16_pcr_pid = current_program_info.u32PCRPID;
		play_info.st_video_decoder_info.u32VideoType = current_program_info.u8VideoType;
		play_info.st_audio_decoder_info.u32AudioType = current_program_info.u8AudioType;

    }
    else //playback
    {
	if(Tsi_ctrl_data.b_record_ts)
	{
		play_info.un16_video_pid = current_program_info.u32VideoPID;
		play_info.un16_audio_pid = current_program_info.u32AudioPID;
		play_info.un16_pcr_pid = current_program_info.u32PCRPID;
		play_info.st_video_decoder_info.u32VideoType = current_program_info.u8VideoType;
		play_info.st_audio_decoder_info.u32AudioType = current_program_info.u8AudioType;
	}
	else
	{
		play_info.un16_video_pid = 0x191;
		play_info.un16_audio_pid = 0x192;
		play_info.un16_pcr_pid = 0x191;
		play_info.st_video_decoder_info.u32VideoType = EN_ES_VIDEO_MPEG2VIDEO;
		play_info.st_audio_decoder_info.u32AudioType = EN_ES_AUDIO_MPEG;


	}
	 
    }

   #if 0
    
    err = fpp_demux_get_device_count(&DmxCount);

    if(DmxCount > 0)
    {
        play_info.pst_demux_info = (ST_FPP_DEMUX_INFO_T *)malloc(sizeof(ST_FPP_DEMUX_INFO_T)*DmxCount); 
        memset(play_info.pst_demux_info,0,sizeof(ST_FPP_DEMUX_INFO_T)*DmxCount);
        err |= fpp_demux_get_device_info(play_info.pst_demux_info);
        for(un8_loop = 0;un8_loop < DmxCount; un8_loop++)
        {
            if(play_info.pst_demux_info[un8_loop].un32_demuxDesc == EN_FPP_DEMUX_SOURCE_TYPE_MEMORY)
            {
                play_info.un32_memory_demux_id = play_info.pst_demux_info[un8_loop].un32_demuxID;
                break;
            }
        }
        if(un8_loop >= DmxCount)
        {
            play_info.un32_memory_demux_id = play_info.pst_demux_info[0].un32_demuxID;
        }
        
    }
    else
    {
        fpi_err("function = %s,line = %d, fpp_demux_get_device_count fail\n", __FUNCTION__, __LINE__);
        return FPI_ERROR_FAIL;
    }
    #endif

    err |= fpp_decoder_get_device_count(&DecoderCount); 
	        printf("function = %s,line = %d, fpp_decoder_get_DecoderCount_count :%d\n", __FUNCTION__, __LINE__,DecoderCount);

    if(DecoderCount > 0)
    {
        play_info.pst_decoder_info = (ST_FPP_DECODER_INFO_T *)malloc(sizeof(ST_FPP_DECODER_INFO_T)*DecoderCount);
        memset(play_info.pst_decoder_info,0,sizeof(ST_FPP_DECODER_INFO_T)*DecoderCount);
        err |= fpp_decoder_get_device_info(play_info.pst_decoder_info);
    }
    else
    {
        fpi_err("function = %s,line = %d, fpp_decoder_get_device_count fail\n", __FUNCTION__, __LINE__);
        return FPI_ERROR_FAIL;
    }

    err = fpp_linein_open(EN_FPP_LINEIN_DTV,EN_FPP_LINEIN_NONE);
	TF_CU_ASSERT_EQUAL(err,FPI_ERROR_SUCCESS,"fpp_linein_open","open linein  failed");
	

    for(un8_loop = 0; un8_loop < DecoderCount;un8_loop++)
    {
        if((play_info.pst_decoder_info[un8_loop].decodertype == EN_FPP_VIDEO_DECODER))
  {
   play_info.un32_video_decoder_id= play_info.pst_decoder_info[un8_loop].un32_deocderId;


  }

        if((play_info.pst_decoder_info[un8_loop].decodertype == EN_FPP_AUDIO_DECODER))
  {
   play_info.un32_audio_decoder_id = play_info.pst_decoder_info[un8_loop].un32_deocderId;


  }
        err |= fpp_decoder_open(play_info.pst_decoder_info[un8_loop].un32_deocderId);
    }

    err |= fpp_zoom_get_window_count(&WindowCount);
    if(WindowCount > 0)
    {
        play_info.pst_window_info = (ST_FPP_ZOOM_WINDOWINFO_T *)malloc(sizeof(ST_FPP_ZOOM_WINDOWINFO_T)*WindowCount);
        memset(play_info.pst_window_info,0,sizeof(ST_FPP_ZOOM_WINDOWINFO_T)*WindowCount);
        err |= fpp_zoom_get_window_info(play_info.pst_window_info );
    }
    else
    {
        fpi_err("function = %s,line = %d, fpp_zoom_get_window_count fail\n", __FUNCTION__, __LINE__);
        return FPI_ERROR_FAIL;
    }

   
 err |= fpp_system_get_panel_resolution(&play_info.st_panel_info);

  

 err |= fpp_zoom_disconnect_window_input(play_info.pst_window_info[0].un32_windowId);


 
    b_video_status = fpi_false;
    b_audio_status = fpi_false;
    b_av_sync = fpi_false;
    return FPI_ERROR_SUCCESS;
}





static fpi_error  _mutitsi_dtv_term(uint32_t test_type,EN_FPP_DEMOD_DELIVERY_TYPE_T en_delivery_type)
{
    fpi_error err = FPI_ERROR_SUCCESS; 

    err = fpp_decoder_close(play_info.un32_video_decoder_id);
    err |= fpp_decoder_close(play_info.un32_audio_decoder_id);
    if(play_info.pst_demux_info != NULL)
    {
        free(play_info.pst_demux_info);
    }
    if(play_info.pst_decoder_info != NULL)
    {
        free(play_info.pst_decoder_info);
    }
    if(play_info.pst_window_info != NULL)
    {
        free(play_info.pst_window_info);
    }
    err = fpp_linein_close(EN_FPP_LINEIN_DTV);
	TF_CU_ASSERT_EQUAL(err,FPI_ERROR_SUCCESS,"fpp_linein_close","failed");
	
    return err;
}





static fpi_error  _mutitsi_dtv_play(uint32_t test_type,EN_FPP_DEMOD_DELIVERY_TYPE_T en_delivery_type)
{
    fpi_error err = FPI_ERROR_SUCCESS; 
    //err = fpp_demux_set_source_type(play_info.un32_memory_demux_id, EN_FPP_DTV_TS_PATH_0,EN_FPP_DEMUX_SOURCE_TYPE_MEMORY);
    if(play_info.un16_video_pid > 0 && play_info.un16_video_pid < 0x1fff)
    {
    	if(test_type == TSI_TEST_TYPE_PLAYBACK)
    	{
        	err |= fpp_decoder_set_source_type(play_info.un32_video_decoder_id ,EN_DECODE_SOURCE_TYPE_MEMORY_TS,&play_info.un32_memory_demux_id);
    	}
	else
	{
		err |=  fpp_decoder_set_source_type(play_info.un32_video_decoder_id ,EN_DECODE_SOURCE_TYPE_DEMUX,&play_info.un32_memory_demux_id);
	}
        err |= fpp_decoder_registe_callback(play_info.un32_video_decoder_id,_mutitsi_fpp_decoder_notify_callback);
        err |= fpp_demux_open_filter(play_info.un32_memory_demux_id,EN_FILTER_VIDEO_PID_TYPE,&play_info.un32_video_channel_id);
        err |= fpp_demux_set_filter(play_info.un32_memory_demux_id,play_info.un32_video_channel_id,play_info.un16_video_pid,NULL,fpi_true);
        err |= fpp_demux_pid_start(play_info.un32_memory_demux_id,play_info.un32_video_channel_id);
        err |= fpp_decoder_set_av_info(play_info.un32_video_decoder_id , &play_info.st_video_decoder_info);
        if(play_info.un16_pcr_pid > 0 && play_info.un16_pcr_pid < 0x1fff)
        {
            err |= fpp_demux_open_filter(play_info.un32_memory_demux_id,EN_FILTER_PCR_PID_TYPE,&play_info.un32_pcr_channel_id);
            err |= fpp_demux_set_filter(play_info.un32_memory_demux_id,play_info.un32_pcr_channel_id,play_info.un16_pcr_pid,NULL,fpi_true);
            err |= fpp_demux_pid_start(play_info.un32_memory_demux_id,play_info.un32_pcr_channel_id); 
            err |= fpp_decoder_set_sync_mode(play_info.un32_video_decoder_id,EN_SYNC_PCR);
        }
        else
        {
            err |= fpp_decoder_set_sync_mode(play_info.un32_video_decoder_id,EN_SYNC_VDEC);
        }

        err |= fpp_decoder_play(play_info.un32_video_decoder_id);
    }

    if(play_info.un16_audio_pid > 0 && play_info.un16_audio_pid < 0x1fff)
    {
    	if(test_type == TSI_TEST_TYPE_PLAYBACK)
    	{
        	err |= fpp_decoder_set_source_type(play_info.un32_audio_decoder_id ,EN_DECODE_SOURCE_TYPE_MEMORY_TS,&play_info.un32_memory_demux_id);
    	}
	else
	{
		err |= fpp_decoder_set_source_type(play_info.un32_audio_decoder_id ,EN_DECODE_SOURCE_TYPE_DEMUX,&play_info.un32_memory_demux_id);
	}
        
        err |= fpp_decoder_registe_callback(play_info.un32_audio_decoder_id,_mutitsi_fpp_decoder_notify_callback);
        err |= fpp_demux_open_filter(play_info.un32_memory_demux_id,EN_FILTER_AUDIO_PID_TYPE,&play_info.un32_audio_channel_id);
        err |= fpp_demux_set_filter(play_info.un32_memory_demux_id,play_info.un32_audio_channel_id,play_info.un16_audio_pid,NULL,fpi_true);
        err |= fpp_demux_pid_start(play_info.un32_memory_demux_id,play_info.un32_audio_channel_id);
        err |= fpp_decoder_set_av_info(play_info.un32_audio_decoder_id , &play_info.st_audio_decoder_info);
        err |= fpp_decoder_play(play_info.un32_audio_decoder_id);
    }

	
    return err;
}





static fpi_error  _mutitsi_dtv_stop(uint32_t test_type,EN_FPP_DEMOD_DELIVERY_TYPE_T en_delivery_type)
{
    fpi_error err = FPI_ERROR_SUCCESS; 
    if(play_info.un16_video_pid > 0 && play_info.un16_video_pid < 0x1fff)
    {
        err = fpp_decoder_stop(play_info.un32_video_decoder_id);
        err |= fpp_demux_pid_stop(play_info.un32_memory_demux_id,play_info.un32_video_channel_id);
        err |= fpp_demux_close_filter(play_info.un32_memory_demux_id,play_info.un32_video_channel_id);
        if(play_info.un16_pcr_pid > 0 && play_info.un16_pcr_pid < 0x1fff)
        {
            err |= fpp_demux_pid_stop(play_info.un32_memory_demux_id,play_info.un32_pcr_channel_id);
            err |= fpp_demux_close_filter(play_info.un32_memory_demux_id,play_info.un32_pcr_channel_id);
        }
    }

    if(play_info.un16_audio_pid > 0 && play_info.un16_audio_pid < 0x1fff)
    {
        err |= fpp_decoder_stop(play_info.un32_audio_decoder_id);
        err |= fpp_demux_pid_stop(play_info.un32_memory_demux_id,play_info.un32_audio_channel_id);
        err |= fpp_demux_close_filter(play_info.un32_memory_demux_id,play_info.un32_audio_channel_id);
    }
    return err;
}






static fpi_error  _mutitsi_dtv_display(void)
{
    fpi_error err = FPI_ERROR_SUCCESS; 
    EN_FPP_LINEIN_TYPE_T en_linein_to = EN_FPP_LINEIN_DTV;
    ST_FPP_SYSTEM_PANEL_PROPERTY_T stpanelinfo;
    
    
 err = fpp_zoom_connect_window_input(en_linein_to, play_info.un32_video_decoder_id, play_info.pst_window_info[0].un32_windowId);
 TF_CU_ASSERT_EQUAL(err,FPI_ERROR_SUCCESS,"fpp_zoom_connect_window_input","failed");

    

 err = fpp_system_get_panel_resolution(&stpanelinfo);
	TF_CU_ASSERT_EQUAL(err,FPI_ERROR_SUCCESS,"fpp_system_get_panel_resolution","failed");

err = fpp_signal_get_format(en_linein_to , &play_info.st_video_info);
TF_CU_ASSERT_EQUAL(err,FPI_ERROR_SUCCESS,"fpp_signal_get_format","failed");


	err = fpp_zoom_set_crop_window(play_info.pst_window_info[0].un32_windowId, 0, 0, 
									play_info.st_video_info.u32HResolution, play_info.st_video_info.u32VResolution);
	TF_CU_ASSERT_EQUAL(err,FPI_ERROR_SUCCESS,"fpp_zoom_set_crop_window","failed");


    if(play_info.st_panel_info.width == 3840 && play_info.st_panel_info.height == 2160)
    {
        play_info.st_panel_info.width = 1920;
        play_info.st_panel_info.height = 1080;
    }
   
 err = fpp_zoom_set_display_window(play_info.pst_window_info[0].un32_windowId, 0,0, play_info.st_panel_info.width, play_info.st_panel_info.height);
 TF_CU_ASSERT_EQUAL(err,FPI_ERROR_SUCCESS,"fpp_zoom_set_display_window","failed");

    
    

 err = fpp_video_mute(fpi_false,0);
TF_CU_ASSERT_EQUAL(err,FPI_ERROR_SUCCESS,"fpp_video_mute","failed");


    

    
    
    return err;
}





fpi_error  _mutitsi_record_write(T_TSI_CTRL * pst_tsi_ctr,uint32_t u32Size)
{
    uint32_t u32SizeLeft=u32Size;
    uint32_t u32ReadSize;
    uint8_t *pu8ReadPointer;
    uint32_t u32WrittenByte;
    
    //DBG_TSI_TEST("%s:%d Old read pointer is 0x%x, write size is %ld\n", __FUNCTION__, __LINE__, (int)(Tsi_ctrl_data.pu8TsiAddr+Tsi_ctrl_data.u32BufOffset),u32Size);
    if(NULL == pst_tsi_ctr->pTsiFileW)
        return FPI_ERROR_FAIL;

    while(u32SizeLeft)
    {
            CHECK_FUNC_RETURN(FPI_ERROR_SUCCESS, \
                fpp_tsi_get_read_buffer_address(pst_tsi_ctr->un32tsi_id,\
                                                 (uint32_t **)&pu8ReadPointer,&u32ReadSize));
            if(NULL == pu8ReadPointer)
                return FPI_ERROR_FAIL;
            u32ReadSize *= 4;
            if(u32ReadSize > u32SizeLeft)
                u32ReadSize = u32SizeLeft;
        
        //here write disk
        if(u32ReadSize)
        {
            u32WrittenByte = fwrite(pu8ReadPointer,sizeof(uint8_t),u32ReadSize,(FILE*)pst_tsi_ctr->pTsiFileW);
            if(u32WrittenByte != u32ReadSize)
            {
                TEST_LOG_ERROR("%s:%d : fwrite %d -> %d\n", __FUNCTION__, __LINE__, u32ReadSize, u32WrittenByte);
                return FPI_ERROR_FAIL;
            }
            printf("\n\r 0000:confirm_data tsid:%d\n\r",pst_tsi_ctr->un32tsi_id);
            CHECK_FUNC_RETURN(FPI_ERROR_SUCCESS, \
                fpp_tsi_confirm_data(pst_tsi_ctr->un32tsi_id, (uint32_t)pu8ReadPointer, u32ReadSize/4));
        }
        
        pst_tsi_ctr->u32BufOffset+=u32ReadSize;
        if(pst_tsi_ctr->u32BufOffset >= pst_tsi_ctr->u32TsiSize)
        {
            pst_tsi_ctr->u32BufOffset -= pst_tsi_ctr->u32TsiSize;
        }
        u32SizeLeft-=u32ReadSize;
    }

    //DBG_TSI_TEST("%s:%d : New read pointer is 0x%x!!\n", __FUNCTION__, __LINE__, (int)(Tsi_ctrl_data.pu8TsiAddr + Tsi_ctrl_data.u32BufOffset));
    
    return FPI_ERROR_SUCCESS;
}



fpi_error _mutitsi_record_init(uint8_t un8_record_id,ST_TEST_FPP_DTV_CHANNEL_PARAM_T * pst_record_param,fpi_bool b_encrypt,fpi_bool b_play)
{
 	uint8_t u8FreeFilter=0;
  	uint8_t u8PidCount=0,i;
  	uint16_t u16Pid[16];
	T_TSI_CTRL st_tsi_ctrl;
	ST_TEST_FPP_PROGRAME_INFO st_program_info;
	ST_FPP_DEMOD_DELIVERY_T st_delivery;
	ST_FPP_DEMUX_INFO_T *pDemuxInfo;

	ST_FPP_TSPATH_INFO_T *pDemodInfo = _mutitsi_get_demod_instance(pst_record_param->delivery_type);

  	memset(&st_tsi_ctrl,0,sizeof(T_TSI_CTRL));	
   	memset(&st_delivery,0,sizeof(ST_FPP_DEMOD_DELIVERY_T));

	
       
	st_delivery.delivery_type = pst_record_param->delivery_type;
	st_delivery.tspath = pDemodInfo->un32_tspath;
	if(st_delivery.delivery_type == EN_FPP_DEMOD_DELIVERY_TYPE_CABLE)
	{
		memcpy(&st_delivery.delivery.cable,&pst_record_param->unDeliveryInfo.cable,sizeof(ST_FPP_DEMOD_CABLE_T));
		st_delivery.delivery.frequency = pst_record_param->unDeliveryInfo.cable.frequency;
	}
	else
	{
		memcpy(&st_delivery.delivery.terrestrial,&pst_record_param->unDeliveryInfo.terrestrial,sizeof(ST_FPP_DEMOD_TERRESTRIAL_T));
		st_delivery.delivery.frequency = pst_record_param->unDeliveryInfo.terrestrial.frequency;
	}
	printf("\n\r0000:%s,%d ,delivery:%d\n\r",__FUNCTION__,__LINE__,st_delivery.delivery_type);

	_mutitsi_start_tuner(un8_record_id,st_delivery);

	if (_mutitsi_get_lock_state(un8_record_id,st_delivery) != fpi_true)
	{
	printf("\n\r0000:%s,%d unlock\n\r",__FUNCTION__,__LINE__);
		
		return FPI_ERROR_FAIL;
	}
	printf("\n\r0000:%s,%d lock\n\r",__FUNCTION__,__LINE__);
	
	if(un8_record_id == 0)
	{
		pDemuxInfo = _mutitsi_get_demux_instance(EN_FPP_DEMUX_SOURCE_TYPE_INNER_DEMOD_TS);
		_mutitsi_demux_set_source_type(pDemuxInfo,pDemodInfo->un32_tspath);
		st_tsi_ctrl.un32_demuxID = pDemuxInfo->un32_demuxID;
	}
	else
	{
		pDemuxInfo = _mutitsi_get_demux_instance(EN_FPP_DEMUX_SOURCE_TYPE_EXTERN_DEMOD_2);
		_mutitsi_demux_set_source_type(pDemuxInfo,pDemodInfo->un32_tspath);
		st_tsi_ctrl.un32_demuxID = pDemuxInfo->un32_demuxID;
	}
	if(b_play)
	{
		
		memset(&st_program_info,0,sizeof(ST_TEST_FPP_PROGRAME_INFO));

 		_mutitsi_dtv_get_program_info(pst_record_param->delivery_type, &st_program_info);
 		u16Pid[0]=(uint16_t)st_program_info.u32VideoPID;
  		u16Pid[1]=(uint16_t)st_program_info.u32AudioPID;
  		u16Pid[2]=(uint16_t)0;//pat pid
   		u16Pid[3]=(uint16_t)TSI_TEST_FILE_PMTPID;
   		u8PidCount=4;

   		printf("Pid number : %d.\n",u8PidCount);
		printf("Pid value:");
   		for(i=0; i<u8PidCount; i++)
   		{
   			printf(" %d ",u16Pid[i]);
  		}
 		printf("\n");

  		current_program_info.u16ServiceID = st_program_info.u16ServiceID;
  		current_program_info.u32VideoPID = st_program_info.u32VideoPID;
  		current_program_info.u32AudioPID = st_program_info.u32AudioPID;
		current_program_info.u32PCRPID = st_program_info.u32PCRPID;
 		current_program_info.u8VideoType = st_program_info.u8VideoType;
		current_program_info.u8AudioType = st_program_info.u8AudioType;

		CHECK_FUNC_RETURN(FPI_ERROR_SUCCESS, _mutitsi_dtv_init(TSI_TEST_TYPE_RECORD,pst_record_param->delivery_type));

		play_info.un32_memory_demux_id = pDemuxInfo->un32_demuxID;

   			printf("vid  %d \n\r",play_info.un16_video_pid);
   			printf("aid  %d ",play_info.un16_audio_pid);
			
   			printf(" demuxid %d ",play_info.un32_memory_demux_id);
	  	CHECK_FUNC_RETURN(FPI_ERROR_SUCCESS, _mutitsi_dtv_play(TSI_TEST_TYPE_RECORD,pst_record_param->delivery_type));
    		printf("dtv play OK!33 \n");
			printf("\n\r0000:%s,%d\n\r",__FUNCTION__,__LINE__);
		 VideoTurnOn();
	    //  AudioTurnOn(lineIn);	

		//return FPI_ERROR_SUCCESS;
	}
	else
	{
		 //channal 0 set pid number
  		memset(u16Pid,0,sizeof(u16Pid));

 		u16Pid[0]=(uint16_t)pst_record_param->stProgInfo.u32VideoPID;
  		u16Pid[1]=(uint16_t)pst_record_param->stProgInfo.u32AudioPID;
  		u16Pid[2]=(uint16_t)0;//pat pid
   		u16Pid[3]=(uint16_t)TSI_TEST_FILE_PMTPID;
   		u8PidCount=4;

		TEST_LOG_INFO("Pid number : %d.\n",u8PidCount);
		TEST_LOG_INFO("Pid value: ");
		for(i=0; i<u8PidCount; i++)
		{
    		TEST_LOG_INFO(" %d ",u16Pid[i]);
		}
		TEST_LOG_INFO("\n");

   		current_program_info.u16ServiceID = pst_record_param->stProgInfo.u16ServiceID;
  		current_program_info.u32VideoPID = pst_record_param->stProgInfo.u32VideoPID;
  		current_program_info.u32AudioPID = pst_record_param->stProgInfo.u32AudioPID;
		current_program_info.u32PCRPID = pst_record_param->stProgInfo.u32PCRPID;
  		current_program_info.u8VideoType = pst_record_param->stProgInfo.u8VideoType;
		current_program_info.u8AudioType = pst_record_param->stProgInfo.u8AudioType;
	}
#if 1
		  if(un8_record_id==1)
		   CHECK_FUNC_RETURN(FPI_ERROR_SUCCESS, fpp_tsi_init (st_tsi_ctrl.un32_demuxID, \
                          	EN_FPP_TSI_BUFFER_TS_OUTEx, &st_tsi_ctrl.un32tsi_id));
	      if(un8_record_id==0)
		   CHECK_FUNC_RETURN(FPI_ERROR_SUCCESS, fpp_tsi_init (st_tsi_ctrl.un32_demuxID, \
                          	EN_FPP_TSI_BUFFER_TS_OUT, &st_tsi_ctrl.un32tsi_id));
	#else
              	    
		   CHECK_FUNC_RETURN(FPI_ERROR_SUCCESS, fpp_tsi_init (st_tsi_ctrl.un32_demuxID, \
                          	EN_FPP_TSI_BUFFER_TS_OUT, &st_tsi_ctrl.un32tsi_id));   
	#endif
		printf("\n\r0000:%s,%d,demuxid:%d,TSIid:%d\n\r",__FUNCTION__,__LINE__,st_tsi_ctrl.un32_demuxID,st_tsi_ctrl.un32tsi_id);
  

	CHECK_FUNC_RETURN(FPI_ERROR_SUCCESS, 
       	fpp_tsi_get_read_buffer_address(st_tsi_ctrl.un32tsi_id,
                		(uint32_t **)(&st_tsi_ctrl.pu8TsiAddr),&st_tsi_ctrl.u32TsiSize));
		
  	st_tsi_ctrl.u32TsiSize=st_tsi_ctrl.u32TsiSize*4;
   	st_tsi_ctrl.u32BufOffset=0;

	CHECK_FUNC_RETURN(FPI_ERROR_SUCCESS, 
       	fpp_tsi_get_free_pids_filter_count(st_tsi_ctrl.un32tsi_id,&u8FreeFilter));

  	CHECK_FUNC_RETURN(FPI_ERROR_SUCCESS, 
   		fpp_tsi_set_record_filters(st_tsi_ctrl.un32tsi_id, u16Pid, u8PidCount));

  	TEST_LOG_INFO("fpp_tsi_set_record_filters OK! \n");

  	CHECK_FUNC_RETURN(FPI_ERROR_SUCCESS, \
   		fpp_tsi_hd_encrypt_enable_disable(st_tsi_ctrl.un32tsi_id, b_encrypt));

	TEST_LOG_INFO("fpp_tsi_start_stop start OK! \n");

 	CHECK_FUNC_RETURN(FPI_ERROR_SUCCESS, \
  		fpp_tsi_start_stop(st_tsi_ctrl.un32tsi_id, fpi_true));

	// Create USB file	
	if(FPI_ERROR_SUCCESS != _mutitsi_getusbpath(st_tsi_ctrl.file_path,sizeof(st_tsi_ctrl.file_path)-1))
    	{
        	TEST_LOG_INFO("GetUsbPath fail, now using default usb path... \n");
        	strncpy(st_tsi_ctrl.file_path, DEFAULT_MEDIA_PATH, sizeof(st_tsi_ctrl.file_path)-1);
   	}

   	if(un8_record_id == 0)
   	{
   		if(fpi_false == b_encrypt)
  		{
        	strcat(st_tsi_ctrl.file_path, "/test_tsi_clearstream.ts"); 
	    	}
	    	else
	    	{
	        	strcat(st_tsi_ctrl.file_path, "/test_tsi_encryption.ts");
	    	}
    
  		TEST_LOG_INFO("the tsi File path = %s\n", st_tsi_ctrl.file_path);
  		st_tsi_ctrl.pTsiFileW = (void*)fopen(st_tsi_ctrl.file_path, "wb");
		    
  		if(st_tsi_ctrl.pTsiFileW==NULL)
  		{
   			TEST_LOG_ERROR("Error: %s:%d creat and open file fail!\n", __FUNCTION__, __LINE__);
   			return FPI_ERROR_FAIL;
  		}

		memcpy(&Tsi_ctrl_data,&st_tsi_ctrl,sizeof(T_TSI_CTRL));
			printf("0000:Tsi_ctrl_data0.tsid:%d\n\r",Tsi_ctrl_data.un32tsi_id);
	
	}
	else if(un8_record_id == 1)
	{
		if(fpi_false == b_encrypt)
  		{
        	strcat(st_tsi_ctrl.file_path, "/test_tsi_clearstream1.ts"); 
	    	}
	    	else
	    	{
	        	strcat(st_tsi_ctrl.file_path, "/test_tsi_encryption1.ts");
	    	}
    
  		TEST_LOG_INFO("the tsi File path = %s\n", st_tsi_ctrl.file_path);
  		st_tsi_ctrl.pTsiFileW = (void*)fopen(st_tsi_ctrl.file_path, "wb");
		    
  		if(st_tsi_ctrl.pTsiFileW==NULL)
  		{
   			TEST_LOG_ERROR("Error: %s:%d creat and open file fail!\n", __FUNCTION__, __LINE__);
   			return FPI_ERROR_FAIL;
  		}

		memcpy(&Tsi_ctrl_data1,&st_tsi_ctrl,sizeof(T_TSI_CTRL));
		printf("0000:Tsi_ctrl_data1.tsid:%d\n\r",Tsi_ctrl_data1.un32tsi_id);
	}
	else
	{
		if(fpi_false == b_encrypt)
  		{
        	strcat(st_tsi_ctrl.file_path, "/test_tsi_clearstream2.ts"); 
    	}
    	else
    	{
        	strcat(st_tsi_ctrl.file_path, "/test_tsi_encryption2.ts");
    	}
    
  		TEST_LOG_INFO("the tsi File path = %s\n", st_tsi_ctrl.file_path);
  		st_tsi_ctrl.pTsiFileW = (void*)fopen(st_tsi_ctrl.file_path, "wb");
		    
  		if(st_tsi_ctrl.pTsiFileW==NULL)
  		{
   			TEST_LOG_ERROR("Error: %s:%d creat and open file fail!\n", __FUNCTION__, __LINE__);
   			return FPI_ERROR_FAIL;
  		}

		memcpy(&Tsi_ctrl_data2,&st_tsi_ctrl,sizeof(T_TSI_CTRL));
	}

	return FPI_ERROR_SUCCESS;
	

	
}

fpi_error _mutitsi_record_term(uint8_t un8_record_id,ST_TEST_FPP_DTV_CHANNEL_PARAM_T * pst_record_param,fpi_bool b_play)
{

	ST_FPP_DEMOD_DELIVERY_T st_delivery;
	memset(&st_delivery,0,sizeof(ST_FPP_DEMOD_DELIVERY_T));

	
       
	st_delivery.delivery_type = pst_record_param->delivery_type;
	st_delivery.tspath = EN_FPP_DTV_TS_PATH_0;
	if(st_delivery.delivery_type == EN_FPP_DEMOD_DELIVERY_TYPE_CABLE)
	{
		memcpy(&st_delivery.delivery.cable,&pst_record_param->unDeliveryInfo.cable,sizeof(ST_FPP_DEMOD_CABLE_T));
	}
	else
	{
		memcpy(&st_delivery.delivery.terrestrial,&pst_record_param->unDeliveryInfo.terrestrial,sizeof(ST_FPP_DEMOD_TERRESTRIAL_T));
	}
	if(b_play)
	{
   		CHECK_FUNC_RETURN(FPI_ERROR_SUCCESS, _mutitsi_dtv_stop(TSI_TEST_TYPE_RECORD,pst_record_param->delivery_type));
   
  		CHECK_FUNC_RETURN(FPI_ERROR_SUCCESS, _mutitsi_dtv_term(TSI_TEST_TYPE_RECORD,pst_record_param->delivery_type));
	}

	if(un8_record_id == 0)
	{
		CHECK_FUNC_RETURN(FPI_ERROR_SUCCESS, \
  			fpp_tsi_start_stop(Tsi_ctrl_data.un32tsi_id, fpi_false));
		
  		TEST_LOG_INFO("fpp_tsi_start_stop stop OK! \n");
		
  		CHECK_FUNC_RETURN(FPI_ERROR_SUCCESS, \
   			fpp_tsi_term(Tsi_ctrl_data.un32tsi_id));

   		TEST_LOG_INFO("fpp_tsi_term OK! \n");
		if(NULL != Tsi_ctrl_data.pTsiFileW)
  		{
  			fclose((FILE*)Tsi_ctrl_data.pTsiFileW);
  			Tsi_ctrl_data.pTsiFileW = NULL;
    		}
	}
	else if(un8_record_id == 1)
	{
		CHECK_FUNC_RETURN(FPI_ERROR_SUCCESS, \
  			fpp_tsi_start_stop(Tsi_ctrl_data1.un32tsi_id, fpi_false));
		CHECK_FUNC_RETURN(FPI_ERROR_SUCCESS, \
   			fpp_tsi_term(Tsi_ctrl_data1.un32tsi_id));

   		TEST_LOG_INFO("fpp_tsi_term OK! \n");
		if(NULL != Tsi_ctrl_data1.pTsiFileW)
  		{
  			fclose((FILE*)Tsi_ctrl_data1.pTsiFileW);
  			Tsi_ctrl_data1.pTsiFileW = NULL;
    		}
	}
	else
	{
		CHECK_FUNC_RETURN(FPI_ERROR_SUCCESS, \
  			fpp_tsi_start_stop(Tsi_ctrl_data2.un32tsi_id, fpi_false));
		CHECK_FUNC_RETURN(FPI_ERROR_SUCCESS, \
   			fpp_tsi_term(Tsi_ctrl_data2.un32tsi_id));

   		TEST_LOG_INFO("fpp_tsi_term OK! \n");
		if(NULL != Tsi_ctrl_data2.pTsiFileW)
  		{
  			fclose((FILE*)Tsi_ctrl_data2.pTsiFileW);
  			Tsi_ctrl_data2.pTsiFileW = NULL;
    		}
	}

	_mutitsi_stop_tuner(un8_record_id,st_delivery);
	return FPI_ERROR_SUCCESS;
}

fpi_error _mutitsi_playback_init(T_TSI_CTRL *pst_tsi_ctrl,EN_FPP_DEMOD_DELIVERY_TYPE_T en_delivery_type,fpi_bool b_encrypt,EN_FPP_TSI_PLAYBACK_MOTION e_play_mode)
{
	uint32_t u32MotionProp=0;

	ST_FPP_TSPATH_INFO_T *pDemodInfo = _mutitsi_get_demod_instance(en_delivery_type);
	printf("\n\r0000:%s,%d\n\r",__FUNCTION__,__LINE__);

	ST_FPP_DEMUX_INFO_T *pDemuxInfo  = _mutitsi_get_demux_instance(EN_FPP_DEMUX_SOURCE_TYPE_MEMORY);
		printf("\n\r0000:%s,%d\n\r",__FUNCTION__,__LINE__);

	if( NULL == pDemuxInfo)
	{
		return FPI_ERROR_FAIL;
	}
		printf("\n\r0000:%s,%d\n\r",__FUNCTION__,__LINE__);

    //  _mutitsi_demux_set_source_type(pDemuxInfo,pDemodInfo->un32_tspath);
       _mutitsi_demux_set_source_type(pDemuxInfo,0);
	pst_tsi_ctrl->un32_demuxID = pDemuxInfo->un32_demuxID;
			printf("\n\r0000:%s,%d\n\r",__FUNCTION__,__LINE__);

	CHECK_FUNC_RETURN(FPI_ERROR_SUCCESS, _mutitsi_dtv_init(TSI_TEST_TYPE_PLAYBACK,en_delivery_type));

	play_info.un32_memory_demux_id = pDemuxInfo->un32_demuxID;

	CHECK_FUNC_RETURN(FPI_ERROR_SUCCESS, _mutitsi_dtv_play(TSI_TEST_TYPE_PLAYBACK,en_delivery_type));
   	TEST_LOG_INFO(" play back OK22! \n");

	if(FPI_ERROR_SUCCESS != _mutitsi_getusbpath(pst_tsi_ctrl->file_path,sizeof(pst_tsi_ctrl->file_path)-1))
    	{
   		TEST_LOG_ERROR("GetUsbPath fail, now using default usb path... \n");
  		strncpy(pst_tsi_ctrl->file_path, DEFAULT_MEDIA_PATH, sizeof(pst_tsi_ctrl->file_path)-1);
  	}

 	strcat(pst_tsi_ctrl->file_path, "/test_tsi.ts");

	pst_tsi_ctrl->pTsiFileR = (void*)fopen(pst_tsi_ctrl->file_path, "rb");

    	if(pst_tsi_ctrl->pTsiFileR==NULL)
    	{
    		TEST_LOG_ERROR("Error: %s:%d open file fail!\n", __FUNCTION__, __LINE__);
  		return FPI_ERROR_FAIL;	
	}

	
	   
 	CHECK_FUNC_RETURN(FPI_ERROR_SUCCESS, \
   		fpp_tsi_init (pst_tsi_ctrl->un32_demuxID, \
                                EN_FPP_TSI_BUFFER_TS_IN, &pst_tsi_ctrl->un32tsi_id));

	TEST_LOG_INFO("fpp_tsi_init OK.\n");


	CHECK_FUNC_RETURN(FPI_ERROR_SUCCESS, \
   		fpp_tsi_hd_encrypt_enable_disable(pst_tsi_ctrl->un32tsi_id, b_encrypt));

 	//query playback hardware support ability
	CHECK_FUNC_RETURN(FPI_ERROR_SUCCESS, \
   		fpp_tsi_query_playback_motion_prop(pst_tsi_ctrl->un32tsi_id, &u32MotionProp));

	pst_tsi_ctrl->eMotion=e_play_mode;
   	TEST_LOG_INFO("Playback motion is EN_FPP_TSI_PB_FF_1X!\n");
	
  	//set playback motion, first time need to use 1x speed in order to get decoder callback event, and then change to actual motion.
 	CHECK_FUNC_RETURN(FPI_ERROR_SUCCESS, \
  		fpp_tsi_set_playback_motion(pst_tsi_ctrl->un32tsi_id, e_play_mode));

 	TEST_LOG_INFO("fpp_tsi_set_playback_motion %d OK!\n",pst_tsi_ctrl->eMotion);

	CHECK_FUNC_RETURN(FPI_ERROR_SUCCESS, \
   		fpp_tsi_start_stop(pst_tsi_ctrl->un32tsi_id, fpi_true));

	TEST_LOG_INFO("fpp_tsi_start_stop start OK! \n");

	return FPI_ERROR_SUCCESS;

}

static int get_char()

{
fd_set rfds;
struct timeval tv;
int ch = -1;
FD_ZERO(&rfds);
FD_SET(0, &rfds);
tv.tv_sec = 0;
tv.tv_usec = 0; 
//设置等待超时时间//检测键盘是否有输入
if (select(1, &rfds, NULL, NULL, &tv) > 0)
{
ch = getchar();
}
return ch;
}

fpi_error _mutitsi_playback_term(T_TSI_CTRL *pst_tsi_ctrl,EN_FPP_DEMOD_DELIVERY_TYPE_T en_delivery_type)
{
	CHECK_FUNC_RETURN(FPI_ERROR_SUCCESS, _mutitsi_dtv_stop(TSI_TEST_TYPE_PLAYBACK,en_delivery_type));
   
  	CHECK_FUNC_RETURN(FPI_ERROR_SUCCESS, _mutitsi_dtv_term(TSI_TEST_TYPE_PLAYBACK,en_delivery_type));

	CHECK_FUNC_RETURN(FPI_ERROR_SUCCESS, \
  		fpp_tsi_start_stop(pst_tsi_ctrl->un32tsi_id, fpi_false));

	CHECK_FUNC_RETURN(FPI_ERROR_SUCCESS, \
   		fpp_tsi_term(pst_tsi_ctrl->un32tsi_id));

   	TEST_LOG_INFO("fpp_tsi_term OK! \n");
	if(NULL != pst_tsi_ctrl->pTsiFileW)
  	{
  		fclose((FILE*)pst_tsi_ctrl->pTsiFileW);
  		pst_tsi_ctrl->pTsiFileW = NULL;
    	}
	
	return FPI_ERROR_SUCCESS;
}


fpi_error  _mutitsi_record_and_play_test(ST_TEST_FPP_DTV_CHANNEL_PARAM_T * pst_record_param,ST_TEST_FPP_DTV_CHANNEL_PARAM_T * pst_play_param,fpi_bool b_encrypt)
{
    uint32_t u32FeedDataThred=0;
    uint32_t u32Fullness=0;
    uint64_t u64TimeInSecsStart,u64TimeInSecsNow,u64TimeInSecsStart1;
    fpi_bool bWaitAVSync = fpi_true;
    fpi_bool bStartRec=fpi_false;
    uint32_t waitDisplayCnt = 0;
    fpi_bool b_record0_init_success = fpi_false; 
    fpi_bool b_record1_init_success = fpi_false;
    fpi_bool b_record0_start = fpi_false;
    fpi_bool b_record1_start = fpi_false;

 	if(_mutitsi_record_init(0,pst_play_param,b_encrypt,fpi_true) == FPI_ERROR_SUCCESS)
 	{
 		b_record0_init_success = fpi_true;
		b_record0_start = fpi_true;
 	}

 	if(_mutitsi_record_init(1,pst_record_param,b_encrypt,fpi_false) == FPI_ERROR_SUCCESS)
 	{
 		b_record1_init_success = fpi_true;
		b_record1_start = fpi_true;
 	}





	printf("\n\r 0000: play:%d,record:%d\n\r",b_record0_init_success,b_record1_init_success);
	
	if(b_record0_init_success == fpi_false && b_record1_init_success == fpi_false)
	{
		CHECK_FUNC_RETURN(FPI_ERROR_SUCCESS, _mutitsi_record_term(0,pst_play_param,fpi_true));
		CHECK_FUNC_RETURN(FPI_ERROR_SUCCESS, _mutitsi_record_term(1,pst_record_param,fpi_false)); 
		return FPI_ERROR_FAIL;
	}
    
    u32FeedDataThred = RECORD_WRITE_SIZE;//Tsi_ctrl_data.u32TsiSize / 4;
    u32FeedDataThred -= u32FeedDataThred%(188*2);

    CHECK_FUNC_RETURN(FPI_ERROR_SUCCESS, \
    	common_power_rtc_get_clk(&u64TimeInSecsStart1));
    u64TimeInSecsNow= u64TimeInSecsStart1;
        

    //get data
    while(1)
    {

    	if(get_char()=='p')
    	{
		_mutitsi_dtv_stop(0,0);
		return FPI_ERROR_SUCCESS;

		}

    	if(b_record0_start)
    	{
        	if(fpi_true == bWaitAVSync)
        	{
            		if(FPI_ERROR_SUCCESS != _mutitsi_dtv_display())
            		{
                		if(TSI_WAITDISPLAY_CNT_MAX <= waitDisplayCnt++)
                		{
                    			TEST_LOG_ERROR("[Error]: quit tsi record process because of waiting display timeout!\n");
                    			TF_CU_FAIL( "_tsi_dtv_display()","_tsi_dtv_display failure");
					b_record0_start = fpi_false;
                		}
                		usleep(TSI_WAITDISPLAY_TIME_SLEEP *1000);//10ms
                		TEST_LOG_INFO("tsi: waiting for dtv signal locked time: %d ms\n", waitDisplayCnt*TSI_WAITDISPLAY_TIME_SLEEP);
                		continue;
            		}
            		else
            		{
                		waitDisplayCnt = 0;
                		bWaitAVSync = fpi_false;
                		bStartRec = fpi_true;
		  		Tsi_ctrl_data.b_record_ts = fpi_true;
				CHECK_FUNC_RETURN(FPI_ERROR_SUCCESS, \
       				common_power_rtc_get_clk(&u64TimeInSecsStart));
   				u64TimeInSecsNow= u64TimeInSecsStart;
            		}
        	}
        
        	//-------------------------------------------------------------
        	CHECK_FUNC_RETURN(FPI_ERROR_SUCCESS, \
        	fpp_tsi_get_read_buffer_fullness(Tsi_ctrl_data.un32tsi_id, &u32Fullness));

        	//DBG_TSI_TEST("fpp_tsi_get_read_buffer_fullness is %d! \n",u32Fullness*4);


        	if(u32Fullness==0xffffffff)
        	{
         		//buffer overflow, reset
     			TEST_LOG_INFO("tsi buffer overflow,  start to reset buffer fullness! \n");
            
     			CHECK_FUNC_RETURN(FPI_ERROR_SUCCESS, \
 					fpp_tsi_reset_buffer_fullness(Tsi_ctrl_data.un32tsi_id));

      			Tsi_ctrl_data.u32BufOffset=0;
  				TEST_LOG_INFO("fpp_tsi_reset_buffer_fullness OK! \n");
        	}
        	else if(u32Fullness*4>=/*RECORD_WRITE_SIZE*/u32FeedDataThred)
        	{
 				DBG_TSI_TEST("fpp_tsi_get_read_buffer_fullness is %d! \n",u32Fullness*4);
     			//if buffer data size is enough, write disk, then change buffer fullness
      			if(FPI_ERROR_SUCCESS != _mutitsi_record_write(&Tsi_ctrl_data,/*RECORD_WRITE_SIZE*/u32FeedDataThred))
      			{
       				TEST_LOG_ERROR("%s:%d fail\n", __FUNCTION__, __LINE__);
       				b_record0_start = fpi_false;;
 				}

    		}
			else
			{
	 			//DBG_TSI_TEST("\n%s:%d buffer data is not enoughl, delay some time!!\n", __FUNCTION__, __LINE__);
		 		//delay 10ms for loading data
       			usleep(10*1000);
	 		}

        	//delay 10ms for loading data
        	//usleep(10*1000);

        	if(fpi_true == bStartRec)
        	{
      			CHECK_FUNC_RETURN(FPI_ERROR_SUCCESS, \
      				common_power_rtc_get_clk(&u64TimeInSecsNow));

   				//end recording
   				if((u64TimeInSecsNow - u64TimeInSecsStart) >= TSI_TEST_FILE_RECORD_LENGTH_IN_SECONDS)
       			{

  					do{
   						CHECK_FUNC_RETURN(FPI_ERROR_SUCCESS, \
   							fpp_tsi_get_read_buffer_fullness(Tsi_ctrl_data.un32tsi_id, &u32Fullness));

       					u32Fullness *= 4;
       					u32Fullness -= u32Fullness%(188*2);
       					if(FPI_ERROR_SUCCESS != _mutitsi_record_write(&Tsi_ctrl_data,u32Fullness))
       					{
   							TEST_LOG_ERROR("%s:%d fail\n", __FUNCTION__, __LINE__);
  							break;
                 				}
   						usleep(10*1000);
  					}while(u32Fullness);
                
   					b_record0_start = fpi_false;
 				}
			}
  		}

		if(b_record1_start)
		{
			CHECK_FUNC_RETURN(FPI_ERROR_SUCCESS, \
   				fpp_tsi_get_read_buffer_fullness(Tsi_ctrl_data1.un32tsi_id, &u32Fullness));

   			if(u32Fullness==0xffffffff)
   			{
  				//buffer overflow, reset
   				TEST_LOG_INFO("tsi buffer overflow,  start to reset buffer fullness! \n");
            
   				CHECK_FUNC_RETURN(FPI_ERROR_SUCCESS, \
 					fpp_tsi_reset_buffer_fullness(Tsi_ctrl_data1.un32tsi_id));

   				Tsi_ctrl_data1.u32BufOffset=0;
  				TEST_LOG_INFO("fpp_tsi_reset_buffer_fullness OK! \n");
   			}
   			else if(u32Fullness*4>=/*RECORD_WRITE_SIZE*/u32FeedDataThred)
   			{
				DBG_TSI_TEST("fpp_tsi_get_read_buffer_fullness is %d! \n",u32Fullness*4);
   				//if buffer data size is enough, write disk, then change buffer fullness
  				if(FPI_ERROR_SUCCESS != _mutitsi_record_write(&Tsi_ctrl_data1,/*RECORD_WRITE_SIZE*/u32FeedDataThred))
   				{
   					TEST_LOG_ERROR("%s:%d fail\n", __FUNCTION__, __LINE__);
       				b_record1_start = fpi_false;
 				}

  			}
			else
			{
	 			//DBG_TSI_TEST("\n%s:%d buffer data is not enoughl, delay some time!!\n", __FUNCTION__, __LINE__);
		 		//delay 10ms for loading data
       			usleep(10*1000);
	 		}

			CHECK_FUNC_RETURN(FPI_ERROR_SUCCESS, \
    				common_power_rtc_get_clk(&u64TimeInSecsNow));

   			//end recording
   			if((u64TimeInSecsNow - u64TimeInSecsStart1) >= TSI_TEST_FILE_RECORD_LENGTH_IN_SECONDS)
       		{
       			
  				do{
   					CHECK_FUNC_RETURN(FPI_ERROR_SUCCESS, \
   						fpp_tsi_get_read_buffer_fullness(Tsi_ctrl_data1.un32tsi_id, &u32Fullness));

       				u32Fullness *= 4;
       				u32Fullness -= u32Fullness%(188*2);
       				if(FPI_ERROR_SUCCESS != _mutitsi_record_write(&Tsi_ctrl_data1,u32Fullness))
       				{
   						TEST_LOG_ERROR("%s:%d fail\n", __FUNCTION__, __LINE__);
  						break;
                 			}
   					usleep(10*1000);
  				}while(u32Fullness);
                
   				b_record1_start = fpi_false;
 			}
		}

		if(b_record0_start == fpi_false && b_record1_start == fpi_false)
		{
			break;
		}
	}

	if(b_record0_init_success)
	{
		CHECK_FUNC_RETURN(FPI_ERROR_SUCCESS, _mutitsi_record_term(0,pst_play_param,fpi_true));       
           
	}
	if(b_record1_init_success)
	{
		CHECK_FUNC_RETURN(FPI_ERROR_SUCCESS, _mutitsi_record_term(0,pst_record_param,fpi_false));  
	}
	return FPI_ERROR_SUCCESS;
}

fpi_error  _mutitsi_record_and_playback_test(ST_TEST_FPP_DTV_CHANNEL_PARAM_T * pst_record_param,ST_TEST_FPP_DTV_CHANNEL_PARAM_T * pst_record_param1,EN_FPP_DEMOD_DELIVERY_TYPE_T playback_delivery_type,fpi_bool b_encrypt,EN_FPP_TSI_PLAYBACK_MOTION e_play_mode)
{
  	uint32_t u32FeedDataThred=0;
   	uint32_t u32PlaybackFullness=0;
  	uint32_t u32FreeSpace=0;
  	fpi_bool bWaitAVSync = fpi_true;
  	fpi_bool bFileEnd = fpi_false;
  	uint64_t u64TimeInSecsStart,u64TimeInSecsLast,u64TimeInSecsNow,u64TimeInSecsStart1,u64TimeInSecsStart2;
   	uint32_t waitDisplayCnt = 0;

	fpi_bool b_record_init_success = fpi_false;
	fpi_bool b_record_init_success1 = fpi_false;
	fpi_bool b_playback_init_success = fpi_false;
	fpi_bool b_record_start = fpi_false;
	fpi_bool b_record_start1 = fpi_false;
	fpi_bool b_playback_start = fpi_false;
	uint32_t u32Fullness=0;


  	printf("\n\r0000:start test5 %s,%d\n\r",__FUNCTION__,__LINE__);

	if(_mutitsi_playback_init(&Tsi_ctrl_data2,playback_delivery_type,fpi_false,e_play_mode) == FPI_ERROR_SUCCESS)
  	{
  		printf("\n\r0000:playback OK %s,%d\n\r",__FUNCTION__,__LINE__);

   		b_playback_init_success = fpi_true;
		b_playback_start = fpi_true;	
  	}
	
#if 1
	else
	{
		_mutitsi_playback_term(&Tsi_ctrl_data2,playback_delivery_type);
	}

		if(_mutitsi_record_init(0,pst_record_param,b_encrypt,fpi_false) == FPI_ERROR_SUCCESS)
	{
		b_record_init_success = fpi_true;
		b_record_start = fpi_true;
	}
	else
	{
		CHECK_FUNC_RETURN(FPI_ERROR_SUCCESS, _mutitsi_record_term(0,pst_record_param,fpi_false));
	}
	if(_mutitsi_record_init(1,pst_record_param1,b_encrypt,fpi_false) == FPI_ERROR_SUCCESS)
	{
		b_record_init_success1 = fpi_true;
		b_record_start1 = fpi_true;
	}
	else
	{
		CHECK_FUNC_RETURN(FPI_ERROR_SUCCESS, _mutitsi_record_term(1,pst_record_param1,fpi_false));
	}


#endif
       if(b_playback_init_success == fpi_false && b_record_init_success == fpi_false && b_record_init_success1 == fpi_false)
       {
       	return FPI_ERROR_FAIL;
       }

	CHECK_FUNC_RETURN(FPI_ERROR_SUCCESS, \
   		common_power_rtc_get_clk(&u64TimeInSecsStart1));
    	u64TimeInSecsLast = u64TimeInSecsStart;
	u64TimeInSecsStart = u64TimeInSecsStart1;
	u64TimeInSecsStart2 = u64TimeInSecsStart1;

	u32FeedDataThred = PLAYBACK_READ_SIZE;//Tsi_ctrl_data.u32TsdSize / 2;
    	u32FeedDataThred -= u32FeedDataThred%(188*2);
  	while(1)
  	{
   		if(b_playback_start)
  		{
   			if(fpi_false == bFileEnd)
  			{
   				CHECK_FUNC_RETURN(FPI_ERROR_SUCCESS, \
   				fpp_tsi_get_write_buffer_free_space(Tsi_ctrl_data2.un32tsi_id, &u32FreeSpace));
            
  				DBG_TSI_TEST("fpp_tsi_get_write_buffer_free_space is %d! \n",u32FreeSpace*4);

   				if((/*Tsi_ctrl_data.u32TsdSize-*/4*u32FreeSpace) >= /*PLAYBACK_READ_SIZE*/u32FeedDataThred)
   				{
   					//if buffer data size is enough, read disk, then change buffer fullness
  					if(FPI_ERROR_SUCCESS != _mutitsi_playback_virtual_disk_read(/*PLAYBACK_READ_SIZE*/u32FeedDataThred))
  					{
        					TEST_LOG_INFO("%s:%d file is empty\n", __FUNCTION__, __LINE__);
   						bFileEnd = fpi_true;
   					}

  				}
  				else
   				{
   					DBG_TSI_TEST("%s:%d buffer near full, delay some time!!\n", __FUNCTION__, __LINE__);
  				}
 			}
  			else
			{
   				static int32_t count=0;
   				uint32_t u32PlaybackFullness_cur=0;
   				CHECK_FUNC_RETURN(FPI_ERROR_SUCCESS, \
   					fpp_tsi_get_playback_buffer_fullness(Tsi_ctrl_data2.un32tsi_id, &u32PlaybackFullness_cur));
   				DBG_TSI_TEST("TSI: fpp_tsi_get_playback_buffer_fullness=%d, last fullness=%d\n", u32PlaybackFullness_cur, u32PlaybackFullness);

   				if(u32PlaybackFullness != u32PlaybackFullness_cur)
   				{
   					count = 0;
   				}
   				else
   				{
   					count++;
   					if(count >= 100)
   					{
  						TEST_LOG_INFO("TSI playback buffer is empty, check count=%d, quit...\n", count);

   						usleep(200*1000);
						b_playback_start = fpi_false;
						CHECK_FUNC_RETURN(FPI_ERROR_SUCCESS, \
        						common_power_rtc_get_clk(&u64TimeInSecsLast));
  					}
                
  				}
				if(u32PlaybackFullness_cur > u32PlaybackFullness)
				{
					TF_CU_FAIL("fpp_tsi_get_playback_buffer_fullness", "fpp_tsi_get_playback_buffer_fullness failure!");
				}
   				u32PlaybackFullness = u32PlaybackFullness_cur;
   			}

   			if(fpi_true == bWaitAVSync)
   			{
   				//enable AV display
   				if(FPI_ERROR_SUCCESS != _mutitsi_dtv_display())
   				{
   					if(TSI_WAITDISPLAY_CNT_MAX <= waitDisplayCnt++)
   					{
   						b_playback_start = fpi_false;// quit...
   						CHECK_FUNC_RETURN(FPI_ERROR_SUCCESS, \
        						common_power_rtc_get_clk(&u64TimeInSecsLast));
   					}
   					//usleep(TSI_WAITDISPLAY_TIME_SLEEP *1000);//10ms
   				}
   				else
   				{
   					waitDisplayCnt = 0;
   					bWaitAVSync = fpi_false;

   					CHECK_FUNC_RETURN(FPI_ERROR_SUCCESS, \
   						common_power_rtc_get_clk(&u64TimeInSecsStart));
   					u64TimeInSecsLast = u64TimeInSecsStart;

                    
 				}
   			}

  			usleep(10*1000);
   		}

  		if(b_record_start)
		{

			CHECK_FUNC_RETURN(FPI_ERROR_SUCCESS, \
        			fpp_tsi_get_read_buffer_fullness(Tsi_ctrl_data.un32tsi_id, &u32Fullness));

   			if(u32Fullness==0xffffffff)
   			{
  				//buffer overflow, reset
   				TEST_LOG_INFO("tsi buffer overflow,  start to reset buffer fullness! \n");
            
   				CHECK_FUNC_RETURN(FPI_ERROR_SUCCESS, \
 					fpp_tsi_reset_buffer_fullness(Tsi_ctrl_data.un32tsi_id));

   				Tsi_ctrl_data.u32BufOffset=0;
  				TEST_LOG_INFO("fpp_tsi_reset_buffer_fullness OK! \n");
   			}
   			else if(u32Fullness*4>=/*RECORD_WRITE_SIZE*/u32FeedDataThred)
   			{
				DBG_TSI_TEST("fpp_tsi_get_read_buffer_fullness is %d! \n",u32Fullness*4);
   				//if buffer data size is enough, write disk, then change buffer fullness
  				if(FPI_ERROR_SUCCESS != _mutitsi_record_write(&Tsi_ctrl_data,/*RECORD_WRITE_SIZE*/u32FeedDataThred))
   				{
   					TEST_LOG_ERROR("%s:%d fail\n", __FUNCTION__, __LINE__);
       				b_record_start = fpi_false;
 				}

  			}
			else
			{
	 			DBG_TSI_TEST("\n%s:%d buffer data is not enoughl, delay some time!!\n", __FUNCTION__, __LINE__);
		 		//delay 10ms for loading data
       			usleep(10*1000);
	 		}

			CHECK_FUNC_RETURN(FPI_ERROR_SUCCESS, \
    				common_power_rtc_get_clk(&u64TimeInSecsNow));

   			//end recording
   			if((u64TimeInSecsNow - u64TimeInSecsStart1) >= TSI_TEST_FILE_RECORD_LENGTH_IN_SECONDS)
       		{
       			
  				do{
   					CHECK_FUNC_RETURN(FPI_ERROR_SUCCESS, \
   						fpp_tsi_get_read_buffer_fullness(Tsi_ctrl_data.un32tsi_id, &u32Fullness));

       				u32Fullness *= 4;
       				u32Fullness -= u32Fullness%(188*2);
       				if(FPI_ERROR_SUCCESS != _mutitsi_record_write(&Tsi_ctrl_data,u32Fullness))
       				{
   						TEST_LOG_ERROR("%s:%d fail\n", __FUNCTION__, __LINE__);
  						break;
                 			}
   					usleep(10*1000);
  				}while(u32Fullness);
                
   				b_record_start = fpi_false;
 			}
		}

		if(b_record_start1)
		{

			CHECK_FUNC_RETURN(FPI_ERROR_SUCCESS, \
        			fpp_tsi_get_read_buffer_fullness(Tsi_ctrl_data1.un32tsi_id, &u32Fullness));

   			if(u32Fullness==0xffffffff)
   			{
  				//buffer overflow, reset
   				TEST_LOG_INFO("tsi buffer overflow,  start to reset buffer fullness! \n");
            
   				CHECK_FUNC_RETURN(FPI_ERROR_SUCCESS, \
 					fpp_tsi_reset_buffer_fullness(Tsi_ctrl_data1.un32tsi_id));

   				Tsi_ctrl_data2.u32BufOffset=0;
  				TEST_LOG_INFO("fpp_tsi_reset_buffer_fullness OK! \n");
   			}
   			else if(u32Fullness*4>=/*RECORD_WRITE_SIZE*/u32FeedDataThred)
   			{
				DBG_TSI_TEST("fpp_tsi_get_read_buffer_fullness is %d! \n",u32Fullness*4);
   				//if buffer data size is enough, write disk, then change buffer fullness
  				if(FPI_ERROR_SUCCESS != _mutitsi_record_write(&Tsi_ctrl_data1,/*RECORD_WRITE_SIZE*/u32FeedDataThred))
   				{
   					TEST_LOG_ERROR("%s:%d fail\n", __FUNCTION__, __LINE__);
       				b_record_start1 = fpi_false;
 				}

  			}
			else
			{
	 			DBG_TSI_TEST("\n%s:%d buffer data is not enoughl, delay some time!!\n", __FUNCTION__, __LINE__);
		 		//delay 10ms for loading data
       			usleep(10*1000);
	 		}

			CHECK_FUNC_RETURN(FPI_ERROR_SUCCESS, \
    				common_power_rtc_get_clk(&u64TimeInSecsNow));

   			//end recording
   			if((u64TimeInSecsNow - u64TimeInSecsStart2) >= TSI_TEST_FILE_RECORD_LENGTH_IN_SECONDS)
       		{
       			
  				do{
   					CHECK_FUNC_RETURN(FPI_ERROR_SUCCESS, \
   						fpp_tsi_get_read_buffer_fullness(Tsi_ctrl_data1.un32tsi_id, &u32Fullness));

       				u32Fullness *= 4;
       				u32Fullness -= u32Fullness%(188*2);
       				if(FPI_ERROR_SUCCESS != _mutitsi_record_write(&Tsi_ctrl_data1,u32Fullness))
       				{
   						TEST_LOG_ERROR("%s:%d fail\n", __FUNCTION__, __LINE__);
  						break;
                 			}
   					usleep(10*1000);
  				}while(u32Fullness);
                
   				b_record_start1 = fpi_false;
 			}
		}

		if(b_playback_start == fpi_false && b_record_start == fpi_false && b_record_start1 == fpi_false)
		{
			break;
		}
    	}

   	if(b_playback_init_success)
	{
		CHECK_FUNC_RETURN(FPI_ERROR_SUCCESS, _mutitsi_playback_term(&Tsi_ctrl_data2,playback_delivery_type));       
           
	}
	if(b_record_init_success)
	{
		CHECK_FUNC_RETURN(FPI_ERROR_SUCCESS, _mutitsi_record_term(0,pst_record_param,fpi_false));  
	}

	if(b_record_init_success1)
	{
		CHECK_FUNC_RETURN(FPI_ERROR_SUCCESS, _mutitsi_record_term(1,pst_record_param,fpi_false));  
	}

	// check the 1x playback duration
  	if(b_playback_init_success)
  	{
  		int32_t difftime = (int32_t)(u64TimeInSecsLast - u64TimeInSecsStart);
   		int32_t tolerance = 3;// seconds

   		TEST_LOG_INFO("TSI Playback duration = %d s, the requirement = %d s\n", difftime, Tsi_ctrl_data2.duration);
        
   		if(difftime >= (Tsi_ctrl_data2.duration + tolerance)/*second*/)
   		{
   			TF_CU_FAIL("_tsi_playback_test", "the duration of 1x playback is more than TSI_TEST_FILE_RECORD_LENGTH_IN_SECONDS, failed!");
   		}
   		else if(difftime <= (Tsi_ctrl_data2.duration - tolerance)/*second*/)
  		{
   			TF_CU_FAIL("_tsi_playback_test", "the duration of 1x playback is less than TSI_TEST_FILE_RECORD_LENGTH_IN_SECONDS, failed!");
  		}
   		else
   		{
   			
  		}
  	}

  	return FPI_ERROR_SUCCESS;
}



void  test_tsi_Record_DVBT_and_Play_DVBC(void)
{
	ST_TEST_FPP_DTV_CHANNEL_PARAM_T st_record_param;
	ST_TEST_FPP_DTV_CHANNEL_PARAM_T st_play_param;


	memset(&st_record_param,0,sizeof(ST_TEST_FPP_DTV_CHANNEL_PARAM_T));
	st_record_param.delivery_type = EN_FPP_DEMOD_DELIVERY_TYPE_TERRESTRIAL;
	st_record_param.unDeliveryInfo.terrestrial.frequency = T_FRE;
	st_record_param.unDeliveryInfo.terrestrial.bandwidth = EN_FPP_DEMOD_BAND_8M;
	st_record_param.stProgInfo.u32VideoPID = T_VID;//101;
	st_record_param.stProgInfo.u32AudioPID = T_AID;//102;   //audio pid:106 audio type :EN_ES_AUDIO_AAC
	st_record_param.stProgInfo.u32PCRPID = T_PCRID;//101;
	st_record_param.stProgInfo.u8VideoType = T_VTYPE;
	st_record_param.stProgInfo.u8AudioType = T_ATYPE;

       memset(&st_play_param,0,sizeof(ST_TEST_FPP_DTV_CHANNEL_PARAM_T));
	st_play_param.delivery_type = EN_FPP_DEMOD_DELIVERY_TYPE_CABLE;
	st_play_param.unDeliveryInfo.cable.frequency = 259000;
	st_play_param.unDeliveryInfo.cable.modulation = EN_FPP_DEMOD_QAM64;
	st_play_param.unDeliveryInfo.cable.symbol = 6875;
	st_play_param.stProgInfo.u32VideoPID = 0x191;
	st_play_param.stProgInfo.u32AudioPID = 0x192;
	st_play_param.stProgInfo.u32PCRPID = 0x191;
	st_play_param.stProgInfo.u8VideoType = EN_ES_VIDEO_MPEG2VIDEO;
	st_play_param.stProgInfo.u8AudioType = EN_ES_AUDIO_MPEG;
	delivery_type = EN_FPP_DEMOD_DELIVERY_TYPE_CABLE;
	memset(&current_program_info,0,sizeof (ST_TEST_FPP_PROGRAME_INFO));
	printf("\n\r0000:2222%s,%d\n\r",__FUNCTION__,__LINE__);

	_mutitsi_record_and_play_test(&st_record_param,&st_play_param,fpi_false);
}



void  test_tsi_Record_DVBC_and_Play_DVBT(void)
{
	ST_TEST_FPP_DTV_CHANNEL_PARAM_T st_record_param;
	ST_TEST_FPP_DTV_CHANNEL_PARAM_T st_play_param;

	memset(&st_record_param,0,sizeof(ST_TEST_FPP_DTV_CHANNEL_PARAM_T));
	st_record_param.delivery_type = EN_FPP_DEMOD_DELIVERY_TYPE_CABLE;
	st_record_param.unDeliveryInfo.cable.frequency = 259000;
	st_record_param.unDeliveryInfo.cable.modulation = EN_FPP_DEMOD_QAM64;
	st_record_param.unDeliveryInfo.cable.symbol = 6875;
	st_record_param.stProgInfo.u32VideoPID = 0x191;
	st_record_param.stProgInfo.u32AudioPID = 0x192;
	st_record_param.stProgInfo.u32PCRPID = 0x191;
	st_record_param.stProgInfo.u8VideoType = EN_ES_VIDEO_MPEG2VIDEO;
	st_record_param.stProgInfo.u8AudioType = EN_ES_AUDIO_MPEG;

	memset(&st_play_param,0,sizeof(ST_TEST_FPP_DTV_CHANNEL_PARAM_T));
	st_play_param.delivery_type = EN_FPP_DEMOD_DELIVERY_TYPE_TERRESTRIAL;
	st_play_param.unDeliveryInfo.terrestrial.frequency = T_FRE;
	st_play_param.unDeliveryInfo.terrestrial.bandwidth = EN_FPP_DEMOD_BAND_8M;
	st_play_param.stProgInfo.u32VideoPID = T_VID;//101;
	st_play_param.stProgInfo.u32AudioPID = T_AID;//102;   //audio pid:106 audio type :EN_ES_AUDIO_AAC
	st_play_param.stProgInfo.u32PCRPID = T_PCRID;//101;
	st_play_param.stProgInfo.u8VideoType = T_VTYPE;
	st_play_param.stProgInfo.u8AudioType = T_ATYPE;

	delivery_type = EN_FPP_DEMOD_DELIVERY_TYPE_TERRESTRIAL;
	memset(&current_program_info,0,sizeof(ST_TEST_FPP_PROGRAME_INFO));
	
	_mutitsi_record_and_play_test(&st_record_param,&st_play_param,fpi_false);
}

void  test_tsi_Record_DVBC_and_Play_DVBC(void)
{
	ST_TEST_FPP_DTV_CHANNEL_PARAM_T st_record_param;
	ST_TEST_FPP_DTV_CHANNEL_PARAM_T st_play_param;

	memset(&st_record_param,0,sizeof(ST_TEST_FPP_DTV_CHANNEL_PARAM_T));
	st_record_param.delivery_type = EN_FPP_DEMOD_DELIVERY_TYPE_CABLE;
	st_record_param.unDeliveryInfo.cable.frequency = 259000;
	st_record_param.unDeliveryInfo.cable.modulation = EN_FPP_DEMOD_QAM64;
	st_record_param.unDeliveryInfo.cable.symbol = 6875;
	st_record_param.stProgInfo.u32VideoPID = 0x191;
	st_record_param.stProgInfo.u32AudioPID = 0x192;
	st_record_param.stProgInfo.u32PCRPID = 0x191;
	st_record_param.stProgInfo.u8VideoType = EN_ES_VIDEO_MPEG2VIDEO;
	st_record_param.stProgInfo.u8AudioType = EN_ES_AUDIO_MPEG;

	memset(&st_play_param,0,sizeof(ST_TEST_FPP_DTV_CHANNEL_PARAM_T));
	st_play_param.delivery_type = EN_FPP_DEMOD_DELIVERY_TYPE_CABLE;
	st_play_param.unDeliveryInfo.cable.frequency = 259000;
	st_play_param.unDeliveryInfo.cable.modulation = EN_FPP_DEMOD_QAM64;
	st_play_param.unDeliveryInfo.cable.symbol = 6875;
	st_play_param.stProgInfo.u32VideoPID = 0x191;
	st_play_param.stProgInfo.u32AudioPID = 0x192;
	st_play_param.stProgInfo.u32PCRPID = 0x191;
	st_play_param.stProgInfo.u8VideoType = EN_ES_VIDEO_MPEG2VIDEO;
	st_play_param.stProgInfo.u8AudioType = EN_ES_AUDIO_MPEG;
	
	delivery_type = EN_FPP_DEMOD_DELIVERY_TYPE_CABLE;
	memset(&current_program_info,0,sizeof(ST_TEST_FPP_PROGRAME_INFO));
	_mutitsi_record_and_play_test(&st_record_param,&st_play_param,fpi_false);
}

void  test_tsi_Record_DVBT_and_Play_DVBT(void)
{
	ST_TEST_FPP_DTV_CHANNEL_PARAM_T st_record_param;
	ST_TEST_FPP_DTV_CHANNEL_PARAM_T st_play_param;

	memset(&st_record_param,0,sizeof(ST_TEST_FPP_DTV_CHANNEL_PARAM_T));
	st_record_param.delivery_type = EN_FPP_DEMOD_DELIVERY_TYPE_TERRESTRIAL;
	st_record_param.unDeliveryInfo.terrestrial.frequency = T_FRE;
	st_record_param.unDeliveryInfo.terrestrial.bandwidth = EN_FPP_DEMOD_BAND_8M;
	st_record_param.stProgInfo.u32VideoPID = T_VID;
	st_record_param.stProgInfo.u32AudioPID = T_AID;   //audio pid:106 audio type :EN_ES_AUDIO_AAC
	st_record_param.stProgInfo.u32PCRPID = T_PCRID;
	st_record_param.stProgInfo.u8VideoType = T_VTYPE;
	st_record_param.stProgInfo.u8AudioType = T_ATYPE;

	memset(&st_play_param,0,sizeof(ST_TEST_FPP_DTV_CHANNEL_PARAM_T));
	st_play_param.delivery_type = EN_FPP_DEMOD_DELIVERY_TYPE_TERRESTRIAL;
	st_play_param.unDeliveryInfo.terrestrial.frequency = T_FRE;
	st_play_param.unDeliveryInfo.terrestrial.bandwidth = EN_FPP_DEMOD_BAND_8M;
	st_play_param.stProgInfo.u32VideoPID = T_VID;
	st_play_param.stProgInfo.u32AudioPID = T_AID;   //audio pid:106 audio type :EN_ES_AUDIO_AAC
	st_play_param.stProgInfo.u32PCRPID = T_PCRID;
	st_play_param.stProgInfo.u8VideoType = T_VTYPE;
	st_play_param.stProgInfo.u8AudioType = T_ATYPE;
	delivery_type = EN_FPP_DEMOD_DELIVERY_TYPE_TERRESTRIAL;
	memset(&current_program_info,0,sizeof(ST_TEST_FPP_PROGRAME_INFO));
	_mutitsi_record_and_play_test(&st_record_param,&st_play_param,fpi_false);
}

void  test_tsi_Record_DVBT_DVBC_and_Playback_1x(void)
{
	ST_TEST_FPP_DTV_CHANNEL_PARAM_T st_record_param;
	ST_TEST_FPP_DTV_CHANNEL_PARAM_T st_record_param1;
	memset(&st_record_param,0,sizeof(ST_TEST_FPP_DTV_CHANNEL_PARAM_T));
	st_record_param.delivery_type = EN_FPP_DEMOD_DELIVERY_TYPE_TERRESTRIAL;
	st_record_param.unDeliveryInfo.terrestrial.frequency = T_FRE;
	st_record_param.unDeliveryInfo.terrestrial.bandwidth = EN_FPP_DEMOD_BAND_8M;
	st_record_param.stProgInfo.u32VideoPID = T_VID;
	st_record_param.stProgInfo.u32AudioPID = T_AID;   //audio pid:106 audio type :EN_ES_AUDIO_AAC
	st_record_param.stProgInfo.u32PCRPID = T_PCRID;
	st_record_param.stProgInfo.u8VideoType = T_VTYPE;
	st_record_param.stProgInfo.u8AudioType = T_ATYPE;

	memset(&st_record_param1,0,sizeof(ST_TEST_FPP_DTV_CHANNEL_PARAM_T));
	st_record_param1.delivery_type = EN_FPP_DEMOD_DELIVERY_TYPE_CABLE;
	st_record_param1.unDeliveryInfo.cable.frequency = 259000;
	st_record_param1.unDeliveryInfo.cable.modulation = EN_FPP_DEMOD_QAM64;
	st_record_param1.unDeliveryInfo.cable.symbol = 6875;
	st_record_param1.stProgInfo.u32VideoPID = 0x191;
	st_record_param1.stProgInfo.u32AudioPID = 0x192;
	st_record_param1.stProgInfo.u32PCRPID = 0x191;
	st_record_param1.stProgInfo.u8VideoType = EN_ES_VIDEO_MPEG2VIDEO;
	st_record_param1.stProgInfo.u8AudioType = EN_ES_AUDIO_MPEG;
	delivery_type = EN_FPP_DEMOD_DELIVERY_TYPE_CABLE;

	_mutitsi_record_and_playback_test(&st_record_param1,&st_record_param,EN_FPP_DEMOD_DELIVERY_TYPE_INVALID,fpi_false,EN_FPP_TSI_PB_FF_1X);
}

void  test_tsi_Record_DVBC_DVBT_and_Playback_1x(void)
{
	ST_TEST_FPP_DTV_CHANNEL_PARAM_T st_record_param;
	ST_TEST_FPP_DTV_CHANNEL_PARAM_T st_record_param1;
	memset(&st_record_param,0,sizeof(ST_TEST_FPP_DTV_CHANNEL_PARAM_T));
	st_record_param.delivery_type = EN_FPP_DEMOD_DELIVERY_TYPE_CABLE;
	st_record_param.unDeliveryInfo.cable.frequency = 259000;
	st_record_param.unDeliveryInfo.cable.modulation = EN_FPP_DEMOD_QAM64;
	st_record_param.unDeliveryInfo.cable.symbol = 6875;
	st_record_param.stProgInfo.u32VideoPID = 0x191;
	st_record_param.stProgInfo.u32AudioPID = 0x192;
	st_record_param.stProgInfo.u32PCRPID = 0x191;
	st_record_param.stProgInfo.u8VideoType = EN_ES_VIDEO_MPEG2VIDEO;
	st_record_param.stProgInfo.u8AudioType = EN_ES_AUDIO_MPEG;

	memset(&st_record_param1,0,sizeof(ST_TEST_FPP_DTV_CHANNEL_PARAM_T));
	st_record_param1.delivery_type = EN_FPP_DEMOD_DELIVERY_TYPE_TERRESTRIAL;
	st_record_param1.unDeliveryInfo.terrestrial.frequency = T_FRE;
	st_record_param1.unDeliveryInfo.terrestrial.bandwidth = EN_FPP_DEMOD_BAND_8M;
	st_record_param1.stProgInfo.u32VideoPID = T_VID;
	st_record_param1.stProgInfo.u32AudioPID = T_AID;   //audio pid:106 audio type :EN_ES_AUDIO_AAC
	st_record_param1.stProgInfo.u32PCRPID = T_PCRID;
	st_record_param1.stProgInfo.u8VideoType = T_VTYPE;
	st_record_param1.stProgInfo.u8AudioType = T_ATYPE;
	_mutitsi_record_and_playback_test(&st_record_param,&st_record_param1,EN_FPP_DEMOD_DELIVERY_TYPE_INVALID,fpi_false,EN_FPP_TSI_PB_FF_1X);
}




//======================================================================================
//======================================================================================

static int suite_success_init(void) 
{
	init_board();
  	printf("\n\r0000:suite_success_init%s,%d\n\r",__FUNCTION__,__LINE__);
   
	
	//InitZoom(&pWindowInfo);

    return 0; 
}
static int suite_success_clean(void)
{
    return 0; 
}


//======================================================================================
#define TEST_CASE_FUNC_LIST(func)   {#func,func}

/*将测试用例添加到测试组中*/
static CU_TestInfo tests_success[] = {

    TEST_CASE_FUNC_LIST( test_tsi_Record_DVBT_and_Play_DVBC ),
    TEST_CASE_FUNC_LIST( test_tsi_Record_DVBC_and_Play_DVBT),
    TEST_CASE_FUNC_LIST( test_tsi_Record_DVBC_and_Play_DVBC ),
     TEST_CASE_FUNC_LIST( test_tsi_Record_DVBT_and_Play_DVBT ),
    TEST_CASE_FUNC_LIST( test_tsi_Record_DVBT_DVBC_and_Playback_1x ),
    TEST_CASE_FUNC_LIST( test_tsi_Record_DVBC_DVBT_and_Playback_1x ),

    CU_TEST_INFO_NULL,
};
#undef TEST_CASE_FUNC_LIST

/*声明测试组*/
static CU_SuiteInfo suites[] = {
  { "suite_test_mutitsi",  suite_success_init, suite_success_clean, NULL },
    CU_SUITE_INFO_NULL,
};

void thal_mutitsi_add_testcase(void) {
    assert(NULL != CU_get_registry());
    assert(!CU_is_test_running());

	suites[0].pTests = testinfo_init(suites[0].pName,tests_success);

    if(NULL == suites[0].pTests)
        return;
    
    /* 将测试组注册到测试框架中. */
    if (CU_register_suites(suites) != CUE_SUCCESS) {
        fprintf(stderr, "suite registration failed - %s\n",
            CU_get_error_msg());
        exit(-1);
    }
}



