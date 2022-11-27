/*
 * NHD0420Driver.c
 *
 * Created: 15.05.2017 08:32:12
 *  Author: mburger
 */ 
//  #include <avr/io.h>
//  #include <avr/interrupt.h>
#include <stdarg.h>
#include <string.h>
#include "avr_compiler.h"
//#include "pmic_driver.h"
#include "TC_driver.h"
//#include "clksys_driver.h"
//#include "sleepConfig.h"
//#include "port_driver.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "event_groups.h"
//#include "stack_macros.h"

#include "NHD0420Driver.h"
 
#define EG_DISPLAY_DELAY 1
#define EG_DISPLAY_CLEAR 2
EventGroupHandle_t egDisplayTiming;

xQueueHandle displayLineQueue;

 
static void ftoa_fixed(char *buffer, double value);
static void ftoa_sci(char *buffer, double value);

void vDisplayUpdateTask(void *pvParameters);
static int display_vprintf(int line, int pos, char const *fmt, va_list arg);


ISR(TCF0_OVF_vect) {
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	xEventGroupSetBitsFromISR(egDisplayTiming, EG_DISPLAY_DELAY,&xHigherPriorityTaskWoken);
	TC0_ConfigClockSource(&TCF0, TC_CLKSEL_OFF_gc); //Disable Timer
	TCF0.INTCTRLA = 0x00;
}

 void delayUS(uint32_t us) {
	if(us < 2) {
		us = 2;
	}	
	TCF0.INTCTRLA = 0x01;
	TCF0.CNT = 0;
	TC0_ConfigWGM(&TCF0, TC_WGMODE_NORMAL_gc);
	if(us < 0xFFFF*2) {
		TC_SetPeriod(&TCF0, us/2);
		TC0_ConfigClockSource(&TCF0, TC_CLKSEL_DIV64_gc); //Enable Timer with Prescaler 65 = 2us
	} else if((us/1000) < 1000) {
		TC_SetPeriod(&TCF0, (us/32));
		TC0_ConfigClockSource(&TCF0, TC_CLKSEL_DIV1024_gc); //Enable Timer with Prescaler 1024 = 32us
	}
	xEventGroupWaitBits(egDisplayTiming, EG_DISPLAY_DELAY, pdTRUE, pdFALSE, 500 / portTICK_RATE_MS ); //Wait 500ms at a maximum
 }
 void setPort(uint8_t data) {
	data &= 0x0F;
	data <<= 4;
	PORTA.OUT &= (data | 0x0F);
	PORTA.OUT |= data;
 }
 void setRS(char value) {
	if(value > 0) {
		PORTD.OUTSET = PIN0_bm;
	} else {
		PORTD.OUTCLR = PIN0_bm;
	}
 }
 void setRW(char value) {
	if(value > 0) {
		PORTD.OUTSET = PIN1_bm;
		} else {
		PORTD.OUTCLR = PIN1_bm;
	}
 }
 void setE(char value) {
	if(value > 0) {
		PORTD.OUTSET = PIN2_bm;
		} else {
		PORTD.OUTCLR = PIN2_bm;
	}
 }
 void Nybble() {
	setE(1);	
	delayUS(1);	
	setE(0);
 }
 void command(char i) {
	setPort((i>>4)&0x0F);
	setRS(0);
	setRW(0);
	Nybble();
	setPort(i & 0x0F);
	Nybble();
 }
 void write(char i) {
	setPort((i>>4)&0x0F);
	setRS(1);
	setRW(0);
	Nybble();
	setPort(i & 0x0F);
	Nybble();
 }
 void displayHome() {
	 command(0x02);
 }
 void _displayClear() {
	 command(0x01);
	 delayUS(2000);
 }
 
 void vInitDisplay() {
	PORTA.DIRSET = PIN4_bm;
	PORTA.DIRSET = PIN5_bm;
	PORTA.DIRSET = PIN6_bm;
	PORTA.DIRSET = PIN7_bm;
	PORTD.DIRSET = PIN0_bm;
	PORTD.DIRSET = PIN1_bm;
	PORTD.DIRSET = PIN2_bm;
	PORTA.OUT &= 0x0F;
	PORTD.OUT &= 0xF8;

	if((displayLineQueue = xQueueCreate(DISPLAY_QUEUE_DEPTH, sizeof(displayLine_t))) == NULL)
	{
		//error(ERR_QUEUE_CREATE_HANDLE_NULL);
	}
	
	egDisplayTiming = xEventGroupCreate();
	

	xTaskCreate(vDisplayUpdateTask, (const char*) "dispUpdate", configMINIMAL_STACK_SIZE+150, NULL, 1, NULL);	
 }
 
 void _displaySetPos(int line, int pos) {
	 switch(line) {
		 case 0:
		 command(0x80 + 0x00 + pos);
		 break;
		 case 1:
		 command(0x80 + 0x40 + pos);
		 break;
		 case 2:
		 command(0x80 + 0x14 + pos);
		 break;
		 case 3:
		 command(0x80 + 0x54 + pos);
		 break;
	 }
	 delayUS(39);
 }

 void _displayWriteChar(char c) {
	 write(c);
	 delayUS(43);
 }
 
 void _displayWriteString(char* s) {
	 for(int i = 0; i < 20; i++) {
		 if(s[i] == '\0') {
			 break;
		 }
		 _displayWriteChar(s[i]);
	 }
 }

 void _displayWriteStringAtPos(int line, int pos, char* s) {
	 _displaySetPos(line, pos);
	 _displayWriteString(s);
 }

 void vDisplayUpdateTask(void *pvParameters) {
	 int i = 0;
	 int j = 0;
	 char displayLines[4][20];
	 for(int i = 0; i < 4;i++) {
		for(int j = 0; j < 20; j ++) {
			displayLines[i][j] = 0x20;
		}
	 }
	 displayLine_t newLine;

	 delayUS(40000);
	 setPort(0x03);
	 delayUS(5000);
	 Nybble();
	 delayUS(160);
	 Nybble();
	 delayUS(160);
	 Nybble();
	 delayUS(160);
	 setPort(0x02);
	 Nybble();
	 command(0x28);
	 command(0x10);
	 command(0x0C); //Cursor and Blinking off
	 command(0x06);
	 
	 for(;;) {		 
		 vTaskDelay(DISPLAY_UPDATE_TIME_MS/portTICK_RATE_MS);
		 if(xEventGroupGetBits(egDisplayTiming) && EG_DISPLAY_CLEAR != 0x00) {
			xEventGroupClearBits(egDisplayTiming, EG_DISPLAY_CLEAR);
			for(i = 0; i < 4;i++) {
				for(j = 0; j < 20; j ++) {
					displayLines[i][j] = 0x20;
				}
			}
		 }
		 while(uxQueueMessagesWaiting(displayLineQueue) > 0) {
			 if(xQueueReceive(displayLineQueue, &newLine, portMAX_DELAY)) {	
				i=0;			
				while((i+newLine.displayPos < 20) && (newLine.displayBuffer[i] != 0x00)) {				
					displayLines[newLine.displayLine][i+newLine.displayPos] = newLine.displayBuffer[i];
					i++;
				}
			 }
		 }
		 for(i = 0; i < 4; i++) {
			 _displayWriteStringAtPos(i,0,&displayLines[i][0]);
		 }
	 }
 }
 

