/*
 * qaminit.h
 *
 * Created: 02/11/2021 20:15:15
 *  Author: burge
 */ 


#ifndef QAMINIT_H_
#define QAMINIT_H_

#define NR_OF_SAMPLES						32UL
#define GENERATOR_FREQUENCY_INITIAL_VALUE	1000UL * NR_OF_SAMPLES
#define DECODER_FREQUENCY_INITIAL_VALUE		1000UL * NR_OF_SAMPLES

extern uint16_t dacBuffer0[NR_OF_SAMPLES];

extern uint16_t dacBuffer1[NR_OF_SAMPLES];

extern uint16_t adcBuffer0[NR_OF_SAMPLES];
extern uint16_t adcBuffer1[NR_OF_SAMPLES];

EventGroupHandle_t evDMAState;
#define		DMAGENREADY	1<<0
#define		DMADECREADY	1<<0

void initGenDMA(void);
void initDecDMA(void);
void initDAC(void);
void initADC(void);
void initDACTimer(void);
void initADCTimer(void);



#endif /* QAMINIT_H_ */