/*
 * utils.c
 *
 * Created: 21.03.2018 11:19:00
 *  Author: mburger
 */ 
 #include "avr_compiler.h"
 #include "utils.h"

 resetReason_t getResetReason(void) {
	 resetReason_t returnValue = RESETREASON_POWERONRESET;
	 // software reset ?
	 if( RST.STATUS & RST_SRF_bm )
	 {
		 // reset this bit
		 RST.STATUS = RST_SRF_bm;
		 returnValue = RESETREASON_SOFTWARERESET;
	 }
	 // power on reset ?
	 else if( RST.STATUS & RST_PORF_bm)
	 {
		 // reset this bit
		 RST.STATUS = RST_PORF_bm;
		 returnValue = RESETREASON_POWERONRESET;
	 }
	 // debugger reset ?
	 else if( RST.STATUS & RST_PDIRF_bm)
	 {
		 // reset this bit
		 RST.STATUS = RST_PDIRF_bm;
		 returnValue = RESETREASON_DEBUGGERRESET;
	 }
	 // external reset ?
	 else if( RST.STATUS & RST_EXTRF_bm)
	 {
		 // reset this bit
		 RST.STATUS = RST_EXTRF_bm;
		 returnValue = RESETREASON_EXTERNALRESET;
	 }
	 return returnValue;
 }