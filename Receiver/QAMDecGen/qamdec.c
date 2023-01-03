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

// EventGroup for different Reasons
EventGroupHandle_t egEventBits = NULL;
#define RISEEDGE		0x01						// steigende Flanke erkannt
#define STARTMEAS		0x02						// Start Measure, idletotpunkt überschritten, start Daten speicherung für 22*32bit
#define BLOCKED			0x04						// 

extern uint16_t array[NR_OF_ARRAY_WHOLE];
//extern uint16_t array2;
extern uint16_t speicherWrite;

void vQuamDec(void* pvParameters)
{
	( void ) pvParameters;
	decoderQueue = xQueueCreate( 4, NR_OF_SAMPLES * sizeof(int16_t) );
	
	while(evDMAState == NULL) {
		vTaskDelay(3/portTICK_RATE_MS);
	}
	
	uint16_t bufferelement[NR_OF_SAMPLES];														// 32 Samples max
	int block = 0;
	uint64_t runner = 0;																		// runner, just counts up by Nr_of_samples to 2^64
	uint16_t speicher[4] = {10000, 10000, 10000, 10000};																	// speicher für peakfinder	
	uint16_t adWert = 2200;																		// maxwert TBD
	static int speicher_1D = 0;
	unsigned int a = 0;
	
	xEventGroupWaitBits(evDMAState, DMADECREADY, false, true, portMAX_DELAY);
	for(;;) {
		while(uxQueueMessagesWaiting(decoderQueue) > 0) {
			if(xQueueReceive(decoderQueue, &bufferelement[0], portMAX_DELAY) == pdTRUE) {
				speicher[3] = speicher[2];
				speicher[2] = speicher[1];
				speicher[1] = speicher[0];
				speicher[0] = bufferelement[0];
			//	speicher[1] = speicher[0];
			//	speicher[2] = speicher[1];
			//	speicher[3] = speicher[2];
				if (speicher[0] > (adWert/1.9)) {												// ausserhalb idle Bereich
					if (speicher[0] > speicher[3]) {											// Steigende Flanke erkannt
						xEventGroupSetBits(egEventBits,RISEEDGE);								// Anfangen Werte zu speichern für 28*32Werte
					}
				}
//  				speicher[1] = speicher[0];
// 					speicher[2] = speicher[1];
//  				speicher[3] = speicher[2];
				
				if (xEventGroupGetBits(egEventBits) & RISEEDGE) {								// Freigabe wenn oben erfüllt
/*
// 					for (a = 0; a <= 4*NR_OF_ARRAY_WHOLE; a++) {								// 28*32 = 896, mit 1'000+ genug Spiel wenn langsamer und Readfunktion mit >32 auch
// 						array[a % NR_OF_ARRAY_WHOLE] = bufferelement[a % NR_OF_ARRAY_2D];		// speichern aktueller Wert
// 						speicherWrite = a;														// abgeschlossener Schreibzyklus speicher für readTask
// 					}	
// 					xEventGroupClearBits(egEventBits,RISEEDGE);									// wenn durchgelaufen, wieder Rücksetzten für nächste Starterkennung
*/
					array[a % NR_OF_ARRAY_WHOLE] = bufferelement[0];							// Wert abspeichern von queue auf Platz [0]
					speicherWrite = a;															// abgeschlossener Schreibzyklus speicher für readTask
					a++;
					
					if(a >= 4*NR_OF_ARRAY_WHOLE) {
						a = 0;
						speicher[0] = 0;														// Speicher wieder zurücksetzen
						speicher[1] = 0;
						speicher[2] = 0;
						speicher[3] = 0;
						speicherWrite = 0;
						xEventGroupClearBits(egEventBits,RISEEDGE);								// wenn durchgelaufen, wieder Rücksetzten für nächste Starterkennung
					}
				}
				
			}
		}		
		vTaskDelay( 2 / portTICK_RATE_MS );
	}
}

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