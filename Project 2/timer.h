#ifndef _timer_
#define _timer_

#include <hidef.h>      /* common defines and macros */
#include "types.h"
#include "derivative.h" /* derivative-specific definitions */
                        
#define WAIT_COMPLETE 1
#define WAITING 0

// Change this value to change the frequency of the output compare signal.
// The value is in Hz.
#define OC_FREQ_HZ    ((UINT16)10)

// Macro definitions for determining the TC1 value for the desired frequency
// in Hz (OC_FREQ_HZ). The formula is:
//
// TC1_VAL = ((Bus Clock Frequency / Prescaler value) / 2) / Desired Freq in Hz
//
// Where:
//        Bus Clock Frequency     = 2 MHz
//        Prescaler Value         = 2 (Effectively giving us a 1 MHz timer)
//        2 --> Since we want to toggle the output at half of the period
//        Desired Frequency in Hz = The value you put in OC_FREQ_HZ
//
#define BUS_CLK_FREQ  ((UINT32) 2000000)   
#define PRESCALE      ((UINT16)  2)         
#define TC1_VAL       ((UINT16)  (((BUS_CLK_FREQ / PRESCALE) / 2) / OC_FREQ_HZ))

void InitializeTimer(void);
void disable_interrupts();
void enable_interrupts();
void wait_cycle(void);
UINT8 timer_check(UINT16 wait_cycles);


#endif