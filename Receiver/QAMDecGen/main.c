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
	
// EventGroup for different Reasons
EventGroupHandle_t egEventsBits = NULL;
#define newDataBit		0x01				// steigende Flanke erkannt
#define dataBlockReady	0x02				// Wenn alle 28 Plätze gefüllt sind Daten abholen zur Verarbeitung
//#define BLOCKED		0x04				// 

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
	
	initDAC();			// 2B commented out during real-testing, saving some space and further
	initDACTimer();		// 2B commented out during real-testing, saving some space and further
	initGenDMA();		// 2B commented out during real-testing, saving some space and further
	initADC();
	initADCTimer();
	initDecDMA();
	//dataPointer(0,0);
	egEventsBits = xEventGroupCreate();
	
	xTaskCreate(vQuamGen, NULL, configMINIMAL_STACK_SIZE, NULL, 1, NULL);			// 2B commented out during real-testing, saving some space and further
	xTaskCreate(vQuamDec, NULL, configMINIMAL_STACK_SIZE + 500, NULL, 1, NULL);
	xTaskCreate(vGetPeak, NULL, configMINIMAL_STACK_SIZE + 200, NULL, 2, NULL);
	xTaskCreate(GetDifference, NULL, configMINIMAL_STACK_SIZE + 200, NULL, 2, NULL);
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
		if (speicherWrite - counterWaveLenghtEnd > 32){
			counterWaveLenghtEnd = speicherWrite;
			counterWaveLenghtstart = counterWaveLenghtEnd -32;
			for (int a = counterWaveLenghtstart; a < counterWaveLenghtEnd; a++) {		// für 32 Werte pro welle höchstwert ermitteln
				if(array[a] > actualPeak) {												// Finden vom Höchstwert der Welle das jeweils nur bei dem H�chstwert der steigenden Welle
					actualPeak = array[a];												// Übergabe vom neuen Höchstwert
					array2[c] = a%32;													// Position vom Höchstwert der welle wird gespeichert
				}
			}
			c++;
 			if (c >= NR_OF_ARRAY_1D) {
				c = 0;
			}
			actualPeak = 0;															// Für nächste Runde wieder auf 0 damit wieder hochgearbeitet werden kann
		} 
		speicherRead_1D++;
		if(speicherRead_1D >=28) {														// Rücksetzen  auf 0 wenn max + 1
			speicherRead_1D = 0;
			xEventGroupSetBits(egEventsBits,dataBlockReady);
		}
		vTaskDelay(4/ portTICK_RATE_MS );
	}
	// vTaskSuspend;																	// Damit keine Resourcen besetzt wenn nicht nötig
}

void GetDifference( void *pvParameters ) {											// Task bestimmt die Zeit zwischen den höchstwerten der Wellen die im array2 gespeichert sind. 1 Messpunkt alle 31.25 uS.	
	uint32_t TimeTable[32] = {3125,6250,9375,12500,15625,18750,21875,25000,			// TimeTable wo die MessPunkte in 10^-8 Sekunden hinterlegt sind.
							  28125,31250,34375,37500,40625,43750,46875,50000,
							  53125,56250,59375,62500,65625,68750,71875,75000,
							  78125,81250,84375,87500,90625,93750,96875,100000};	// Wir warscheindlich nicht benötigt 06.12.2022 PS
	uint32_t HoechstwertPos1 = 0;													// Variable för aktuelle Position vom Höchstwert
	uint32_t HoechstwertPos2 = 0;													// Variabel för nöchste Position Höchstwert	
	int32_t DifferenzPos = 0;
	int a = 0;
	uint8_t  WellenWert[28] = {NULL};												// Empfangen Daten in einem Array. Zugeortnet mit dem Wert 00 / 01 / 10 / 11 pro welle
	
	for(;;) {
		for(int i = 0; i <= 27; i ++){
			HoechstwertPos1 = TimeTable[array2[i]];
			a = i+1;
			HoechstwertPos2 = TimeTable[array2[a]];
			DifferenzPos = HoechstwertPos2+HoechstwertPos1;
			if(i<=3){
				if(DifferenzPos == 100000){											// Toleranzbereich? aktuell nur perfekte Werte möglich:
					WellenWert[a] = 0;											// Wie wird Synchonisiert mit den ersten peaks, aktuell wird von 0 Durchgang ausgegangen
				}
				if(DifferenzPos == 175000){	
					WellenWert[a] = 2;
				}
				if(DifferenzPos == 150000){
					WellenWert[a] = 3;
				}
				if(DifferenzPos == 125000){
					WellenWert[a] = 1;
				}
			}
			else{
				if(WellenWert[i] == 0){
					if(DifferenzPos == 100000){
						WellenWert[a] = 0;
					}
					if(DifferenzPos == 175000){
						WellenWert[a] = 2;
					}
					if(DifferenzPos == 150000){
						WellenWert[a] = 3;
					}
					if(DifferenzPos == 125000){
						WellenWert[a] = 1;
					}
				}
				if(WellenWert[i] == 1){
					if(DifferenzPos == 75000){
						WellenWert[a] = 0;
					}
					if(DifferenzPos == 150000){
						WellenWert[a] = 2;
					}
					if(DifferenzPos == 125000){
						WellenWert[a] = 3;
					}
					if(DifferenzPos == 100000){
						WellenWert[a] = 1;
					}
				}
				if(WellenWert[i] == 2){
					if(DifferenzPos == 25000){
						WellenWert[a] = 0;
					}
					if(DifferenzPos == 100000){
						WellenWert[a] = 2;
					}
					if(DifferenzPos == 75000){
						WellenWert[a] = 3;
					}
					if(DifferenzPos == 50000){
						WellenWert[a] = 1;
					}
				}
				if(WellenWert[i] == 3){
					if(DifferenzPos == 50000){
						WellenWert[a] = 0;
					}
					if(DifferenzPos == 125000){
						WellenWert[a] = 2;
					}
					if(DifferenzPos == 125000){
						WellenWert[a] = 3;
					}
					if(DifferenzPos == 100000){
						WellenWert[a] = 1;
					}
				}
			}
		}
	vTaskDelay( 50 / portTICK_RATE_MS );
	//vTaskSuspend;																	// Damit keine Resourcen besetzt wenn nicht n�tig
	}
}

