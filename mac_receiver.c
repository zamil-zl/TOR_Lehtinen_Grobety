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
			//IDENTIFYING BYTES	
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
				macRxTemp.type = DATA_IND;
				//identify if we are destination 
				//do a mask of the bit at the correct position
												//MY address																//broadcast address
				if(frameHead[0] & 0b00011000 == 0b00011000 || frameHead[0] & 0b01111000 == 0b01111000)
				{
					
					
					//compute CheckSum
					
					
					
					
					
					
					
					
					
					
					/*//identify dst SAPI == chat
					if(frameHead[0] & CHAT_SAPI)
					{
						//give msg to  chat Rx
							rxStatus = osMessageQueuePut(queue_chatR_id, &macRxTemp, osPriorityNormal, osWaitForever);
					}
					//identify dst SAPI == TIME
					else if(frameHead[0] & CHAT_SAPI == TIME_SAPI)
					{
						//give msg to time Rx
							rxStatus = osMessageQueuePut(queue_timeR_id, &macRxTemp, osPriorityNormal, osWaitForever);
					}*/
					
					
					
				}
				//if msg is a broadCast = SPECIAL ADDRESS
				/*else if()
				{
					
					
				}*/
				
				//we are not dst we send back to PHY
				else {
					macRxTemp.type = TO_PHY;
					rxStatus = osMessageQueuePut(queue_phyS_id, &macRxTemp, osPriorityNormal, osWaitForever);
				}
				
				
				
				
				
				
				
				
				
				
				
				
				
			}
			
			
			
		}
	
	
	
	
}
