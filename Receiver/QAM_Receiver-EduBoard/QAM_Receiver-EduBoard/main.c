/*
 * QAM_Receiver_EduBoard.c
 *
 * Created: 20.03.2018 18:32:07
 * Author : chaos
 */ 

//#include <avr/io.h>
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
#include "stack_macros.h"

#include "mem_check.h"

#include "init.h"
#include "utils.h"
#include "errorHandler.h"
#include "NHD0420Driver.h"

//extern void vApplicationIdleHook( void );							// dono, without doesn't work
void vApplicationIdleHook( void ){}									// dono, without doesn't work
void vDisplay(void *pvParameters);									// Display SPI
void vSPIData(void *pvParameters);
void vTask3(void *pvParameters);


// TaskHandle for controlling tasks
TaskHandle_t display;
TaskHandle_t data;
TaskHandle_t task3;
TaskHandle_t task4;
TaskHandle_t task5;

//EventGroup for ButtonEvents.
EventGroupHandle_t egEventBits = NULL;
#define event1	0x01				// 
#define event2	0x02				// 
#define event3	0x04				// 
#define event4	0x08				// 
#define event5	0x10				// 
#define event6	0x20				// 
#define event7	0x40				// 
#define event8	0x80				// 
#define BUTTON_ALL	0xFF



int main(void)
{
	vInitClock();
	vInitDisplay();
	
	PORTC.DIR = 0x0D;				// SPI Display; Port C 4,5,7 Output, 6 Input (CS(active Low), MOSI, MISO, Serial CK)
	PORTE.DIR = 0x0D;				// SPI receiver; Port E 4,5,7 Output, 6 Input
	PORTF.OUT |= 0x80;				// CS on HIGH, others LOW
	PORTF.OUT |= 0x80;				// CS on HIGH, others LOW
	
	xTaskCreate( vDisplay, (const char *) "ledBlink1", configMINIMAL_STACK_SIZE+10, NULL, 2, &display);
	xTaskCreate( vSPIData, (const char *) "ledBlink2", configMINIMAL_STACK_SIZE+10, NULL, 3, &data);
	xTaskCreate( vTask3, (const char *) "ledBlink3", configMINIMAL_STACK_SIZE+10, NULL, 1, &task3);
	
	vDisplayClear();
	vDisplayWriteStringAtPos(0,0,"FreeRTOS 10.0.1");
	vDisplayWriteStringAtPos(1,0,"EDUBoard 1.0");
	vDisplayWriteStringAtPos(2,0,"QAM4");
	vDisplayWriteStringAtPos(3,0,"ResetReason: %d", 0);
	vTaskStartScheduler();
	return 0;
}

void vDisplay(void *pvParameters) {
	// state machine Start > verbindung > Datensammeln
	typedef enum {start, connect, transfer} state_t;
		
	state_t currentState = start;
	for(;;) {
		
		switch (currentState) {
			case start:
			
				break;	
			case connect:
			
				break;
			case transfer:
			
				break;
			default:
			
				break;
		}
			
		vTaskDelay(100 / portTICK_RATE_MS);
	}
}

void vSPIData(void *pvParameters) {
	
	for(;;) {
		
		vTaskDelay(100 / portTICK_RATE_MS);
	}
}

void vTask3(void *pvParameters) {
	
	for(;;) {
		
		vTaskDelay(100 / portTICK_RATE_MS);
	}
}


/*
	if (xEventGroupGetBits(egEventBits) & RESET_SHORT) { }
	xEventGroupSetBits(egEventBits, ALGORITHMUS);
	xEventGroupClearBits(egEventBits, 0x03);
	xEventGroupClearBits(egEventBits, PI_COLLECT);
	xEventGroupWaitBits(egEventBits, PI_COLLECT, false, true, portMAX_DELAY);
	
	  	TickType_t xLastWakeTime;
	  	const TickType_t xFrequency = 10;
	  	xLastWakeTime = xTaskGetTickCount();
		for (;;) { vTaskDelayUntil(&xLastWakeTime, xFrequency); }
	
	start = xTaskGetTickCountFromISR();
	
	vTaskSuspend(zeit);
	vTaskResume(leibniz);	
	
	eTaskState state = eTaskGetState(leibniz);
	if(state == eSuspended) { }
*/