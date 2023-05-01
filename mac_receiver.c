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

frameHeader frameHead;
char * framePtr;
	osStatus_t rxStatus;											// return error code
queueMsg_t dataIndMsg;	

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
	// TODO think about checking if you're the dst / not / broadcast
	//once done at teh following check if you're src - to know if databck or no
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
	else if (myFrameHeader.dst_addr == BROADCAST_ADDRESS){
		return broadcast;
	}
	else if(myFrameHeader.src_addr == MYADDRESS)
	{
		result = iAmSrc;
		return result;
	}
	
	else{
		//error case
	}
	
}


void showMsg(uint8_t correctSapi)
{
	
			dataIndMsg.type = DATA_IND;
		char * lcdMsg = osMemoryPoolAlloc(memPool,osWaitForever);
			//I get rid of status byte
			//*framePtr = *framePtr >> 1;
			//i fill data info into .anyPtr
			for(uint8_t i = 0; i < (frameHead.userDataLength) ; i++)
			{
				//I want the user data starting in byte 3
				lcdMsg[i] = framePtr[i+3];
			}
			lcdMsg[frameHead.userDataLength] = '\0';
			//I fill src addr and src sapi
			dataIndMsg.anyPtr = lcdMsg;
			dataIndMsg.addr = frameHead.src_addr;
			dataIndMsg.sapi = frameHead.src_sapi;
			//give msg to  chat Rx

			osMessageQueueId_t miaou = queue_chatR_id;
			if(correctSapi == TIME_SAPI)
			{
				miaou = queue_timeR_id;	
			}
			
			rxStatus = osMessageQueuePut(miaou, &dataIndMsg, osPriorityNormal, osWaitForever);

	
}

bool verify_CRC(uint8_t crcToCheck)
{

	uint8_t myCheckSum = 0;

	
	
			for(int i = 0; i < (framePtr[2]+3); i++)
			{
				myCheckSum += framePtr[i];
			}
			//keep just 6 LSB of crc
			myCheckSum = myCheckSum & 0x3F;
						
						
	return (crcToCheck == myCheckSum);
		
}



void MacReceiver(void *argument)
{
	
	queueMsg_t macRxTemp;							// queue message
	//frameHeader * frameHead = osMemoryPoolAlloc(memPool,osWaitForever);;
	//frameStatus * frameStat = osMemoryPoolAlloc(memPool,osWaitForever);;
	//uint8_t myCheckSum;
	for (;;)														// loop until doomsday
	{
	//receive my msg and place it in macRxTemp
	rxStatus = osMessageQueueGet( queue_macR_id,&macRxTemp,NULL,osWaitForever); 
	enum state msgState;
	framePtr = (char *) macRxTemp.anyPtr;
	
	
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
					//DebugFrame("I am a DATA");
					//I analyse data
					
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
							
						//compute CheckSum
						//framePtr[2] = length of frame
						//do I go to length or length - 1
						//for(int i = 0; i < (framePtr[2]+3); i++)
						//{
							//myCheckSum += framePtr[i];
						//}
						//keep just 6 LSB of crc
						//myCheckSum = myCheckSum & 0x3F;
						//CS ok or no 
						//I am connected
						if(gTokenInterface.connected){
							frameStat.read = 1; // depends if I am co
								//if(frameStat.checkSum == myCheckSum)
								if(verify_CRC(frameStat.checkSum))
								{
									frameStat.acknowledge = 1;
									dataIndMsg.type = DATA_IND;
									
									showMsg(frameHead.dst_sapi);
									
								}
								else //checksum not ok
								{
										frameStat.acknowledge = 0;
								}
								
									//give back to phy and no DATABACK
											macRxTemp.type = TO_PHY;
								
						}
						else	//not co = R & A : 0 + send to phy
						{
							frameStat.read = 0; // depends if I am co
							frameStat.acknowledge = 0;
							
						}
						//putting back status byte to correct place
						framePtr[framePtr[2]+3] = frameStat.acknowledge | frameStat.read << 1 | frameStat.checkSum << 2; 
							macRxTemp.anyPtr = framePtr;
							rxStatus = osMessageQueuePut(queue_phyS_id, &macRxTemp, osPriorityNormal, osWaitForever);
		
					
							break;
						
						case iAmDst_Src :
							//I give databack so sender free token // no touch to any bits
						macRxTemp.type = DATABACK;
						//verify checksum 
						if(verify_CRC(frameStat.checkSum))
						{
									//verify connection state
									if(gTokenInterface.connected)
									{
										//change read & ack 
										frameStat.acknowledge = 1;
										showMsg(frameHead.dst_sapi);
										frameStat.read = 1;
										framePtr[framePtr[2]+3] = frameStat.acknowledge | frameStat.read << 1 | frameStat.checkSum << 2; 
										macRxTemp.anyPtr = framePtr;
										rxStatus = osMessageQueuePut(queue_macS_id, &macRxTemp, osPriorityNormal, osWaitForever);
									}
									else{
									// nothing should happen bcse I am not connected		
									}
						}	
							
						break;
						
						
						case iAmSrc :
							
						macRxTemp.type = DATABACK;
						rxStatus = osMessageQueuePut(queue_macS_id, &macRxTemp, osPriorityNormal, osWaitForever);
							
						break;
						
						
						case broadcast: 
							
						//identify sapi to show in LCD
					
						//continue pass broadcast + no change of read & ack
						if(frameHead.src_addr != MYADDRESS)			
						{
							//if I am connected i show 
							if(gTokenInterface.connected)
							{
								//showLCD
								showMsg(frameHead.dst_sapi);
							}
							//if not connected just pass on 
						macRxTemp.type = TO_PHY;
						rxStatus = osMessageQueuePut(queue_phyS_id, &macRxTemp, osPriorityNormal, osWaitForever);
						}
						else {
						macRxTemp.type = DATABACK;
						rxStatus = osMessageQueuePut(queue_macS_id, &macRxTemp, osPriorityNormal, osWaitForever);
						//what do we do
						}
							
						break;
						
						
						default :
							
						
						break;
					}
						
				}
				
			}
			
		}
				
	}

	