void vDisplayClear() {
	xEventGroupSetBits(egDisplayTiming, EG_DISPLAY_CLEAR);
}

void vDisplayWriteStringAtPos(int line, int pos, char const *fmt, ...) {
	va_list arg;	
	va_start(arg, fmt);
	display_vprintf(line, pos, fmt, arg);
	va_end(arg);	
}
 
static int display_vprintf(int line, int pos, char const *fmt, va_list arg) {
	int int_temp;
	char char_temp;
	char *string_temp;
	double double_temp;
	char ch;
	int length = 0;

	static char buffer[20];
	static char str[20];
	for(int i = 0; i < 20; i++) {
		buffer[i] = 0x00;
	}
	for(int i = 0; i < 20; i++) {
		str[i] = 0x00;
	}
	while ((ch = *fmt++)!=false) {
		if ( '%' == ch ) {
			switch (ch = *fmt++) {
				/* %% - print out a single %    */
				case '%':
				str[length] = '%';
				length++;
				break;

				/* %c: print out a character    */
				case 'c':
				char_temp = va_arg(arg, int);
				str[length] = char_temp;
				length++;
				break;

				/* %s: print out a string       */
				case 's':
				string_temp = va_arg(arg, char *);
				for(int i = 0; i < strlen(string_temp);i++) {
					str[length+i] = string_temp[i];
				}
				length += strlen(string_temp);
				break;

				/* %d: print out an int         */
				case 'd':
				int_temp = va_arg(arg, int);
				itoa(int_temp, buffer, 10);
				for(int i = 0; i < strlen(buffer);i++) {
					str[length+i] = buffer[i];
				}
				length += strlen(buffer);
				break;

				/* %x: print out an int in hex  */
				case 'x':
				int_temp = va_arg(arg, int);
				itoa(int_temp, buffer, 16);
				for(int i = 0; i < strlen(buffer);i++) {
					str[length+i] = buffer[i];
				}
				length += strlen(buffer);
				break;

				case 'f':
				double_temp = va_arg(arg, double);
				ftoa_fixed(buffer, double_temp);
				for(int i = 0; i < strlen(buffer);i++) {
					str[length+i] = buffer[i];
				}
				length += strlen(buffer);
				break;

				case 'e':
				double_temp = va_arg(arg, double);
				ftoa_sci(buffer, double_temp);
				for(int i = 0; i < strlen(buffer);i++) {
					str[length+i] = buffer[i];
				}
				length += strlen(buffer);
				break;
			}
		}
		else {
			str[length] = ch;
			if(str[length] == '\n') {
				str[length] = '\0';				
			}
			length++;
		}
	}
	if(length + pos >= 20) {
		length = 20-pos;
	}
	displayLine_t newLine;
	for(int i = 0; i < 20; i++) {
		newLine.displayBuffer[i] = 0x00;
	}
	newLine.displayLine = line;
	newLine.displayPos = pos;
	for(int i = 0; i < length;i++) {
		newLine.displayBuffer[i] = str[i];
	}	
	xQueueSend(displayLineQueue, (void *) &newLine, portMAX_DELAY);
	
	
	return length;
}

