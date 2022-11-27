/*
 * init.c
 *
 * Created: 20.03.2018 19:00:42
 *  Author: chaos
 */ 

#include "init.h"
#include "clksys_driver.h"


void vInitClock(void)
{
	CLKSYS_Enable( OSC_RC2MEN_bm );
	do {} while ( CLKSYS_IsReady( OSC_RC2MRDY_bm ) == 0 );
	CLKSYS_Main_ClockSource_Select( CLK_SCLKSEL_RC2M_gc );
	CLKSYS_Disable( OSC_RC32MEN_bm | OSC_RC32KEN_bm | OSC_XOSCEN_bm | OSC_PLLEN_bm);
	CLKSYS_Prescalers_Config( CLK_PSADIV_1_gc, CLK_PSBCDIV_1_1_gc );
	CLKSYS_XOSC_Config( OSC_FRQRANGE_2TO9_gc,false,OSC_XOSCSEL_XTAL_256CLK_gc );
	CLKSYS_Enable( OSC_XOSCEN_bm );
	do {} while ( CLKSYS_IsReady( OSC_XOSCRDY_bm ) == 0 );
	CLKSYS_PLL_Config( OSC_PLLSRC_XOSC_gc, 4 );
	CLKSYS_Enable( OSC_PLLEN_bm );
	CLKSYS_Prescalers_Config( CLK_PSADIV_1_gc, CLK_PSBCDIV_1_1_gc );
	do {} while ( CLKSYS_IsReady( OSC_PLLRDY_bm ) == 0 );
	CLKSYS_Main_ClockSource_Select( CLK_SCLKSEL_PLL_gc );
	CLKSYS_Disable( OSC_RC32MEN_bm | OSC_RC2MEN_bm | OSC_RC32KEN_bm);
}