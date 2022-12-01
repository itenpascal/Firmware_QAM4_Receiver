/*
* qamdec.c
*
* Created: 05.05.2020 16:38:25
*  Author: Chaos Pascal Philipp
*/ 

#include "avr_compiler.h"
#include "pmic_driver.h"
#include "TC_driver.h"
#include "clksys_driver.h"
#include "sleepConfig.h"
#include "port_driver.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "event_groups.h"
#include "semphr.h"
#include "stack_macros.h"

#include "mem_check.h"
#include "errorHandler.h"

#include "qaminit.h"
#include "qamdec.h"

QueueHandle_t decoderQueue;

void vQuamDec(void* pvParameters)
{
	( void ) pvParameters;
	
	decoderQueue = xQueueCreate( 4, NR_OF_SAMPLES * sizeof(int16_t) );
	
	while(evDMAState == NULL) {
		vTaskDelay(3/portTICK_RATE_MS);
	}
	
	uint16_t bufferelement[NR_OF_SAMPLES];									// 32 Samples max
	uint16_t negPeakelement/*[64]*/= 0;										// 64 Samples
	uint16_t posPeakelement = 0;
	uint16_t posPeak[18] = {NULL};
	uint16_t negPeak = 0;
	uint64_t runner = 0;													// runner, just counts up by Nr_of_samples to 2^64
	int sigCount = 0;
	
	xEventGroupWaitBits(evDMAState, DMADECREADY, false, true, portMAX_DELAY);
	for(;;) {
		while(uxQueueMessagesWaiting(decoderQueue) > 0) {
			if(xQueueReceive(decoderQueue, &bufferelement[0], portMAX_DELAY) == pdTRUE) {
				for(int a = 0; a < NR_OF_SAMPLES; a++) 	{
					runner++;
					if (a == 0) {
						negPeakelement = 10000;
						posPeakelement = 0;
						//posPeak = 0;
						//negPeak = 0;
					}
					if (bufferelement[a] > posPeakelement) {
						posPeakelement = bufferelement[a];
						posPeak[sigCount] = runner;													// to save the peakround
					}
						if (bufferelement[a] <= 0.90 * posPeakelement ) {												
							posPeakelement = 0;
							sigCount++;																// to save the 18Bit needed for peak
							if (sigCount >=18) {
								sigCount = 0;
							}
						}
					//}
					if (bufferelement[a] < negPeakelement) {
  						negPeakelement = bufferelement[a];
  						negPeak = a;
  					}
				}
				//vTaskDelay(10);
				//Decode Buffer
			}
		}		
		vTaskDelay( 2 / portTICK_RATE_MS );
	}
}

/*
speicher[0] = bufferelement[0];
speicher[1] = speicher[0];
speicher[2] = speicher[1];
speicher[3] = speicher[2];
if(speicher[0] > (AD-WERT/1./) {	// Störungen filtern 
	if(speicher[0] > speicher[3]) {	// steigend erkennen und dass langsam interessant wird		// was passiert bei 0 -> direkt peak dann fallend? Geht nicht
						// else if(wenn direkt auf nähe maximum und dann (fallende Flanke?)
		if (bufferelement[a] > posPeakelement) {
			posPeakelement = bufferelement[a];										// peakwert speichern
			posPeak[sigCount] = runner;												// durchlaufzählerwert speichern (fehleranfällig, da mit Task = Zeitproblematik)
						// speichern addresse in Array								// von 32 möglichen Plätzen an x/32
		}
						// fallende Flanke erkennen, posPeak[sigCount] +1 im array für nächsten Peak
	}
}
*/

/*
posPeak[x+1] - posPeak[x] = Abstand => innerhalb +- 8 feldern?
*/

void fillDecoderQueue(uint16_t buffer[NR_OF_SAMPLES])
{
	BaseType_t xTaskWokenByReceive = pdFALSE;
	xQueueSendFromISR( decoderQueue, &buffer[0], &xTaskWokenByReceive );
}

ISR(DMA_CH2_vect)
{
	DMA.CH2.CTRLB|=0x10;
	fillDecoderQueue( &adcBuffer0[0] );
}

ISR(DMA_CH3_vect)
{
	DMA.CH3.CTRLB |= 0x10;
	fillDecoderQueue( &adcBuffer1[0] );
}