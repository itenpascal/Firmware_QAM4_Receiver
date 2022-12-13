/*
 * qamdec.h
 *
 * Created: 05.05.2020 16:38:15
 *  Author: Chaos
 */ 

#define NR_OF_ARRAY_WHOLE					256		// 



#ifndef QAMDEC_H_
#define QAMDEC_H_

uint16_t array[NR_OF_ARRAY_WHOLE];
uint16_t speicherWrite;

void vQuamDec(void* pvParameters);


#endif /* QAMDEC_H_ */