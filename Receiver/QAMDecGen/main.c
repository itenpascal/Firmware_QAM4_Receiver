/*
 * QAMDecGen.c
 *
 * Created: 20.03.2018 18:32:07
 * Author : Martin Burger
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

#include "qaminit.h"
#include "qamgen.h"
#include "qamdec.h"


extern void vApplicationIdleHook( void );
void vLedBlink(void *pvParameters);
//void GetPeak( void *pvParameters );
//void GetDifference( void *pvParameters);
void vGetData( void *pvParameters);
//void dataPointer(int mode, int adressNr, uint16_t data[30][32]);
uint16_t *dataPointer(int mode, uint16_t data[15][32]);

TaskHandle_t handler;

uint16_t array1[30][32] = {NULL};	// zwischenlösung für 22 Waves und je 32Werte für weiterbearbeitung; sollte durch den Queue gefüllt werden
int array2[30] = {NULL};		// den arrayplatz speichern an welcher Stelle der Peak ist für reveserse engineering welcher Bitwert

void vApplicationIdleHook( void )
{	
	
}

int main(void)
{
	resetReason_t reason = getResetReason();

	vInitClock();
	vInitDisplay();
	
	initDAC();			// 2B commented out during real-testing, saving some space and further
	initDACTimer();		// 2B commented out during real-testing, saving some space and further
	initGenDMA();		// 2B commented out during real-testing, saving some space and further
	initADC();
	initADCTimer();
	initDecDMA();
	//dataPointer(0,0);
	
	xTaskCreate(vQuamGen, NULL, configMINIMAL_STACK_SIZE + 500, NULL, 2, NULL);		// 2B commented out during real-testing, saving some space and further
	xTaskCreate(vQuamDec, NULL, configMINIMAL_STACK_SIZE + 100, NULL, 1, NULL);
	//xTaskCreate(GetPeak, NULL, configMINIMAL_STACK_SIZE + 250, NULL, 1, NULL);
	//xTaskCreate(GetDifference, NULL, configMINIMAL_STACK_SIZE + 250, NULL, 1, NULL);
	xTaskCreate(vGetData, NULL, configMINIMAL_STACK_SIZE + 250, NULL, 1, NULL);		// https://www.mikrocontroller.net/topic/1198


	vDisplayClear();
	vDisplayWriteStringAtPos(0,0,"FreeRTOS 10.0.1");
	vDisplayWriteStringAtPos(1,0,"EDUBoard 1.0");
	vDisplayWriteStringAtPos(2,0,"QAMDECGEN-Base");
	vDisplayWriteStringAtPos(3,0,"ResetReason: %d", reason);
	vTaskStartScheduler();
	return 0;
}

void GetPeak( void *pvParameters ) {												// Peaks aus dem Array mit allen 22 Wellen lesen und diese in einem Weiteren Array abspeichern
	uint16_t actualPeak = 0;														// Zwischenspeicher des höchsten Werts
	// How to get pointeradress[0] from different Task?
	// Mutex damit diese Daten gesperrt sind während Bearbeitung
	for (int a = 0; a >= 27; a++) {			// for 22 waves with data
		for (int b = 0; b >= 31; b++) {		// for 32 samples per wave
			if(array1[a][b] > actualPeak) {
				actualPeak = array1[a][b];
				array2[a] = b;
			}
			actualPeak = 0;															// Für nächste Runde wider auf 0 damit wieder hochgearbeitet werden kann
		}
		vTaskDelay(100/ portTICK_RATE_MS );
	}
	// Mutex Ende
	// vTaskSuspend;																// Damit keine Resourcen besetzt wenn nicht nötig
}

void GetDifference( void *pvParameters ) {
	
	vTaskDelay( 100 / portTICK_RATE_MS );
	//vTaskSuspend;																	// Damit keine Resourcen besetzt wenn nicht nötig
}

uint16_t *dataPointer(int mode, uint16_t data[15][32]) {
	static uint16_t speicher;
	switch (mode) {
		case 0:			// write 
			speicher = &data[15][32];
			return 0;
			//break;
		case 1:			// read 
			return speicher;
			//break;
		default:
			break;
	}	
}

// void dataPointer(int mode, int adressNr, uint16_t data[30][32]) {			// write blockierter Bereich, reading erlaubt nicht blockierter bereich, umgekehrt warten, lesen weniger relevant
// 	static uint16_t data2bB[30][32];
// 	int adress = adressNr;
// 	uint16_t data1 = &data[30][32];
// 	static int adressWrite = 0;
// 	if (mode = 1) {
// 		adressWrite = mode;	// speicher wo schreiben letzte Runde war
// 	}
// 	switch (mode) {
// 		case 0:				// case init
// 			adressNr = 0;
// 			//data = {NULL};
// 			break;
// 		case 1:				// write data to adress from vQamDec
// //  			for (int a = 0; a >= 31; a++) {
// //  				data2bB[adress][a] = data[adress][a];
// //  			}
// 			return true;
// 			//break;
// 		case 2:				// read data in decoder
// 			
// 
// 
// 			return true;
// 			//break;
// 		case -1:				// wenn daten gelesen und verwertet (sofern nicht neue daten bereits geschrieben) kann ignoriert werden wenn nur lesen wenn fertig gefüllt
// 
// 			break;
// 		default :
// 			break;
// 	}
// }

void vGetData( void *pvParameters ) {
	
	vTaskDelay( 10 / portTICK_RATE_MS );
}

// void dataPointer(int mode, int adressNr, uint16_t **data) {			// write blockierter Bereich, reading erlaubt nicht blockierter bereich, umgekehrt warten, lesen weniger relevant
// 	static uint16_t data2bB[28][32];
// 	int adress = adressNr;
// 	
// 	//static 
// 	//uint16_t dataWrite[32] = { 0 };
// 	//uint16_t data[32] = dataIn[32];
// 	switch (mode) {
// 		case 0:				// case init
// 			adressNr = 0;
// 			//data = {NULL};
// 			break;
// 		case 1:				// write data to adress from vQamDec
// 			//data2bB = **data;
// 			for (int a = 0; a >= 31; a++) {
// 				data2bB[adress][a] = data[adress][a];
// 			}
// 			return true;
// 			//break;
// 		case 2:				// read data in decoder
// 		
// 			return true;
// 			//break;
// 		case -1:				// wenn daten gelesen und verwertet (sofern nicht neue daten bereits geschrieben) kann ignoriert werden wenn nur lesen wenn fertig gefüllt
// 		
// 			break;
// 		default :
// 			break;
// 	} 
// }