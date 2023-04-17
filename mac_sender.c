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
	
	TokenData *myToken;
	queueMsg_t macSenderRx;	
	queueMsg_t macSenderTx;	
	//osMessageQueueId_t queue_macS_temp;
	osStatus_t tempQstatus;
	
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
							
						break;
						
						default :
							
						
						break;
						
						
					}
			
			
			
			
			
			
		}
		
		
		
	}
	
}
