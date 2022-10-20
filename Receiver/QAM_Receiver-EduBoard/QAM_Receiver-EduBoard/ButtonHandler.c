/*
 * ButtonHandler.c
 *
 * Created: 21.06.2017 12:50:56
 *  Author: mburger
 */ 
 #include <avr/io.h>
 #include "ButtonHandler.h"

 #define Button1_Value (PORTF.IN & PIN4_bm) >> PIN4_bp
 #define Button2_Value (PORTF.IN & PIN5_bm) >> PIN5_bp
 #define Button3_Value (PORTF.IN & PIN6_bm) >> PIN6_bp
 #define Button4_Value (PORTF.IN & PIN7_bm) >> PIN7_bp

 #define BUTTON_PRESS_SHORT			100
 #define BUTTON_PRESS_LONG			500

 

 void initButtons(void) {
	PORTF.DIRCLR = PIN4_bm; //SW1
	PORTF.DIRCLR = PIN5_bm; //SW2
	PORTF.DIRCLR = PIN6_bm; //SW3
	PORTF.DIRCLR = PIN7_bm; //SW4
 }

 button_press_t b1Status;
 button_press_t b2Status;
 button_press_t b3Status;
 button_press_t b4Status;

 void updateButtons(void) {
	static uint16_t b1Count = 0;
	static uint16_t b2Count = 0;
	static uint16_t b3Count = 0;
	static uint16_t b4Count = 0;
	if(Button1_Value == 0) {
		if(b1Count < 60000) {
			b1Count++;
		}
	} else {
		if(b1Count > (BUTTON_PRESS_SHORT / (1000/BUTTON_UPDATE_FREQUENCY_HZ))) {
			if(b1Count > (BUTTON_PRESS_LONG / (1000/BUTTON_UPDATE_FREQUENCY_HZ))) {
				//Button was pressed Long
				b1Status = LONG_PRESSED;
			} else {
				//Button was pressed Short	
				b1Status = SHORT_PRESSED;
			}
		} else {
			b1Status = NOT_PRESSED;
		}
		b1Count = 0;
	}
	if(Button2_Value == 0) {
		if(b2Count < 60000) {
			b2Count++;
		}
	} else {
		if(b2Count > (BUTTON_PRESS_SHORT / (1000/BUTTON_UPDATE_FREQUENCY_HZ))) {
			if(b2Count > (BUTTON_PRESS_LONG / (1000/BUTTON_UPDATE_FREQUENCY_HZ))) {
				//Button was pressed Long
				b2Status = LONG_PRESSED;
				} else {
				//Button was pressed Short
				b2Status = SHORT_PRESSED;
			}
		} else {
			b2Status = NOT_PRESSED;			
		}
		b2Count = 0;
	}
	if(Button3_Value == 0) {
		if(b3Count < 60000) {
			b3Count++;
		}
	} else {
		if(b3Count > (BUTTON_PRESS_SHORT / (1000/BUTTON_UPDATE_FREQUENCY_HZ))) {
			if(b3Count > (BUTTON_PRESS_LONG / (1000/BUTTON_UPDATE_FREQUENCY_HZ))) {
				//Button was pressed Long
				b3Status = LONG_PRESSED;
				} else {
				//Button was pressed Short
				b3Status = SHORT_PRESSED;
			}
		} else {
			b3Status = NOT_PRESSED;
		}
		b3Count = 0;
	}
	if(Button4_Value == 0) {
		if(b4Count < 60000) {
			b4Count++;
		}
	} else {
		if(b4Count > (BUTTON_PRESS_SHORT / (1000/BUTTON_UPDATE_FREQUENCY_HZ))) {
			if(b4Count > (BUTTON_PRESS_LONG / (1000/BUTTON_UPDATE_FREQUENCY_HZ))) {
				//Button was pressed Long
				b4Status = LONG_PRESSED;
				} else {
				//Button was pressed Short
				b4Status = SHORT_PRESSED;
			}
		} else {
			b4Status = NOT_PRESSED;
		}
		b4Count = 0;
	}
 }

 button_press_t getButtonPress(button_t button) {
	switch(button) {
		case BUTTON1:
			return b1Status;
		break;
		case BUTTON2:
			return b2Status;
		break;
		case BUTTON3:
			return b3Status;
		break;
		case BUTTON4:
			return b4Status;
		break;
	}
	return NOT_PRESSED;
 }