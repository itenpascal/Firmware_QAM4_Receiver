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
#define STARTMEAS		0x02						// Start Measure, idletotpunkt �berschritten, start Daten speicherung f�r 22*32bit
#define BLOCKED			0x04						// 

extern uint16_t array[NR_OF_ARRAY_WHOLE];
//extern uint16_t array2;
extern uint16_t speicherWrite;

void vQuamDec(void* pvParameters)
{
	egEventBits = xEventGroupCreate();
	( void ) pvParameters;
	decoderQueue = xQueueCreate( 4, NR_OF_SAMPLES * sizeof(int16_t) );
	
	while(evDMAState == NULL) {
		vTaskDelay(3/portTICK_RATE_MS);
	}
	
	xEventGroupClearBits(egEventBits,RISEEDGE);
	uint16_t bufferelement[NR_OF_SAMPLES];														// 32 Samples max
	uint16_t speicher[4] = {10000, 10000, 10000, 10000};										// speicher f�r peakfinder initialisierung
	uint16_t adWert = 2200;																		// maxwert TBD
	static int speicher_1D = 0;
	unsigned int a = 0;
	unsigned int r = 0;					// für 32er Loop
	
	xEventGroupWaitBits(evDMAState, DMADECREADY, false, true, portMAX_DELAY);
	for(;;) {
		while(uxQueueMessagesWaiting(decoderQueue) > 0) {
			if(xQueueReceive(decoderQueue, &bufferelement[0], portMAX_DELAY) == pdTRUE) {
				if (xEventGroupGetBits(egEventBits) & RISEEDGE) {								// wenn bit NICHT aktiv
					
				} else {					
					for(; r <= NR_OF_ARRAY_2D - 1; r++) {										// alle bufferelemente prüfen 
						if (bufferelement[r] > (adWert/1.8)) {									// wenn bufferelement ausserhalb idle Bereich
							speicher[3] = speicher[2];
							speicher[2] = speicher[1];
							speicher[1] = speicher[0];
							speicher[0] = bufferelement[r];
							if (bufferelement[r] > speicher[3]) {								// Steigende Flanke erkannt
								xEventGroupSetBits(egEventBits,RISEEDGE);						// Anfangen Werte zu speichern f�r 28*32Werte
								r = 0;
								break;
							}
						} else {
							speicher[0] = 10000;
						}
					}
					r = 0;	
				}
				if (xEventGroupGetBits(egEventBits) & RISEEDGE) {								// Freigabe wenn oben erf�llt
 					while (a <= 4*NR_OF_ARRAY_WHOLE) {											// 28*32 = 896, mit 1'000+ genug Spiel wenn langsamer und Readfunktion mit >32 auch
 						array[a % NR_OF_ARRAY_WHOLE] = bufferelement[a % NR_OF_ARRAY_2D];		// speichern aktueller Wert
 						speicherWrite = a;														// abgeschlossener Schreibzyklus speicher f�r readTask
						a++;																	// raufzählen, vor nächstem if, damit richtig geteilt wird
						if ((a % NR_OF_ARRAY_2D) == 0) {
							break;
						}
 					}
					if(a >= 4*NR_OF_ARRAY_WHOLE) {												// Speicher wieder zur�cksetzen
						speicher[0] = 10000;														
						speicher[1] = 10000;
						speicher[2] = 10000;
						speicher[3] = 10000;
						//speicherWrite = 0;
						xEventGroupClearBits(egEventBits,RISEEDGE);								// wenn durchgelaufen, wieder R�cksetzten f�r n�chste Starterkennung
						a = 0;
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