static int normalize(double *val) {
    int exponent = 0;
    double value = *val;

    while (value >= 1.0) {
        value /= 10.0;
        ++exponent;
    }

    while (value < 0.1) {
        value *= 10.0;
        --exponent;
    }
    *val = value;
    return exponent;
}

static void ftoa_fixed(char *buffer, double value) {  
    /* carry out a fixed conversion of a double value to a string, with a precision of 5 decimal digits. 
     * Values with absolute values less than 0.000001 are rounded to 0.0
     * Note: this blindly assumes that the buffer will be large enough to hold the largest possible result.
     * The largest value we expect is an IEEE 754 double precision real, with maximum magnitude of approximately
     * e+308. The C standard requires an implementation to allow a single conversion to produce up to 512 
     * characters, so that's what we really expect as the buffer size.     
     */

    int exponent = 0;
    int places = 0;
    static const int width = 4;

    if (value == 0.0) {
        buffer[0] = '0';
        buffer[1] = '\0';
        return;
    }         

    if (value < 0.0) {
        *buffer++ = '-';
        value = -value;
    }

    exponent = normalize(&value);

    while (exponent > 0) {
        int digit = value * 10;
        *buffer++ = digit + '0';
        value = value * 10 - digit;
        ++places;
        --exponent;
    }

    if (places == 0)
        *buffer++ = '0';

    *buffer++ = '.';

    while (exponent < 0 && places < width) {
        *buffer++ = '0';
        --exponent;
        ++places;
    }

    while (places < width) {
        int digit = value * 10.0;
        *buffer++ = digit + '0';
        value = value * 10.0 - digit;
        ++places;
    }
    *buffer = '\0';
}

void ftoa_sci(char *buffer, double value) {
    int exponent = 0;    
    static const int width = 4;

    if (value == 0.0) {
        buffer[0] = '0';
        buffer[1] = '\0';
        return;
    }

    if (value < 0.0) {
        *buffer++ = '-';
        value = -value;
    }

    exponent = normalize(&value);

    int digit = value * 10.0;
    *buffer++ = digit + '0';
    value = value * 10.0 - digit;
    --exponent;

    *buffer++ = '.';

    for (int i = 0; i < width; i++) {
        int digit = value * 10.0;
        *buffer++ = digit + '0';
        value = value * 10.0 - digit;
    }

    *buffer++ = 'e';
    itoa(exponent, buffer, 10);
}
