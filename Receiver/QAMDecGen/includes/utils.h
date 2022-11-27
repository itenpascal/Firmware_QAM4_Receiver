/*
 * utils.h
 *
 * Created: 21.03.2018 11:18:47
 *  Author: mburger
 */ 


#ifndef UTILS_H_
#define UTILS_H_

typedef enum resetReason
{
	RESETREASON_SOFTWARERESET = 1,
	RESETREASON_POWERONRESET = 2,
	RESETREASON_DEBUGGERRESET = 3,
	RESETREASON_EXTERNALRESET = 4,
} resetReason_t;

resetReason_t getResetReason(void);

#endif /* UTILS_H_ */