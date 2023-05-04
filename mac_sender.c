#include "stm32f7xx_hal.h"

#include <stdio.h>
#include <string.h>

#include "main.h"


	
	uint8_t *myData = NULL;

void MacSender(void *argument)
{
	typedef struct TokenData{
		uint8_t token_id;
		uint8_t	station_list[15];			///< 0 to 15 
	} TokenData ;
	
	char*msg;
	char*myDataError ;
	
	TokenData *myToken;
	uint8_t * tokenPtr;
	uint8_t try_crc;
	uint8_t * originalPtr;
	uint8_t *myDataBack ;

	bool_t change;
	
	queueMsg_t macSenderRx;	
	queueMsg_t macSenderRx_IN;	
	queueMsg_t macSenderTx;	
	queueMsg_t macSenderTx_T;
	
	osStatus_t retCode;
	osStatus_t tempQstatus;
	osStatus_t tempQstatus_IN;
	
	uint16_t crc;
	osMessageQueueId_t queue_macS_IN_id;
	const osMessageQueueAttr_t queue_macS_IN_attr = {
	.name = "MAC_SENDER  "  	
};
	queue_macS_IN_id = osMessageQueueNew(3,sizeof(struct queueMsg_t),&queue_macS_IN_attr);
	
	for (;;)														// loop until doomsday
	{
		//wait that a msg arrives in 
		tempQstatus = osMessageQueueGet(queue_macS_id,&macSenderRx,NULL,osWaitForever);
		//if we read correctly
		if(tempQstatus == osOK)
		{
					switch(macSenderRx.type)
					{
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
						//receive a message asking to create a token
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
						case NEW_TOKEN :
							try_crc = 0;
							assert_param(myToken == NULL);
							myToken = osMemoryPoolAlloc(memPool,osWaitForever); //memory allocation of 100 bytes for my frame
							myToken->token_id = TOKEN_TAG;						
							for( int i = 0; i < 15; i ++)
							{	
								//fill info for my station for time/chat sapi
								if(i == MYADDRESS)
								{
									myToken->station_list[i] = (1<<TIME_SAPI) | (1<<CHAT_SAPI);
									gTokenInterface.myAddress = MYADDRESS;
									change = false;
								}
								//fill info for other station to 0
								else{
									myToken->station_list[i] = 0;
									gTokenInterface.station_list[i] = 0;
								}
							}
							
							macSenderTx.type = TO_PHY;
							macSenderTx.anyPtr = myToken;
							tempQstatus = osMessageQueuePut(queue_phyS_id, &macSenderTx, osPriorityNormal, osWaitForever); //memory free at PHY
							gTokenInterface.connected = true;
							myToken = NULL;
										
						break;
							
						case START:
							gTokenInterface.connected = true;
						break;
						
						case STOP:
							gTokenInterface.connected = false ;
						break;
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////							
						// from CHAT Sender or Time Sender
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
						case DATA_IND :					
							crc = 0;
							// control (2 bytes)
							myData = osMemoryPoolAlloc(memPool,osWaitForever);   			//memory allocation of 100 bytes for my frame	
							myData[0] = 0;
							if(macSenderRx.addr == BROADCAST_ADDRESS){
								myData[0] = (MYADDRESS << 3)+ TIME_SAPI ;
							}
							else{
								myData[0] = (MYADDRESS << 3)+ CHAT_SAPI ;
							}
							myData[1] = 0;
							myData[1] = (macSenderRx.addr << 3)+ macSenderRx.sapi;
							msg = ((char*)macSenderRx.anyPtr);
							// length (1 byte)
							myData[2] = strlen(msg);
							// User Data (length bytes)
							for(uint16_t i = 0; i<myData[2] ; i++){
								myData[3+i] = msg[i];
							}
							// crc calculate
							for(uint16_t i = 0; i<(3+myData[2]) ; i++){
								crc += myData[i];
							}
							//crc + R&A = 0 (1 byte)
							myData[3+myData[2]] = (crc<<2); 
							
							macSenderTx.type = TO_PHY;
							macSenderTx.anyPtr = myData;
							tempQstatus = osMessageQueuePut(queue_macS_IN_id, &macSenderTx, osPriorityNormal, 1);					

								if(tempQstatus == osErrorTimeout)
								{
										retCode = osMemoryPoolFree(memPool,macSenderTx.anyPtr);					//FREE string SENDER from app => qulist full
										CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);								
								}									
								
							retCode = osMemoryPoolFree(memPool,macSenderRx.anyPtr);					//FREE string REICEIVE from app
							CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);											
						break;
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////						
						// from MACReceiver
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
						case TOKEN : 
							
							tokenPtr = macSenderRx.anyPtr;
							myToken = (TokenData *) tokenPtr;
							
							tempQstatus_IN = osMessageQueueGet(queue_macS_IN_id,&macSenderRx_IN,NULL,NULL);
							
							// Message to send in the qulist in													
							if(tempQstatus_IN == osOK)
							{					
								macSenderTx.type = macSenderRx_IN.type;
								macSenderTx.anyPtr = macSenderRx_IN.anyPtr;
								
								assert_param(originalPtr == NULL);
								originalPtr = osMemoryPoolAlloc(memPool,osWaitForever);
								memcpy(originalPtr, macSenderRx_IN.anyPtr,50);
								tempQstatus = osMessageQueuePut(queue_phyS_id, &macSenderTx, osPriorityNormal, osWaitForever);
								
							}
							
							// qulist is empty -> send the token
							else{
								if(gTokenInterface.connected){
									myToken->station_list[MYADDRESS] = (1<<TIME_SAPI) | (1<<CHAT_SAPI);
								}
								else{
									myToken->station_list[MYADDRESS] = (1<<TIME_SAPI);
								}
								
								//check if they are change in the station list
								for(uint16_t i = 0; i<15 ; i++){
									if(gTokenInterface.station_list[i] != myToken->station_list[i]){
										gTokenInterface.station_list[i] = myToken->station_list[i];
										change = true;
									}	
								}
								
								//if they are change in the station list -> send the new list
								if(change){
									macSenderTx.anyPtr = NULL;
									macSenderTx.addr = 0;
									macSenderTx.sapi = 0;
									macSenderTx.type = TOKEN_LIST;
									tempQstatus = osMessageQueuePut(queue_lcd_id, &macSenderTx, osPriorityNormal, osWaitForever);
									change = false;
								}
								
								macSenderTx.type = TO_PHY;
								macSenderTx.anyPtr = tokenPtr;
								tempQstatus = osMessageQueuePut(queue_phyS_id, &macSenderTx, osPriorityNormal, osWaitForever);
								
								tokenPtr = NULL;
								myToken = NULL;
							}
						break;
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////							
						// from MACReceiver
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
						case DATABACK :	
							myDataBack = macSenderRx.anyPtr;
							macSenderTx.type = TO_PHY;
						////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
							// read and ack == 1 -> message read and all good -> send the token
						////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
							if((myDataBack[3+myDataBack[2]]&0x03) == 0x03){								
								
								// free databack
								retCode = osMemoryPoolFree(memPool,myDataBack);					//FREE MY DATA BACK !
								CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);
								myDataBack = NULL;
								
								// free original
								retCode = osMemoryPoolFree(memPool,originalPtr);					//FREE MY DATA BACK !
								CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);
								originalPtr = NULL;
								
								// send token
								macSenderTx.anyPtr = tokenPtr;
								tempQstatus = osMessageQueuePut(queue_phyS_id, &macSenderTx, osPriorityNormal, osWaitForever);
								tokenPtr = NULL;
							}
						////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
							// read ok ; ack = 0 -> message read but crc error -> resend message 
						////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
							else if((myDataBack[3+myDataBack[2]]&0x02)== 0x02){
								
								//try to resend the message
								if(try_crc<3){
									
									// free databack
									retCode = osMemoryPoolFree(memPool,myDataBack);					//FREE MY DATA BACK !
									CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);
									myDataBack = NULL;
									
									// copy original to malloc
									macSenderTx.anyPtr= osMemoryPoolAlloc(memPool,osWaitForever); 
									memcpy(macSenderTx.anyPtr,originalPtr, 50);
									try_crc++;
									
									// send the copy
									tempQstatus = osMessageQueuePut(queue_phyS_id, &macSenderTx, osPriorityNormal, osWaitForever);
								}
								
								// mesage still have bad reiver => free the token
								else{
									
									//Send a repot to the LCD
									try_crc = 0;
									macSenderTx.type = MAC_ERROR;
									myDataError = osMemoryPoolAlloc(memPool,osWaitForever);
									sprintf(myDataError, "MAC error : \nStation %d probleme crc\n\0", (myDataBack[1]>>3)+1);  
									macSenderTx.anyPtr = myDataError;
									macSenderTx.addr = myDataBack[1]>>3;
									tempQstatus = osMessageQueuePut(queue_lcd_id, &macSenderTx, osPriorityNormal, osWaitForever);
									myDataError = NULL;
									
									// free databack
									retCode = osMemoryPoolFree(memPool,myDataBack);					//FREE MY DATA BACK !
									CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);
									myDataBack = NULL;
									
									// free original
									retCode = osMemoryPoolFree(memPool,originalPtr);					//FREE MY DATA BACK !
									CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);
									originalPtr = NULL;
									
									// send token
									macSenderTx_T.type = TO_PHY;
									macSenderTx_T.anyPtr = tokenPtr;

									tempQstatus = osMessageQueuePut(queue_phyS_id, &macSenderTx_T, osPriorityNormal, osWaitForever);
									tokenPtr = NULL;
								}
							}
						////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
							// read = 0 ; message never receive -> free the token ; send message error
						////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
							else {
								
								//Send a repot to the LCD
								macSenderTx.type = MAC_ERROR;
								myDataError = osMemoryPoolAlloc(memPool,osWaitForever);
								sprintf(myDataError, "MAC error : \nStation %d don't answer\n\0", (myDataBack[1]>>3)+1);  
								macSenderTx.anyPtr = myDataError;
								macSenderTx.addr = myDataBack[1]>>3;
								tempQstatus = osMessageQueuePut(queue_lcd_id, &macSenderTx, osPriorityNormal, osWaitForever);
								myDataError = NULL;
								
								// free databack
								retCode = osMemoryPoolFree(memPool,myDataBack);					//FREE MY DATA BACK !
								CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);
								myDataBack = NULL;
								
								// free original
								retCode = osMemoryPoolFree(memPool,originalPtr);					//FREE MY DATA BACK !
								CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);
								originalPtr = NULL;
								
								// send token
								macSenderTx_T.type = TO_PHY;
								macSenderTx_T.anyPtr = tokenPtr;
								tempQstatus = osMessageQueuePut(queue_phyS_id, &macSenderTx_T, osPriorityNormal, osWaitForever);
								tokenPtr = NULL;
							}

						break; 

						default :
						break;
						
					}	
		}
	
	}
}
