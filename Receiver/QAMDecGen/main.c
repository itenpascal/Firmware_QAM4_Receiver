/*
 * QAMDecGen.c
 *
 * Created: 20.03.2018 18:32:07
 * Author : Martin Burger, Philipp, Pascal
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
#include "semphr.h"

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
void vGetPeak( void *pvParameters );
void GetDifference( void *pvParameters);
void vCalcData( void *pvParameters);
void vDisplay( void *pvParameters);
uint16_t *dataPointer(int mode, int speicher_1D, uint16_t data[NR_OF_ARRAY_2D]);
float dataTemp (int mode, float temp);

TaskHandle_t handler;
TaskHandle_t calculator;

uint16_t array[NR_OF_ARRAY_WHOLE] = {0};			// 256 Speicherplätze; darf nicht static sein (?) Fehlereldung; in qamdec.h & qam.dec.c als extern
static uint16_t array2[NR_OF_ARRAY_1D] = {0};		// den arrayplatz speichern an welcher Stelle der Peak ist f�r reveserse engineering welcher Bitwert
uint16_t speicherWrite = 0;
static char  WellenWert[56] = {NULL};						// Empfangen Daten in einem Array. Zugeortnet mit dem Wert 00 / 01 / 10 / 11 pro welle bei Difference Rechnung
	
// EventGroup for different Reasons
EventGroupHandle_t egEventsBits = NULL;
#define newDataBit		0x01				// steigende Flanke erkannt
#define dataBlockReady	0x02				// Wenn alle 28 Plätze gefüllt sind Daten abholen zur Verarbeitung
#define binaryReady		0x04				// Wenn alle 56speicherbitsgespeichert wurden

#define LOCK_WRITER 1
#define myLock 1

typedef struct RWLockManagement {
	SemaphoreHandle_t groupSeparator;
	signed long currentReaderCounter;
	volatile unsigned long readerSpinLock;
}RWLockManagement_t;

unsigned long CreateRWLock(RWLockManagement_t *Lock) {
	unsigned long a_Result = 0;
	Lock->groupSeparator = xSemaphoreCreateBinary();
	if (Lock->groupSeparator) {
		Lock->currentReaderCounter = 0;
		Lock->readerSpinLock = 0;
		if (xSemaphoreGive(Lock->groupSeparator) == pdTRUE) {
			a_Result = 1;
		}
	}
}

unsigned short incrementReader(RWLockManagement_t * Lock) {
	if(Lock->currentReaderCounter++ == 0) {
		return 1;
	}
	return 0;
}

unsigned short decrementReader(RWLockManagement_t * Lock) {
	if(--Lock->currentReaderCounter == 0) {
		return 1;
	}
	return 0;
}

void claimRWLock(RWLockManagement_t * Lock, unsigned short Mode) {
	if(Mode == LOCK_WRITER) {
		xSemaphoreTake(Lock->groupSeparator, portMAX_DELAY);
		} else {
		if(incrementReader(Lock)) {
			xSemaphoreTake(Lock->groupSeparator, portMAX_DELAY);
			Lock->readerSpinLock = 1;
		}
		while(!Lock->readerSpinLock) {
			vTaskDelay(2);
		}
	}
}

void releaseRWLock(RWLockManagement_t * Lock, unsigned short Mode) {
	if(Mode == LOCK_WRITER) {
		xSemaphoreGive(Lock->groupSeparator);
		} else {
		if(decrementReader(Lock)) {
			Lock->readerSpinLock = 0;
			xSemaphoreGive(Lock->groupSeparator);
		}
	}
}

void vApplicationIdleHook( void )
{	
	
}

int main(void)
{
	resetReason_t reason = getResetReason();

	vInitClock();
	vInitDisplay();
	
	//initDAC();			// 2B commented out during real-testing, saving some space and further
	//initDACTimer();		// 2B commented out during real-testing, saving some space and further
	//initGenDMA();		// 2B commented out during real-testing, saving some space and further
	initADC();
	initADCTimer();
	initDecDMA();
	//dataPointer(0,0);
	egEventsBits = xEventGroupCreate();
	
	//xTaskCreate(vQuamGen, NULL, configMINIMAL_STACK_SIZE, NULL, 1, NULL);			// 2B commented out during real-testing, saving some space and further
	xTaskCreate(vQuamDec, NULL, configMINIMAL_STACK_SIZE + 500, NULL, 1, NULL);
	xTaskCreate(vGetPeak, NULL, configMINIMAL_STACK_SIZE + 200, NULL, 2, NULL);
	xTaskCreate(GetDifference, NULL, configMINIMAL_STACK_SIZE + 400, NULL, 2, NULL);
	xTaskCreate(vCalcData, NULL, configMINIMAL_STACK_SIZE + 200, NULL, 2, calculator);
	xTaskCreate(vDisplay, NULL, configMINIMAL_STACK_SIZE + 100, NULL, 3, NULL);		// https://www.mikrocontroller.net/topic/1198


	vDisplayClear();
	//vDisplayWriteStringAtPos(0,0,"FreeRTOS 10.0.1");
	//vDisplayWriteStringAtPos(1,0,"EDUBoard 1.0");
	//vDisplayWriteStringAtPos(2,0,"QAMDECGEN-Base");
	//vDisplayWriteStringAtPos(3,0,"ResetReason: %d", reason);
	vTaskStartScheduler();
	return 0;
}

void vGetPeak( void *pvParameters ) {
	//int c;													// Peaks aus dem Array mit allen 28 Wellen lesen und diese in einem Weiteren Array abspeichern
	static int speicherRead_1D = 0;
	//uint16_t speicherPointer[NR_OF_ARRAY_1D] = {0};									// Zeigt die aktuelle Position wo geschrieben wird
	uint16_t counterWaveLenghtstart = 0;												// definiert ab wo die Welle beginnt 
	uint16_t counterWaveLenghtEnd = 0;													// definiert bis wo gelesen wird wenn Ende
	int c = 0;																			// zähler für den addresspointer im Array 2
	uint16_t actualPeak = 0;															// Zwischenspeicher des höchsten Werts
	for (;;) {
		if ((speicherWrite - counterWaveLenghtEnd) > 32){
			counterWaveLenghtEnd = speicherWrite;
			for (int b = 0; b <= ((int)speicherWrite/counterWaveLenghtstart); b++) {
				for (int a = counterWaveLenghtstart; a < counterWaveLenghtstart + 32; a++) {	// für 32 Werte pro welle höchstwert ermitteln
					if(array[a%256] > actualPeak) {												// Finden vom Höchstwert der Welle das jeweils nur bei dem H�chstwert der steigenden Welle
						actualPeak = array[a%256];												// Übergabe vom neuen Höchstwert
						array2[c] = a%32;														// Position vom Höchstwert der welle wird gespeichert
					}
				}
				c++;	
				counterWaveLenghtstart = counterWaveLenghtstart + 32;
				if (counterWaveLenghtstart >= 4*NR_OF_ARRAY_WHOLE) {
					counterWaveLenghtstart = 0;
				}
 				if (c >= NR_OF_ARRAY_1D) {
					c = 0;
					counterWaveLenghtstart = 0;
					xEventGroupSetBits(egEventsBits,dataBlockReady);
				}
				actualPeak = 0;																// Für nächste Runde wieder auf 0 damit wieder hochgearbeitet werden kann
			}	
	}
		vTaskDelay(2/ portTICK_RATE_MS );
	}
	// vTaskSuspend;																	// Damit keine Resourcen besetzt wenn nicht nötig
}

void GetDifference( void *pvParameters ) {												// Task bestimmt die Zeit zwischen den höchstwerten der Wellen die im array2 gespeichert sind. 1 Messpunkt alle 31.25 uS.	
	uint32_t TimeTable[32] = {3125,6250,9375,12500,15625,18750,21875,25000,				// TimeTable wo die MessPunkte in 10^-8 Sekunden hinterlegt sind.
							  28125,31250,34375,37500,40625,43750,46875,50000,
							  53125,56250,59375,62500,65625,68750,71875,75000,
							  78125,81250,84375,87500,90625,93750,96875,100000};		// Wir warscheindlich nicht benötigt 06.12.2022 PS
	uint32_t HoechstwertPos1 = 0;														// Variable för aktuelle Position vom Höchstwert
	uint32_t HoechstwertPos2 = 0;														// Variabel för nöchste Position Höchstwert	
	int32_t DifferenzPos = 0;
	int a = 0;
	//char  WellenWert[56] = {NULL};													// Empfangen Daten in einem Array. Zugeortnet mit dem Wert 00 / 01 / 10 / 11 pro welle
	uint8_t lastValue = 00;
	for(;;) {
		for(int i = 0; i <= 27; i ++){
			xEventGroupWaitBits(egEventsBits, dataBlockReady, false, true, portMAX_DELAY);	// warten bis Signalbit gesetzt
			HoechstwertPos1 = TimeTable[array2[i]];
			a = i+1;
			HoechstwertPos2 = TimeTable[array2[a]];
			DifferenzPos = HoechstwertPos2+(100000-HoechstwertPos1);
			
			if(i <= 3 | lastValue == 00) {	// 0									// wenn kleiner 3, Synchronisation, danach normal was vorher (Erster durchlauf 75k ?)
				if(DifferenzPos >= 85000 & DifferenzPos <=110000){					// 100k		0
					//strcat( WellenWert , "00");	
					WellenWert[2*i] = '0'; 
					WellenWert[2*i+1] = '0';
					lastValue = 00;
					continue;														// direkt zum Schleifanfang wieder => Durchlaufszeit sparen
				}
				if(DifferenzPos >= 165000 & DifferenzPos <=185000){					// 175k		2
					//strcat( WellenWert , "10");
					WellenWert[2*i] = '1'; 
				 	WellenWert[2*i+1] = '0';
					lastValue = 10;
					continue;
				}
				if(DifferenzPos >= 140000 & DifferenzPos <=160000){					// 150k		3
					//strcat(WellenWert , "11");
					WellenWert[2*i] = '1'; 
					WellenWert[2*i+1] = '1';
					lastValue = 11;
					continue;
				}
				if(DifferenzPos >= 115000 & DifferenzPos <=135000){					// 125k		1
					//strcat(WellenWert , "01");
					WellenWert[2*i] = '0'; 
					WellenWert[2*i+1] = '1';
					lastValue = 01;
					continue;
				}
			}
			if(lastValue == 01){		// 1
				if(DifferenzPos >= 65000 & DifferenzPos <= 85000){					// 75k		0
					//strcat( WellenWert , "00");
					WellenWert[2*i] = '0'; 
					WellenWert[2*i+1] = '0';	
					lastValue = 00;
					continue;
				}
				if(DifferenzPos >= 140000 & DifferenzPos <=160000){					// 150k		2
					//strcat( WellenWert , "10");
					WellenWert[2*i] = '1'; 
					WellenWert[2*i+1] = '0';
					lastValue = 10;
					continue;
				}
				if(DifferenzPos >= 115000 & DifferenzPos <=135000){					// 125k		3
					//strcat(WellenWert , "11");
					WellenWert[2*i] = '1'; 
					WellenWert[2*i+1] = '1';
					lastValue = 11;
					continue;
				}
				if(DifferenzPos >= 85000 & DifferenzPos <=110000){					// 100k		1
					//strcat(WellenWert , "01");
					WellenWert[2*i] = '0'; 
					WellenWert[2*i+1] = '1';
					lastValue = 01;
					continue;
				}
			}
			if(lastValue == 10){		// 2
				if(DifferenzPos >= 15000 & DifferenzPos <=35000){					// 25k		0
					//strcat( WellenWert , "00");	
					WellenWert[2*i] = '0'; 
					WellenWert[2*i+1] = '0';
					lastValue = 00;
					continue;
				}
				if(DifferenzPos >= 85000 & DifferenzPos <=110000){					// 100k		2
					//strcat( WellenWert , "10");
					WellenWert[2*i] = '1'; 
					WellenWert[2*i+1] = '0';
					lastValue = 10;
					continue;
				}
				if(DifferenzPos >= 65000 & DifferenzPos <=80000){					// 75k		3
					//strcat(WellenWert , "11");
					WellenWert[2*i] = '1'; 
					WellenWert[2*i+1] = '1';
					lastValue = 11;
					continue;
				}
				if(DifferenzPos >= 40000 & DifferenzPos <=60000){					// 50k		1
					//strcat(WellenWert , "01");
					WellenWert[2*i] = '0'; 
					WellenWert[2*i+1] = '1';
					lastValue = 01;
					continue;
				}
			}
			if(lastValue == 11){		//	3
				if(DifferenzPos >= 40000 & DifferenzPos <=60000){					// 50k		0
					//strcat( WellenWert , "00");	
					WellenWert[2*i] = '0'; 
					WellenWert[2*i+1] = '0';
					lastValue = 00;
					continue;
				}
				if(DifferenzPos >= 140000 & DifferenzPos <=160000){					// 150k		2	
					//strcat( WellenWert , "10");
					WellenWert[2*i] = '1'; 
					WellenWert[2*i+1] = '0';
					lastValue = 10;
					continue;
				}
				if(DifferenzPos >= 115000 & DifferenzPos <=135000){					// 125k		3	
					//strcat(WellenWert , "11");
					WellenWert[2*i] = '1'; 
					WellenWert[2*i+1] = '1';
					lastValue = 11;
					continue;
				}
				if(DifferenzPos >= 85000 & DifferenzPos <=110000){					// 100k		1
					//strcat(WellenWert , "01");	
					WellenWert[2*i] = '0'; 
					WellenWert[2*i+1] = '1';
					lastValue = 01;
					continue;
				}
			}
		}
		xEventGroupSetBits(egEventsBits,binaryReady);
		xEventGroupClearBits(egEventsBits,dataBlockReady); 
	//	a = 0;
		vTaskDelay( 2 / portTICK_RATE_MS );
		//vTaskSuspend;																	// Damit keine Resourcen besetzt wenn nicht n�tig
	}
}

void vCalcData( void *pvParameters ) {													// Nützliche Daten aus dem Array ziehen, wie Temp
//	uint8_t arraySynch[4] = {0};														// arraydaten von Global abspeichern
//	uint8_t arrayLenght[4] = {0};														//  
//	uint8_t arrayCRC[4] = {0};															// Checksumme
	float temp = 0;	
	for(;;) {
		xEventGroupWaitBits(egEventsBits, binaryReady, false, true, portMAX_DELAY);		// warten bis Signalbit gesetzt
		//binaerDifference = WellenWert;
 		for (int r = 7; r >= 0; r--) {													// für Länge
// 			// arraySynch[3 - r] = array[r] von vGetDifference mit 0-3;					// 
 			
 		}
		for (int r = 15; r >= 8; r--) {													// für Synchronisation
			// arrayLenght[3 - r] = array[r + 4] von vGetDifference mit 0-3;			// 
			
		}		
		for (int r = 47; r >= 16; r--) {												// Verkehrte Reihenfolge gesendet, heisst hier wird es wieder richtiggestellt
			// arrayDifference[15 - r] = array[r + 8] von vGetDifference mit 0-3;		// 
 			if(WellenWert[r] == 49) {													// Binär Zähler alle 1nsen (Ascii 48 = 0, 49 = 1)
 				temp = temp + pow(2,r -16);												// 
 			}
		}
		for (int r = 55; r >= 48; r--) {												// für Checksumme
			// arrayCRC[3 - r] = array[r + 24] von vGetDifference mit 0-3;				// 
			
		}
		
	//	claimRWLock(myLock, LOCK_WRITER);												// sperren des Zugriffs auf diese Daten
		dataTemp(0, temp/1000);															// 0 = Schreiber, temp = Daten
	//	releaseRWLock(myLock, LOCK_WRITER);												// freigeben des Zugriffs auf die Daten
		xEventGroupClearBits(egEventsBits,binaryReady);									// Rücksetzen der Signalbits
		temp = 0;
		strcpy(WellenWert, "                                                       ");	// Array wieder leeren (alle 49er sicher streichen)
		vTaskDelay(2);
	}
} 

float dataTemp (int mode, float temp) {													// Übergabe des berechneten Werts von vCalcData zu VDisplay ohne Globale Variable mit möglichkeit Zugriff zu sperren
	static float temperature;
	switch (mode) {
	case 0:					// schreiben
		temperature = temp;
		break;
	case 1:					// lesen
		return temperature;
		//break; 
	}
}
			
void vDisplay( void *pvParameters ) {													// von Binär zu Temp rechnen
	bool running = false;																		// TBD löschen wenn temp erfolgreich übergeben möglich
	double temp = 0;
	bool halfSec = 0;																	// Datenabholen ale .5 Sekunden, darstellen jede Sekunde 
	for(;;) {
	//	claimRWLock(myLock, LOCK_WRITER);												// sperren des Zugriffs auf diese Daten
		//temp = round(dataTemp(1,0)) + 0.005; // 0.0005								// 1 = Leser, 0 keine Daten; 0.005 für Aufrunden der Daten ansonsten x.99..
		temp = dataTemp(1,0);															// 1 = Leser, 0 keine Daten; 
	//	releaseRWLock(myLock, LOCK_WRITER);												// freigeben des Zugriffs auf die Daten
		if (halfSec) {
			vDisplayClear();
			vDisplayWriteStringAtPos(0,3,"QAM4 - Projekt");
			vDisplayWriteStringAtPos(1,6,"TSE 2009");
			if(running) {vDisplayWriteStringAtPos(2,0,"--------------------");}
			vDisplayWriteStringAtPos(3,0,"Temperatur: %f", temp);
			halfSec = false;
		} else {
			halfSec = true;
			running = !running;															// Sichtbarkeit das uC noch läuft auch wenn sich keine Zahl verändert
		}
		vTaskDelay( 500 / portTICK_RATE_MS );
	}
}
