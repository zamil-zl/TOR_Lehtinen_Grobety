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
	//DataInd *myData;
	//DataControl myControl;
	
	queueMsg_t macSenderRx;	
	queueMsg_t macSenderRx_IN;	
	queueMsg_t macSenderTx;	
	//osMessageQueueId_t queue_macS_temp;
	osStatus_t tempQstatus;
	osStatus_t tempQstatus_IN;
	
	uint16_t crc;
	osMessageQueueId_t queue_macS_id;
	osMessageQueueId_t queue_macS_IN_id;
	const osMessageQueueAttr_t queue_macS_IN_attr = {
	.name = "MAC_SENDER  "  	
};
	queue_macS_id = osMessageQueueNew(2,sizeof(struct queueMsg_t),&queue_macS_IN_attr);
	
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
									myToken->station_list[i] = 0xA;
								}
								//fill info for other station to 0
								else{
									myToken->station_list[i] = 0;
								}
								
							}
							macSenderTx.type = TO_PHY;
							macSenderTx.anyPtr = myToken;
							tempQstatus = osMessageQueuePut(queue_phyS_id, &macSenderTx, osPriorityNormal, osWaitForever);
								
						break;
							
						case DATA_IND :
							//memory allocation of 100 bytes for my frame						
							crc = 0;
							myData = osMemoryPoolAlloc(memPool,osWaitForever);
						
							myData[0] = (macSenderRx.addr << 2);
							myData[1] = macSenderRx.sapi;
							myData[2] = (MYADDRESS << 2);
							myData[3] = 0xA ;
							msg = ((char*)macSenderRx.anyPtr);
							myData[4] = strlen(msg);
							for(uint16_t i = 0; i<myData[4] ; i++){
								myData[5+i] = msg[i];
							}
							for(uint16_t i = 0; i<5+myData[4] ; i++){
								crc = myData[i];
							}
							myData[5+myData[4]] = (crc<<2);
							
							macSenderTx.type = TO_PHY;
							macSenderTx.anyPtr = myData;
							tempQstatus = osMessageQueuePut(queue_macS_id, &macSenderTx, osPriorityNormal, osWaitForever);							
							//TO DO : config the queu to block message to send until reicive token
						break;
						
						
						case TOKEN :
							//I arrive here 
							tempQstatus_IN = osMessageQueueGet(queue_macS_IN_id,&macSenderRx_IN,NULL,osWaitForever);
							macSenderTx = macSenderRx_IN ;
							tempQstatus = osMessageQueuePut(queue_phyS_id, &macSenderTx, osPriorityNormal, osWaitForever);
						

							myToken = macSenderTx.anyPtr;
							for( int i = 0; i < 15; i ++){	
								//fill info for my station for time/chat sapi
								if(i == MYADDRESS)
								{
									myToken->station_list[i] = 0xA;
								}
								//fill info for other station to 0								
							}
							myToken.type = TO_PHY;
							macSenderTx = myToken;
							tempQstatus = osMessageQueuePut(queue_phyS_id, &macSenderTx, osPriorityNormal, osWaitForever);
								
						break;
						
						

						default :
							
						
						break;
						
						
					}
			
			
			
			
			
			
		}
		
		
		
	}
	
}
