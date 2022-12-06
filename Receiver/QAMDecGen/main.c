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
void dataPointer(int mode, int adressNr, uint16_t *data);

TaskHandle_t handler;

uint16_t array1[28][32] = {NULL};	// zwischenlösung für 22 Waves und je 32Werte für weiterbearbeitung; sollte durch den Queue gefüllt werden
uint16_t array2[28] = {NULL};		// den arrayplatz speichern an welcher Stelle der Peak ist für reveserse engineering welcher Bitwert

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
	xTaskCreate(vGetData, NULL, configMINIMAL_STACK_SIZE + 250, NULL, 1, NULL);


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
	for (int a = 0; a <= 27; a++) {													// for 22 waves with data
		for (int b = 0; b <= 31; b++) {												// for 32 samples per wave
			if(array1[a][b] > actualPeak) {											// Finden vom Höchstwert der Welle das jeweils nur bei dem Höchstwert der steigenden Welle
				actualPeak = array1[a][b];											// übergabe vom neuen Höchstwert
				array2[a] = b;														// Position vom Höchstwert der welle wird gespeichert
			}
			actualPeak = 0;															// Für nächste Runde wieder auf 0 damit wieder hochgearbeitet werden kann
		}
		vTaskDelay(100/ portTICK_RATE_MS );
	}
	// Mutex Ende
	// vTaskSuspend;																// Damit keine Resourcen besetzt wenn nicht nötig
}

void GetDifference( void *pvParameters ) {											// Task bestimmt die Zeit zwischen den höchstwerten der Wellen die im array2 gespeichert sind. 1 Messpunkt alle 31.25 uS.
	uint32_t TimeTable[32] = [3125,6250,9375,12500,15625,18750,21875,25000,		// TimeTable wo die MessPunkte in 10^-8 Sekunden hinterlegt sind.
							  28125,31250,34375,37500,40625,43750,46875,50000,
							  53125,56250,59375,62500,65625,68750,71875,75000,
							  78125,81250,84375,87500,90625,93750,96875,100000];		// Wir warscheindlich nicht benötigt 06.12.2022 PS
	uint32_t HoechstwertPos1 = 0;													// Variable für aktuelle Position vom Höchstwert
	uint32_t HoechstwertPos2 = 0;													// Variabel für nächste Position Höchstwert	
	uint32_t DifferenzPos = 0;
	uint8_t  WellenWert[28] = {NULL};												// Empfangen Daten in einem Array. Zugeortnet mit dem Wert 00 01 10 11 pro welle
	
	for(int i = 0; i <= 27; i ++){
		HoechstwertPos1 = TimeTable[array2[i]];
		HoechstwertPos2 = TimeTable[array2[i++]];
		DifferenzPos = HoechstwertPos2-HoechstwertPos1
		if(i<=3){
			if(DifferenzPos = 100000){
				WellenWert[i++] = 0;
			}
			if(DifferenzPos = 175000){
				WellenWert[i++] = 2;
			}
			if(DifferenzPos = 150000){
				WellenWert[i++] = 3;
			}
			if(DifferenzPos = 125000){
				WellenWert[i++] = 1;
			}
		}
		else{
			if(WellenWert[0] == 0){
				if(DifferenzPos = 100000){
					WellenWert[i++] = 0;
				}
				if(DifferenzPos = 175000){
					WellenWert[i++] = 2;
				}
				if(DifferenzPos = 150000){
					WellenWert[i++] = 3;
				}
				if(DifferenzPos = 125000){
					WellenWert[i++] = 1;
				}
			}
			if(WellenWert[0] == 1){
				if(DifferenzPos = 75000){
					WellenWert[i++] = 0;
				}
				if(DifferenzPos = 150000){
					WellenWert[i++] = 2;
				}
				if(DifferenzPos = 125000){
					WellenWert[i++] = 3;
				}
				if(DifferenzPos = 100000){
					WellenWert[i++] = 1;
				}
			}
			if(WellenWert[0] == 2){
				if(DifferenzPos = 25000){
					WellenWert[i++] = 0;
				}
				if(DifferenzPos = 100000){
					WellenWert[i++] = 2;
				}
				if(DifferenzPos = 75000){
					WellenWert[i++] = 3;
				}
				if(DifferenzPos = 50000){
					WellenWert[i++] = 1;
				}
			}
			if(WellenWert[0] == 3){
				if(DifferenzPos = 50000){
					WellenWert[i++] = 0;
				}
				if(DifferenzPos = 125000){
					WellenWert[i++] = 2;
				}
				if(DifferenzPos = 125000){
					WellenWert[i++] = 3;
				}
				if(DifferenzPos = 100000){
					WellenWert[i++] = 1;
				}
			}
		}
	}
	
	
	vTaskDelay( 100 / portTICK_RATE_MS );
	//vTaskSuspend;																	// Damit keine Resourcen besetzt wenn nicht nötig
}

void dataPointer(int mode, int adressNr, uint16_t *data) {			// write blockierter Bereich, reading erlaubt nicht blockierter bereich, umgekehrt warten, lesen weniger relevant
	static uint16_t data2bB[22][32];
	int adress = adressNr;
	//static 
	//uint16_t dataWrite[32] = { 0 };
	//uint16_t data[32] = dataIn[32];
	switch (mode) {
		case 0:				// case init
			adressNr = 0;
			//data = {NULL};
			break;
		case 1:				// write data to adress from vQamDec
			for (int a = 0; a >= 31; a++) {
				data2bB[adress][a] = &data + a;
			}
			return true;
			//break;
		case 2:				// read data in decoder
		
			return true;
			//break;
		case -1:				// wenn daten gelesen und verwertet (sofern nicht neue daten bereits geschrieben) kann ignoriert werden wenn nur lesen wenn fertig gefüllt
		
			break;
		default :
			break;
	} 
}

void vGetData( void *pvParameters ) {
	
	vTaskDelay( 10 / portTICK_RATE_MS );
}