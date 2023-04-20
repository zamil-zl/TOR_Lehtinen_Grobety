#include "stm32f7xx_hal.h"

#include <stdio.h>
#include <string.h>

#include "main.h"

void MacReceiver(void *argument)
{
	// TODO
	
	queueMsg_t macRxTemp;							// queue message
	
	osStatus_t rxStatus;											// return error code
	/*
	typedef struct FrameHead{
		uint8_t controlSrc;
		uint8_t controlDst;
		
	} FrameHead ;
	
		FrameHead *myFrame;
*/
	char * frameHead;
	//receive my msg and place it in macRxTemp
	rxStatus = osMessageQueueGet( queue_macR_id,&macRxTemp,NULL,osWaitForever); 
	
	
	if(rxStatus == osOK)
		{
				
			frameHead = (char *) macRxTemp.anyPtr;
			DebugFrame(frameHead);							// for debug info only
			//I am a TOKEN
			if(frameHead[0] == 0xFF)
			{
					//I send my TOKEN back to MAC sender
				
				macRxTemp.type = TOKEN;
				rxStatus = osMessageQueuePut(queue_macS_id, &macRxTemp, osPriorityNormal, osWaitForever);
			}
			else{
				DebugFrame("I am a DATA");
				
				
				
				
				
				
				
				
				
				
				
			}
			
			
			
		}
	
	
	
	
}