#ifndef _PIT_H 
#define _PIT_H 

#include "types.h"

//enable PIT interrupts
void init_PIT(uint32_t frequency);

void PIT_handler(void);

uint8_t get_scheduled_term(void);

#endif

