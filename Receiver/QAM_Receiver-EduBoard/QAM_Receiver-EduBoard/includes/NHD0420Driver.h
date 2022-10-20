/*
 * NHD0420Driver.h
 *
 * Created: 15.05.2017 08:31:46
 *  Author: mburger
 */ 


#ifndef NHD0420DRIVER_H_
#define NHD0420DRIVER_H_

#define DISPLAY_QUEUE_DEPTH 8 //Queue Depth of Display Queue. The more vDisplayWriteStringAtPos calls you have between Display-Updates, the more Queue-Spots you need.
#define DISPLAY_UPDATE_TIME_MS 200 //Update-Time of Display-Task. 


typedef struct{
	 uint8_t displayLine;
	 uint8_t displayPos;
	 uint8_t displayBuffer[20];
}displayLine_t;

void vInitDisplay();
void vDisplayClear();
void vDisplayWriteStringAtPos(int line, int pos, char const *fmt, ...);

#endif /* NHD0420DRIVER_H_ */