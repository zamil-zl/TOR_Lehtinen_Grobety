#include "stm32f7xx_hal.h"

#include <stdio.h>
#include <string.h>

#include "main.h"

enum state{iAmDst, iAmDst_Src, iAmNotDst,  iAmSrc, broadcast};


typedef struct frameHeader
{
	uint8_t src_sapi;
	uint8_t src_addr;
	uint8_t dst_sapi;
	uint8_t dst_addr;
	uint8_t userDataLength
	
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
		result->userDataLength = framePtr[2];
	
}

enum state analyse_state(frameHeader myFrameHeader) 
{
	enum state result;
	if(myFrameHeader.dst_addr == MYADDRESS && myFrameHeader.src_addr == MYADDRESS)
	{
		result = iAmDst_Src;
		return result;
	}
	else if(myFrameHeader.dst_addr == MYADDRESS)
	{
		result = iAmDst;
		return result;
	}
	else if(myFrameHeader.dst_addr == MYADDRESS)
	{
		result = broadcast;
		return result;
	}
	
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
	enum state msgState;
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
				msgState = analyse_state(frameHead);
				
				
				
				switch(msgState)
				{
					case iAmDst:
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
						macRxTemp.anyPtr = framePtr;
						rxStatus = osMessageQueuePut(queue_macS_id, &macRxTemp, osPriorityNormal, osWaitForever);
						
						macRxTemp.type = DATA_IND;
						if(frameHead.dst_sapi == CHAT_SAPI)
								{
									//i fill data info into .anyPtr
									for(uint8_t i = 0; i < (frameHead.userDataLength-3) ; i++)
									{
										//I want the user data starting in byte 3
										framePtr[i] = framePtr[i+3];
									}
									//I fill src addr and src sapi after
									framePtr[frameHead.userDataLength] = frameHead.src_addr + frameHead.src_sapi;
									macRxTemp.anyPtr = framePtr;
									//give msg to  chat Rx
										rxStatus = osMessageQueuePut(queue_chatR_id, &macRxTemp, osPriorityNormal, osWaitForever);
								}
								//identify dst SAPI == TIME
								else if(frameHead.dst_sapi == TIME_SAPI)
								{
									//i fill data info into .anyPtr
									for(uint8_t i = 0; i < (frameHead.userDataLength-3) ; i++)
									{
										//I want the user data starting in byte 3
										framePtr[i] = framePtr[i+3];
									}
									//I fill src addr and src sapi after
									framePtr[frameHead.userDataLength] = frameHead.src_addr + frameHead.src_sapi;
									macRxTemp.anyPtr = framePtr;
									//give msg to time Rx
									rxStatus = osMessageQueuePut(queue_timeR_id, &macRxTemp, osPriorityNormal, osWaitForever);
								}
						}
					else //checksum not ok
					{
						//give back to phy and no DATABACK
						frameStat.read = 1;
						frameStat.acknowledge = 0;
						macRxTemp.type = DATABACK;
						framePtr[framePtr[2]+3] = frameStat.checkSum + frameStat.read + frameStat.acknowledge; 
						macRxTemp.anyPtr = framePtr;
						rxStatus = osMessageQueuePut(queue_macS_id, &macRxTemp, osPriorityNormal, osWaitForever);
	
					}
					
						break;
					
					case iAmDst_Src :
						//I give databack so sender free token // no touch to any bits
						macRxTemp.type = DATABACK;
						//read And ACK to Change????
					
					
						rxStatus = osMessageQueuePut(queue_macS_id, &macRxTemp, osPriorityNormal, osWaitForever);
							
					
						//identify sapi to show in LCD
						macRxTemp.type = DATA_IND;
						if(frameHead.dst_sapi == CHAT_SAPI)
								{
									//i fill data info into .anyPtr
									for(uint8_t i = 0; i < (frameHead.userDataLength-3) ; i++)
									{
										//I want the user data starting in byte 3
										framePtr[i] = framePtr[i+3];
									}
									//I fill src addr and src sapi after
									framePtr[frameHead.userDataLength] = frameHead.src_addr + frameHead.src_sapi;
									macRxTemp.anyPtr = framePtr;
									//give msg to  chat Rx
										rxStatus = osMessageQueuePut(queue_chatR_id, &macRxTemp, osPriorityNormal, osWaitForever);
								}
								//identify dst SAPI == TIME
								else if(frameHead.dst_sapi == TIME_SAPI)
								{
									//i fill data info into .anyPtr
									for(uint8_t i = 0; i < (frameHead.userDataLength-3) ; i++)
									{
										//I want the user data starting in byte 3
										framePtr[i] = framePtr[i+3];
									}
									//I fill src addr and src sapi after
									framePtr[frameHead.userDataLength] = frameHead.src_addr + frameHead.src_sapi;
									macRxTemp.anyPtr = framePtr;
									//give msg to time Rx
									rxStatus = osMessageQueuePut(queue_timeR_id, &macRxTemp, osPriorityNormal, osWaitForever);
								}
						
						
					break;
					
					
					case iAmSrc :
						
					macRxTemp.type = DATABACK;
					rxStatus = osMessageQueuePut(queue_macS_id, &macRxTemp, osPriorityNormal, osWaitForever);
							
					
						
					
					break;
					
					
					case broadcast: 
						
