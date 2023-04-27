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
	uint8_t checkSum;
	bool acknowledge;
	bool read;
	
	
}frameStatus;


//return a pointer on my frameHeader struct
void analyse_Status(char* framePtr,frameStatus * result)
{
		
		char statusByte = framePtr[framePtr[2] + 3]; 
		
	
	
		//we do a mask on the first bit to have only the ack bit
		result->acknowledge = statusByte & 0x1;
		//get rid of three first bits leaves us with src_addr
		result->read = (statusByte & 0x2) == 2;
		result->checkSum = (statusByte & 0xFC) >> 2;
	
}

//return a pointer of on my frameHeader struct
void analyse_Header(char* framePtr, frameHeader * result)
{
	
		//we do a mask on the 3 first bits to have only the src Sapi
		result->src_sapi = (framePtr[0] & 0x7);
		//get rid of three first bits leaves us with src_addr
		result->src_addr = framePtr[0] >> 3;
		result->dst_sapi = framePtr[1] & 0x7;
		result->dst_addr = framePtr[1] >> 3;
	
}

void MacReceiver(void *argument)
{
	
	queueMsg_t macRxTemp;							// queue message
	osStatus_t rxStatus;											// return error code
	//frameHeader * frameHead = osMemoryPoolAlloc(memPool,osWaitForever);;
	//frameStatus * frameStat = osMemoryPoolAlloc(memPool,osWaitForever);;
	uint8_t myCheckSum;
	for (;;)														// loop until doomsday
	{
	//receive my msg and place it in macRxTemp
	rxStatus = osMessageQueueGet( queue_macR_id,&macRxTemp,NULL,osWaitForever); 
	
	char * framePtr = (char *) macRxTemp.anyPtr;
	
	
	if(rxStatus == osOK)
		{
			//IDENTIFYING BYTES	
			//frameHead = (char *) macRxTemp.anyPtr; //before  
			
			//I am a TOKEN
			if(framePtr[0] == TOKEN_TAG) 
			{
				//I send my TOKEN back to MAC sender
				macRxTemp.type = TOKEN;
				rxStatus = osMessageQueuePut(queue_macS_id, &macRxTemp, osPriorityNormal, osWaitForever);
			}
			else{
				DebugFrame("I am a DATA");
				//I analyse data
				frameHeader frameHead;
				frameStatus frameStat;
				analyse_Header(framePtr, &frameHead); // update
				analyse_Status(framePtr,&frameStat);
				
				macRxTemp.type = DATA_IND;
				//identify if we are destination or if msg is broadcast
												//MY address																//broadcast address
				if(frameHead.dst_addr == MYADDRESS || frameHead.dst_addr == BROADCAST_ADDRESS)
				{
					myCheckSum = 0;
					//compute CheckSum
					//framePtr[2] = length of frame
					//do I go to length or length - 1
					for(int i = 0; i < (framePtr[2]+3); i++)
					{
						myCheckSum += framePtr[i];
					}
					//keep just 6 LSB of crc
					myCheckSum = myCheckSum & 0x3F;
					//CS ok or no 
					if(frameStat.checkSum == myCheckSum)
					{
						frameStat.read = 1;
						frameStat.acknowledge = 1;
						
						//Send msg to mac sender
						macRxTemp.type = DATABACK;
						framePtr[framePtr[2]+3] = frameStat.checkSum + frameStat.read + frameStat.acknowledge; 
						rxStatus = osMessageQueuePut(queue_macS_id, &macRxTemp, osPriorityNormal, osWaitForever);
										//identify dst SAPI == chat
								if(frameHead.dst_sapi == CHAT_SAPI)
								{
									//give msg to  chat Rx
										rxStatus = osMessageQueuePut(queue_chatR_id, &macRxTemp, osPriorityNormal, osWaitForever);
								}
								//identify dst SAPI == TIME
								else if(frameHead.dst_sapi == TIME_SAPI)
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
						frameStat.read = 1;
						frameStat.acknowledge = 0;
						macRxTemp.type = DATABACK;
						framePtr[framePtr[2]+3] = frameStat.checkSum + frameStat.read + frameStat.acknowledge; 
						rxStatus = osMessageQueuePut(queue_macS_id, &macRxTemp, osPriorityNormal, osWaitForever);
	
					}
				
					//CHECK OF THE SOURCE 
					//I am the source 
					if(frameHead.src_addr == MYADDRESS)
					{
						//check if dst has red the msg correctly 
						if(frameStat.read == 1)
						{
								//check if dst computed the checkSum correctly
								if(frameStat.acknowledge == 1) 
								{
										//GIVE TOKEN
									macRxTemp.type = TOKEN;
									rxStatus = osMessageQueuePut(queue_macS_id, &macRxTemp, osPriorityNormal, osWaitForever);
			
								}								
								else
								{
									frameStat.acknowledge = 0;
									frameStat.read = 0;
									//EMIT DATA ?? 
								}
								
						}
						else
						{
							//INDICATE MAC ERROR AND GIVE TOKEN
							
							
							macRxTemp.type = TOKEN;
							rxStatus = osMessageQueuePut(queue_macS_id, &macRxTemp, osPriorityNormal, osWaitForever);

						}
						
					}
					else {
						//IS IT A BROADCAST 
						
						
					}
					
				}
				//we are not destination 
				else {
					//am I the src
					if(frameHead.src_addr == MYADDRESS)
					{
						//i give msg to sender so analyse and decide what to do
						macRxTemp.type = DATABACK;
						rxStatus = osMessageQueuePut(queue_macS_id, &macRxTemp, osPriorityNormal, osWaitForever);
					}
					else{
						//need to pass msg to next colleague
						macRxTemp.type = TO_PHY;
						rxStatus = osMessageQueuePut(queue_phyS_id, &macRxTemp, osPriorityNormal, osWaitForever);					
					}
					
					
					}
				
			}
			
		}
	}
	
}