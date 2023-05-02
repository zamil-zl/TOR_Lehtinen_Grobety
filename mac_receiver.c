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
			//DebugFrame(frameHead);							// for debug info only
			//I am a TOKEN
			if(frameHead[0] == 0xFF)
			{
					//I send my TOKEN back to MAC sender
<<<<<<< Updated upstream
=======
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
						//necessary to calculate CRC ????
						frameStat.acknowledge = 1;
						showMsg(frameHead.dst_sapi);
						frameStat.read = 1;
						framePtr[framePtr[2]+3] = frameStat.acknowledge | frameStat.read << 1 | frameStat.checkSum << 2; 
						macRxTemp.anyPtr = framePtr;
						rxStatus = osMessageQueuePut(queue_macS_id, &macRxTemp, osPriorityNormal, osWaitForever);
								//what do we do
						}				
						break;

						default :					
						break;
					}
						
				}
>>>>>>> Stashed changes
				
				macRxTemp.type = TOKEN;
				rxStatus = osMessageQueuePut(queue_macS_id, &macRxTemp, osPriorityNormal, osWaitForever);
			}
			else{
				DebugFrame("I am a DATA");
			}
			
			
			
		}
	
	
	
	
}