#include "stm32f7xx_hal.h"

#include <stdio.h>
#include <string.h>

#include "main.h"


	


void MacSender(void *argument)
{
	typedef struct TokenData{
		uint8_t token_id;
		uint8_t	station_list[15];			///< 0 to 15 
	} TokenData ;
	
	char*msg;
	TokenData *myToken;
	uint8_t *myData ;
	char*myDataError ;
	uint8_t *myDataBack ;
	bool_t change;

	//DataInd *myData;
	//DataControl myControl;
	
	queueMsg_t macSenderRx;	
	queueMsg_t macSenderRx_IN;	
	queueMsg_t macSenderTx;	
	//osMessageQueueId_t queue_macS_temp;
	osStatus_t tempQstatus;
	osStatus_t tempQstatus_IN;
	
	uint16_t crc;
	osMessageQueueId_t queue_macS_IN_id;
	const osMessageQueueAttr_t queue_macS_IN_attr = {
	.name = "MAC_SENDER  "  	
};
	queue_macS_IN_id = osMessageQueueNew(4,sizeof(struct queueMsg_t),&queue_macS_IN_attr);
	
	for (;;)														// loop until doomsday
	{
		//wait that a msg arrives in 
		tempQstatus = osMessageQueueGet(queue_macS_id,&macSenderRx,NULL,osWaitForever);
		//if we read correctly
		if(tempQstatus == osOK)
		{
					switch(macSenderRx.type)
					{
						//receive a message asking to create a token
						case NEW_TOKEN :
							//memory allocation of 100 bytes for my frame
							myToken = osMemoryPoolAlloc(memPool,osWaitForever);
							//indicates that frame is a token	
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
							tempQstatus = osMessageQueuePut(queue_phyS_id, &macSenderTx, osPriorityNormal, osWaitForever);
							gTokenInterface.connected = true;
						break;
							
						case START:
							gTokenInterface.connected = true;
						break;
						
						case STOP:
							gTokenInterface.connected = false ;
						break;
							
						// from CHAT Sender or Time Sender
						case DATA_IND :
							//memory allocation of 100 bytes for my frame						
							crc = 0;
							// control (2 bytes)
							myData = osMemoryPoolAlloc(memPool,osWaitForever);
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
							//crc + R&A = 0
							myData[3+myData[2]] = (crc<<2); 
							
							macSenderTx.type = TO_PHY;
							macSenderTx.anyPtr = myData;
							tempQstatus = osMessageQueuePut(queue_macS_IN_id, &macSenderTx, osPriorityNormal, osWaitForever);							
						break;
						
						// from MACReceiver
						case TOKEN : 
							tempQstatus_IN = osMessageQueueGet(queue_macS_IN_id,&macSenderRx_IN,NULL,NULL);
							myToken = macSenderRx.anyPtr;
							// message in qulist -> send message
							if(tempQstatus_IN == osOK)
							{
								macSenderTx = macSenderRx_IN ;
								tempQstatus = osMessageQueuePut(queue_phyS_id, &macSenderTx, osPriorityNormal, osWaitForever);
							}
							// qulist in empty -> send the token
							else{
								if(gTokenInterface.connected){
									myToken->station_list[MYADDRESS] = (1<<TIME_SAPI) | (1<<CHAT_SAPI);
								}
								else{
									myToken->station_list[MYADDRESS] = 0;
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
								macSenderTx.anyPtr = myToken;
								tempQstatus = osMessageQueuePut(queue_phyS_id, &macSenderTx, osPriorityNormal, osWaitForever);
							}
						break;
							
						// from MACReceiver
						case DATABACK :
							myDataBack = osMemoryPoolAlloc(memPool,osWaitForever);
							myDataBack = (uint8_t*)macSenderRx.anyPtr;
							macSenderTx.type = TO_PHY;
							// read and ack == 1 -> message read and all good -> send the token
							if(myDataBack[3+myDataBack[2]]&3== 3){
								myToken->station_list[MYADDRESS] = (1<<TIME_SAPI) | (1<<CHAT_SAPI);
								macSenderTx.anyPtr = myToken;
								tempQstatus = osMessageQueuePut(queue_phyS_id, &macSenderTx, osPriorityNormal, osWaitForever);
							}
							// read ok ; ack = 0 -> message read but crc error -> resend message 
							else if(myDataBack[3+myDataBack[2]]&2== 2){
								myDataBack[2+myDataBack[2]]= myDataBack[2+myDataBack[2]]& 0xFC; //r&a -> 0
								macSenderTx = macSenderRx ;
								tempQstatus = osMessageQueuePut(queue_phyS_id, &macSenderTx, osPriorityNormal, osWaitForever);
							}
							// read = 0 ; message never receive -> free the token ; send message error
							else {
								macSenderTx.type = MAC_ERROR;
								myDataError = osMemoryPoolAlloc(memPool,osWaitForever);
								sprintf(myDataError, "MAC error : \nStation %d don't answer\n \0", (myDataBack[1]>>3)+1);  
								//myDataError[myDataBack[2]+1] = 0 //VERIFIE !!!
								macSenderTx.anyPtr = myDataError;
								macSenderTx.addr = myDataBack[1]>>3;
								tempQstatus = osMessageQueuePut(queue_lcd_id, &macSenderTx, osPriorityNormal, osWaitForever);
								
								myToken->station_list[MYADDRESS] = (1<<TIME_SAPI) | (1<<CHAT_SAPI);
								macSenderTx.type = TO_PHY;
								macSenderTx.anyPtr = myToken;
								tempQstatus = osMessageQueuePut(queue_phyS_id, &macSenderTx, osPriorityNormal, osWaitForever);
							}
						break; 

						default :
						break;
						
					}	
		}
	
	}
}
