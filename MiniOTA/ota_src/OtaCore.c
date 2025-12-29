/*******************************************************************************
  * @file           : 
  * @brief          : 
  ******************************************************************************
  * @attention
  *
  *
  *****************************************************************************/
#include "OtaInterface.h"
#include "OtaXmodem.h"
#include "OtaPort.h"
#include "OtaJump.h"

void OTA_RunOTA(void)
{
	OTA_DebugSend("[OTA]step in RunOTA");
	OTA_XmodemInit();
	while(1)
	{
		/* 周期为1s得检查传输是否未开始 */
		if(OTA_GetXmodemHandle()->state == XM_WAIT_START && OTA_XmodemRevCompFlag() == 0)
		{
			OTA_SendByte(0x43);
			//OTA_DebugSend("[OTA]Send C\r\n");
			
			uint8_t i;
			for(i = 0; i < 100 ; i++ )
			{
				OTA_Delay1ms();
			}
		}
		
		if(OTA_XmodemRevCompFlag() == 2)
		{
			JumpToApp();
		}
	}
}
