/*
 * errorHandler.c
 *
 * Created: 21.03.2018 11:22:41
 *  Author: mburger
 */ 

 #include "avr_compiler.h"
 #include "FreeRTOS.h"
 #include "task.h"
 #include "queue.h"
 #include "event_groups.h"
 #include "stack_macros.h"
 #include "errorHandler.h"


 typedef void tskTCB;
 extern volatile tskTCB * volatile pxCurrentTCB;

 // local prototypes
 void vApplicationStackOverflowHook( xTaskHandle *pxTask, signed portCHAR *pcTaskName );
 void vApplicationMallocFailedHook( void );

 //----------------------------------------------
 // catch heap overflow
 //
 void vApplicationMallocFailedHook( void )
 {
	 error(ERR_LOW_HEAP_SPACE);
 }


 //----------------------------------------------
 // to report kernel detected stack overflows
 //
 void vApplicationStackOverflowHook( xTaskHandle *pxTask, signed portCHAR *pcTaskName )
 {
	 /* Just to stop compiler warnings. */
	 (void) pxTask;
	 (void) pcTaskName;

	 error(ERR_STACK_OVERFLOW);

 }

 //----------------------------------------------
 //
 // for errors that are not directly caused by
 // kernel problems
 //
 void errorNonFatal(uint8_t errCode)
 {
	 (void)errCode;

	 uint8_t a=42; (void)a;

 }

 //----------------------------------------------
 //
 void error(uint8_t errCode)
 {
	 uint8_t a;

	 if(errCode==ERR_STACK_OVERFLOW)
	 a = 1;
	 else if(errCode==ERR_QUEUE_SEND_FAILED)
	 a = 2;
	 else if(errCode==ERR_BYTES_SHOULD_BE_AVAILABLE)
	 a = 3;
	 else
	 a = 4;

	 // TODO from here:
	 //
	 // - log the occurance of any error in flash mem (inc counter)
	 // - print a message

	 software_reset();
 }

 //----------------------------------------------
 //
 void software_reset(void)
 {	 
	 asm("nop");
	 CPU_CCP  = CCP_IOREG_gc;
	 RST.CTRL = RST_SWRST_bm ;	 
 }