					//identify sapi to show in LCD
						macRxTemp.type = DATA_IND;
						if(frameHead.dst_sapi == CHAT_SAPI)
								{
									//i fill data info into .anyPtr
									for(uint8_t i = 0; i < (frameHead.userDataLength-3) ; i++)
									{
										//I want the user data starting in byte 3
										framePtr[i] = framePtr[i+3];
									}
									//I fill src addr and src sapi after
									framePtr[frameHead.userDataLength] = frameHead.src_addr + frameHead.src_sapi;
									macRxTemp.anyPtr = framePtr;
									//give msg to  chat Rx
										rxStatus = osMessageQueuePut(queue_chatR_id, &macRxTemp, osPriorityNormal, osWaitForever);
								}
								//identify dst SAPI == TIME
								else if(frameHead.dst_sapi == TIME_SAPI)
								{
									//i fill data info into .anyPtr
									for(uint8_t i = 0; i < (frameHead.userDataLength-3) ; i++)
									{
										//I want the user data starting in byte 3
										framePtr[i] = framePtr[i+3];
									}
									//I fill src addr and src sapi after
									framePtr[frameHead.userDataLength] = frameHead.src_addr + frameHead.src_sapi;
									macRxTemp.anyPtr = framePtr;
									//give msg to time Rx
									rxStatus = osMessageQueuePut(queue_timeR_id, &macRxTemp, osPriorityNormal, osWaitForever);
								}
					//continue pass broadcast
					if(frameHead.dst_addr != MYADDRESS)			
					{
					macRxTemp.type = TO_PHY;
					rxStatus = osMessageQueuePut(queue_phyS_id, &macRxTemp, osPriorityNormal, osWaitForever);
					}
					else {
					macRxTemp.type = TO_PHY;
					//rxStatus = osMessageQueuePut(queue_phyS_id, &macRxTemp, osPriorityNormal, osWaitForever);
					//what do we do
					}
						
					break;
					
					
					default :
						
					
					break;
				}
					
			}
			
		}
		
	}
				
				/*if(frameHead.dst_addr == MYADDRESS)
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
						macRxTemp.anyPtr = framePtr;
						rxStatus = osMessageQueuePut(queue_macS_id, &macRxTemp, osPriorityNormal, osWaitForever);
							*/			//identify dst SAPI == chat
							/*	if(frameHead.dst_sapi == CHAT_SAPI)
								{
									macRxTemp.type = DATA_IND;
									//i fill data info into .anyPtr
									for(uint8_t i = 0; i < (frameHead.userDataLength-3) ; i++)
									{
										//I want the user data starting in byte 3
										framePtr[i] = framePtr[i+3];
									}
									//I fill src addr and src sapi after
									framePtr[frameHead.userDataLength] = frameHead.src_addr + frameHead.src_sapi;
									macRxTemp.anyPtr = framePtr;
									//give msg to  chat Rx
										rxStatus = osMessageQueuePut(queue_chatR_id, &macRxTemp, osPriorityNormal, osWaitForever);
								}
								//identify dst SAPI == TIME
								else if(frameHead.dst_sapi == TIME_SAPI)
								{
									macRxTemp.type = DATA_IND;
									//give msg to time Rx
									rxStatus = osMessageQueuePut(queue_timeR_id, &macRxTemp, osPriorityNormal, osWaitForever);
								}*/
								//if nor time sapi nor chat sapi exist msg is a broadCast ??
								/*else{
									
								}
						
					}
					else
					{
						//give back to phy and no DATABACK
						frameStat.read = 1;
						frameStat.acknowledge = 0;
						macRxTemp.type = DATABACK;
						framePtr[framePtr[2]+3] = frameStat.checkSum + frameStat.read + frameStat.acknowledge; 
						macRxTemp.anyPtr = framePtr;
						rxStatus = osMessageQueuePut(queue_macS_id, &macRxTemp, osPriorityNormal, osWaitForever);
	
					}*/
				
					//CHECK OF THE SOURCE 
					//I am the source 
					/*if(frameHead.src_addr == MYADDRESS)
					{
						//check if dst has red the msg correctly 
						if(frameStat.read == 1)
						{
								//check if dst computed the checkSum correctly
								if(frameStat.acknowledge == 1) 
								{
										//GIVE MSG NO CHANGE
									macRxTemp.type = DATABACK;
									rxStatus = osMessageQueuePut(queue_macS_id, &macRxTemp, osPriorityNormal, osWaitForever);
			
								}								
								else
								{
									macRxTemp.type = DATABACK;
									rxStatus = osMessageQueuePut(queue_macS_id, &macRxTemp, osPriorityNormal, osWaitForever);
								}
								
						}
						else
						{
							//INDICATE read bit not 
							macRxTemp.type = DATABACK;
							rxStatus = osMessageQueuePut(queue_macS_id, &macRxTemp, osPriorityNormal, osWaitForever);
						}
						
					}
					else {
						//IS IT A BROADCAST 
						
						
					}
					
				}*/
				//we are not destination 
				//else {
					//am I the src
					/*if(frameHead.src_addr == MYADDRESS)
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
	*/
//}
}