#include "stm32f7xx_hal.h"

#include <stdio.h>
#include <string.h>

#include "main.h"

typedef struct frameHeader
{
	uint8_t src_sapi;
	uint8_t src_addr;
	uint8_t dst_sapi;
	uint8_t dst_addr;
	
}frameHeader;

typedef struct frameStatus
{
	
	bool acknowledge;
	bool read;
	uint8_t checkSum;
	
}frameStatus;


//return a pointer on my frameHeader struct
frameStatus * analyse_Status(char* framePtr)
{
		frameStatus *myFrame; //osMemoryPoolAlloc(memPool,osWaitForever); if no mempool alloc pointer = NULL ???

		//we do a mask on the first bit to have only the ack bit
		myFrame->acknowledge = framePtr[0] & 0x1;
		//get rid of three first bits leaves us with src_addr
		myFrame->read = framePtr[0] & 0x2;
		myFrame->checkSum = framePtr[0] >> 2;
	
		return myFrame;
}

//return a pointer of on my frameHeader struct
frameHeader * analyse_Header(char* framePtr)
{
		frameHeader *myFrame; //osMemoryPoolAlloc(memPool,osWaitForever); if no mempool alloc pointer = NULL ???
	
		//we do a mask on the 3 first bits to have only the src Sapi
		myFrame->src_sapi = (framePtr[0] & 0x7);
		//get rid of three first bits leaves us with src_addr
		myFrame->src_addr = framePtr[0] >> 3;
		myFrame->dst_sapi = framePtr[1] & 0x7;
		myFrame->dst_addr = framePtr[1] >> 3;
	
		return myFrame;
}

void MacReceiver(void *argument)
{
	
	queueMsg_t macRxTemp;							// queue message
	osStatus_t rxStatus;											// return error code
	frameHeader * frameHead;
	frameStatus * frameStat;
	uint8_t myCheckSum;
	for (;;)														// loop until doomsday
	{
	//receive my msg and place it in macRxTemp
	rxStatus = osMessageQueueGet( queue_macR_id,&macRxTemp,NULL,osWaitForever); 
	
	char * msgHeader = (char *) macRxTemp.anyPtr;
	
	
	if(rxStatus == osOK)
		{
			//IDENTIFYING BYTES	
			//frameHead = (char *) macRxTemp.anyPtr; //before  
			
			//I am a TOKEN
			if(msgHeader[0] == TOKEN_TAG) 
			{
				//I send my TOKEN back to MAC sender
				macRxTemp.type = TOKEN;
				rxStatus = osMessageQueuePut(queue_macS_id, &macRxTemp, osPriorityNormal, osWaitForever);
			}
			else{
				DebugFrame("I am a DATA");
				//I analyse data
				frameHead = analyse_Header((char *) macRxTemp.anyPtr); // update
				frameStat = analyse_Status((char *) macRxTemp.anyPtr);
				
				macRxTemp.type = DATA_IND;
				//identify if we are destination or if msg is broadcast
												//MY address																//broadcast address
				if(frameHead->dst_addr == MYADDRESS || frameHead->dst_addr == BROADCAST_ADDRESS)
				{
					
					//compute CheckSum
					//msgHeader[2] = length of frame
					//do I go to length or length - 1
					for(int i = 0; i < msgHeader[2]; i++)
					{
						myCheckSum += msgHeader[i];
					}
					//CS ok or no 
					if(frameStat->checkSum == myCheckSum)
					{
						frameStat->read = 1;
						frameStat->acknowledge = 1;
						
						//Send msg to mac sender
						rxStatus = osMessageQueuePut(queue_macS_id, &macRxTemp, osPriorityNormal, osWaitForever);
										//identify dst SAPI == chat
								if(frameHead->dst_sapi == CHAT_SAPI)
								{
									//give msg to  chat Rx
										rxStatus = osMessageQueuePut(queue_chatR_id, &macRxTemp, osPriorityNormal, osWaitForever);
								}
								//identify dst SAPI == TIME
								else if(frameHead->dst_sapi == TIME_SAPI)
								{
									//give msg to time Rx
										rxStatus = osMessageQueuePut(queue_timeR_id, &macRxTemp, osPriorityNormal, osWaitForever);
								}
								//if nor time sapi nor chat sapi exist msg is a broadCast ??
								else{
									
								}
						
					}
					else
					{
						frameStat->read = 1;
						frameStat->acknowledge = 0;
							
						//TODO : Indicate that we have an error	
					}
				
					//CHECK OF THE SOURCE 
					//I am the source 
					if(frameHead->src_addr == MYADDRESS)
					{
						//check if dst has red the msg correctly 
						if(frameStat->read == 1)
						{
								//check if dst computed the checkSum correctly
								if(frameStat->acknowledge == 1) 
								{
										//GIVE TOKEN
								}								
								else
								{
									frameStat->acknowledge = 0;
									frameStat->read = 0;
									//EMIT DATA ?? 
								}
								
						}
						else
						{
							//INDICATE MAC ERROR AND GIVE TOKEN
						}
						
					}
					else {
						//IS IT A BROADCAST 
						
						
					}
					
				}
				//we are not destination
				else {
					macRxTemp.type = TO_PHY;
					rxStatus = osMessageQueuePut(queue_phyS_id, &macRxTemp, osPriorityNormal, osWaitForever);
				}
				
			}
			
		}
	}
	
}