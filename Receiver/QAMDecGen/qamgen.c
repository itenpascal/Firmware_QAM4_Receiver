/*
* qamgen.c
*
* Created: 05.05.2020 16:24:59
*  Author: Chaos
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

#include "qaminit.h"
#include "qamgen.h"


const int16_t sinLookup100[NR_OF_SAMPLES*2] = {0x0,0x18F,0x30F,0x471,0x5A7,0x6A6,0x763,0x7D8,
												0x7FF,0x7D8,0x763,0x6A6,0x5A7,0x471,0x30F,0x18F,
												0x0,0xFE71,0xFCF1,0xFB8F,0xFA59,0xF95A,0xF89D,0xF828,
												0xF801,0xF828,0xF89D,0xF95A,0xFA59,0xFB8F,0xFCF1,0xFE71,
												0x0,0x18F,0x30F,0x471,0x5A7,0x6A6,0x763,0x7D8,
												0x7FF,0x7D8,0x763,0x6A6,0x5A7,0x471,0x30F,0x18F,
												0x0,0xFE71,0xFCF1,0xFB8F,0xFA59,0xF95A,0xF89D,0xF828,
												0xF801,0xF828,0xF89D,0xF95A,0xFA59,0xFB8F,0xFCF1,0xFE71};
														
const int16_t sinLookup50[NR_OF_SAMPLES*2] = {0x0,0xC8,0x187,0x238,0x2D3,0x353,0x3B1,0x3EB,
												0x3FF,0x3EB,0x3B1,0x353,0x2D3,0x238,0x187,0xC8,
												0x0,0xFF38,0xFE79,0xFDC8,0xFD2D,0xFCAD,0xFC4F,0xFC15,
												0xFC01,0xFC15,0xFC4F,0xFCAD,0xFD2D,0xFDC8,0xFE79,0xFF38,
												0x0,0xC8,0x187,0x238,0x2D3,0x353,0x3B1,0x3EB,
												0x3FF,0x3EB,0x3B1,0x353,0x2D3,0x238,0x187,0xC8,
												0x0,0xFF38,0xFE79,0xFDC8,0xFD2D,0xFCAD,0xFC4F,0xFC15,
												0xFC01,0xFC15,0xFC4F,0xFCAD,0xFD2D,0xFDC8,0xFE79,0xFF38,};

#define SENDBUFFER_SIZE 20
uint8_t sendbuffer[SENDBUFFER_SIZE] = {0,1,0,1,0,1,2,1,3,0,1,1,3,2,1,0,0,1,0,1};

//uint8_t sendbuffer[100];
uint8_t sendID = 0;
void createSendData() {
	sendID++;
	char senddata[10] = "HelloWorld";
	uint8_t datalen = strlen(senddata);
	sendbuffer[0] = 1;
	sendbuffer[1] = 2;
	sendbuffer[2] = sendID & 0x03;
	sendbuffer[3] = (sendID >> 2) & 0x03;
	sendbuffer[4] = (datalen >> 0) & 0x03;
	sendbuffer[5] = (datalen >> 2) & 0x03;
	sendbuffer[6] = (datalen >> 4) & 0x03;
	sendbuffer[7] = (datalen >> 6) & 0x03;
	for(int i = 0; i < datalen;i++) {
		sendbuffer[8 + i*4 + 0] = (senddata[i] >> 0) & 0x03;
		sendbuffer[8 + i*4 + 1] = (senddata[i] >> 2) & 0x03;
		sendbuffer[8 + i*4 + 2] = (senddata[i] >> 4) & 0x03;
		sendbuffer[8 + i*4 + 3] = (senddata[i] >> 6) & 0x03;
	}
	uint8_t checksum = 0;
	for(int i = 0; i < 7 + (datalen * 4); i++) {
		checksum += sendbuffer[i];
	}
	sendbuffer[7 + (datalen * 4) + 0] = (checksum >> 0) & 0x03;
	sendbuffer[7 + (datalen * 4) + 1] = (checksum >> 2) & 0x03;
	sendbuffer[7 + (datalen * 4) + 2] = (checksum >> 4) & 0x03;
	sendbuffer[7 + (datalen * 4) + 3] = (checksum >> 6) & 0x03;
}

void vQuamGen(void *pvParameters) {
	while(evDMAState == NULL) {
		vTaskDelay(3/portTICK_RATE_MS);
	}
	xEventGroupWaitBits(evDMAState, DMAGENREADY, false, true, portMAX_DELAY);
	for(;;) {
		vTaskDelay(10/portTICK_RATE_MS);
	}
}

void fillBuffer(uint16_t buffer[NR_OF_SAMPLES]) {
	static int pSendbuffer = 0;
	
	for(int i = 0; i < NR_OF_SAMPLES;i++) {
		switch(sendbuffer[pSendbuffer]) {
			case 0:
				buffer[i] = 0x800 + (sinLookup100[i]);
			break;
			case 1:
				buffer[i] = 0x800 + (sinLookup100[i+8]);
			break;
			case 2:
				buffer[i] = 0x800 + (sinLookup100[i+24]);
			break;
			case 3:
				buffer[i] = 0x800 + (sinLookup100[i+16]);
			break;
		}
	}
	if(pSendbuffer < SENDBUFFER_SIZE-1) {
		pSendbuffer++;
	} else {
		pSendbuffer = 0;
	}
}

ISR(DMA_CH0_vect)
{
	//static signed BaseType_t test;
	
	DMA.CH0.CTRLB|=0x10;
	fillBuffer(&dacBuffer0[0]);
	
}

ISR(DMA_CH1_vect)
{
	DMA.CH1.CTRLB|=0x10;
	fillBuffer(&dacBuffer1[0]);
}