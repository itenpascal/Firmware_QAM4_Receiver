/*
 * ButtonHandler.h
 *
 * Created: 21.06.2017 12:51:09
 *  Author: mburger
 */ 


#ifndef BUTTONHANDLER_H_
#define BUTTONHANDLER_H_

/*---------------------------------------------------------------------------------*/
// UpdateFrequency of the main-program-loop. Should be between 20Hz and 1000Hz
/*---------------------------------------------------------------------------------*/
#define BUTTON_UPDATE_FREQUENCY_HZ	100

typedef enum button_tag {
	BUTTON1,
	BUTTON2,
	BUTTON3,
	BUTTON4
}button_t;

typedef enum button_press_tag {
	LONG_PRESSED,
	SHORT_PRESSED,
	NOT_PRESSED
} button_press_t;

/*---------------------------------------------------------------------------------*/
//Call this Init-Function in the Init-Section of your program
/*---------------------------------------------------------------------------------*/
void initButtons(void);

/*---------------------------------------------------------------------------------*/
// Button Handler. It handles all Button-relevant things like debouncing
// Has to be called every main-program-cycle. As often as it is defined in 
// BUTTON_UPDATE_FREQUENCY_HZ.
/*---------------------------------------------------------------------------------*/
void updateButtons(void);

/*---------------------------------------------------------------------------------*/
// Has to be called directly after each updateButtons() call for each Button you 
// want to check.
/*---------------------------------------------------------------------------------*/
button_press_t getButtonPress(button_t button);

#endif /* BUTTONHANDLER_H_ */