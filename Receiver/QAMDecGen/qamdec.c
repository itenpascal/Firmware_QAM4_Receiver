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
#define RISEEDGE		0x01				// steigende Flanke erkannt
#define STARTMEAS		0x02				// Start Measure, idletotpunkt überschritten, start Daten speicherung für 22*32bit
#define BLOCKED			0x04				// 

void vQuamDec(void* pvParameters)
{
	( void ) pvParameters;
	
	decoderQueue = xQueueCreate( 4, NR_OF_SAMPLES * sizeof(int16_t) );
	
	while(evDMAState == NULL) {
		vTaskDelay(3/portTICK_RATE_MS);
	}
	
	uint16_t bufferelement[NR_OF_SAMPLES];									// 32 Samples max
	int block = 0;
	uint64_t runner = 0;													// runner, just counts up by Nr_of_samples to 2^64
	uint16_t speicher = 0;											// speicher für peakfinder
	static uint16_t speicher1[15][32] = {0};	
	static uint16_t speicher2[15][32] = {0};
	uint16_t adWert = 2200;													// maxwert TBD
	static int sigCount = 0;
	
	//dataPointer(0, sigCount, speicher2[30][32]);
	
	xEventGroupWaitBits(evDMAState, DMADECREADY, false, true, portMAX_DELAY);
	for(;;) {
		while(uxQueueMessagesWaiting(decoderQueue) > 0) {
			if(xQueueReceive(decoderQueue, &bufferelement[0], portMAX_DELAY) == pdTRUE) {
				speicher = bufferelement[0];
				if(speicher >= (adWert/1.8)) {									// Störungen im Idle filtern; bei Idle in ca 1/2 MaxSpannung
						xEventGroupSetBits(egEventBits, STARTMEAS);				// Erkennung Steigende Flanke
				}
				if (xEventGroupGetBits(egEventBits) & STARTMEAS) {
					for (int b = 0; b <= 31; b++) {
						if (b <= 15) {
							speicher2[sigCount][b] = &bufferelement[b];
						} else {
							speicher2[sigCount][b] = &bufferelement[b];
						}
					}
					if (sigCount = 15) {
						dataPointer(0, speicher1[15][32]);				// modus 0 = write data (halbe Daten, erster Block)
					} else if (sigCount >= 30) {
						dataPointer(0, speicher2[15][32]);				// modus 0 = write data (halbe Daten, zweiter Block)
						xEventGroupClearBits(egEventBits, STARTMEAS);	// Rücksetzen ausser Idle bit
					}
					sigCount++;											// Raufzählen für die Anzahl Wellen
				}
			}
		}		
		vTaskDelay( 10 / portTICK_RATE_MS );
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