/*
 * errorHandler.h
 *
 * Created: 21.03.2018 11:22:28
 *  Author: mburger
 */ 


#ifndef ERRORHANDLER_H_
#define ERRORHANDLER_H_

// error code defs
//
#define ERR_TEST					  100
#define ERR_QUEUE_CREATE_HANDLE_NULL  42
#define ERR_STACK_OVERFLOW            43
#define ERR_QUEUE_SEND_FAILED         44
#define ERR_QUEUE_RECEIVE_FAILED      45
#define ERR_INVALID_CHANNELINDEX      46
#define ERR_BYTES_SHOULD_BE_AVAILABLE 47
#define ERR_BAD_ISR					  48
#define ERR_DISPATCH_QUEUE_FULL       49
#define ERR_LOW_STACK_SPACE           50
#define ERR_INTERRUPT_YIELD           51
#define ERR_LOW_HEAP_SPACE            52 // not enought rtos heap available
#define ERR_BYTE_LOST_ON_SEND		  53
#define ERR_BYTE_LOST_ON_RECEIVE      54
#define ERR_STRING_OVERFLOW           55
#define ERR_INVALID_STRING_ADDRESS    56
#define ERR_CHANNEL_QUEUE_FULL        57
#define ERR_HEAP_TOO_LARGE			  58 // decrease configTOTAL_HEAP_SIZE = decrease rtos heap !
#define ERR_LOW_GLOBAL_STACK_SPACE    59

// error reporting function
//
void errorNonFatal(uint8_t errCode);
void error(uint8_t errCode);
void checkAllStacks(void);
void software_reset(void);

#endif /* ERRORHANDLER_H_ */