void vCalcData( void *pvParameters ) {								// Nützliche Daten aus dem Array ziehen, wie Temp
//	uint16_t arraySynch[4] = {0};									// arraydaten von Global abspeichern
	uint16_t arrayLenght[4] = {0};									// 
	uint16_t arrayDifference[16] = {0};								// 
	uint16_t arrayCRC[4] = {0};										// Checksumme
	float temp = 0;	
	for(;;) {
		xEventGroupWaitBits(egEventsBits, dataBlockReady, false, true, portMAX_DELAY);	// warten bis Signalbit gesetzt
// 		for (int r; r <= 4; r++) {
// 			// arraySynch[3 - r] = array[r] von vGetDifference mit 0-3;						// reihenfolge umkehren (
// 			
// 		}
		for (int r; r <= 4; r++) {
			// arrayLenght[3 - r] = array[r + 4] von vGetDifference mit 0-3;						// reihenfolge umkehren (
			
		}
		for (int r; r <= 16; r++) {
			// arrayDifference[15 - r] = array[r + 8] von vGetDifference mit 0-3;						// reihenfolge umkehren (
			
		}
		for (int r; r <= 4; r++) {
			// arrayCRC[3 - r] = array[r + 24] von vGetDifference mit 0-3;						// reihenfolge umkehren (
			
		}
		
		/* 
		
		 Umrechnen der Daten
		 
		*/ 
		
	//	claimRWLock(myLock, LOCK_WRITER);						// sperren des Zugriffs auf diese Daten
		dataTemp(0, temp);										// 0 = schreiber, temp = Daten
	//	releaseRWLock(myLock, LOCK_WRITER);						// freigeben des Zugriffs auf die Daten
		xEventGroupClearBits(egEventsBits,dataBlockReady);		// Rücksetzen der Signalbits
		vTaskDelay(100);
	}
} 

float dataTemp (int mode, float temp) {
	static float temperature;
	switch (mode) {
	case 0:							// schreiben
		temperature = temp;
		break;
	case 1:							// lesen
		return temperature;
		//break; 
	}
}
			
void vDisplay( void *pvParameters ) {			// von Binär zu Temp rechnen
	int test = 0;								// TBD löschen wenn temp erfolgreich übergeben möglich
	float temp = 0;
	bool halfSec = 0;							// Datenabholen ale .5 Sekunden, darstellen jede Sekunde 
	for(;;) {
	//	claimRWLock(myLock, LOCK_WRITER);						// sperren des Zugriffs auf diese Daten
		temp = round(dataTemp(1,0)) + 0.005; // 0.0005			// 1 = leser, 0 keine Daten
	//	releaseRWLock(myLock, LOCK_WRITER);						// freigeben des Zugriffs auf die Daten
		if (halfSec) {
			vDisplayClear();
			vDisplayWriteStringAtPos(0,0,"QAM - Projekt");
			vDisplayWriteStringAtPos(1,0,"TSE 2009");
			vDisplayWriteStringAtPos(2,0,"");
			vDisplayWriteStringAtPos(3,0,"Temperatur: %d G C", test);
			halfSec = false;
		} else {
			halfSec = true;
		}
		test++;
		vTaskDelay( 500 / portTICK_RATE_MS );
	}
}
