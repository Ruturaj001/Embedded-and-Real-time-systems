#include "timer.h"

UINT8 wait_time;

// Initializes I/O and timer settings for the demo.
//--------------------------------------------------------------       
void InitializeTimer(void)
{
  // Set the timer prescaler to %2, since the bus clock is at 2 MHz,
  // and we want the timer running at 1 MHz
  TSCR2_PR0 = 1;
  TSCR2_PR1 = 0;
  TSCR2_PR2 = 0;
    
  // Enable output compare on Channel 1
  TIOS_IOS1 = 1;
  
  // Set up timer compare value
  TC1 = TC1_VAL;
  
  // Clear the Output Compare Interrupt Flag (Channel 1) 
  TFLG1 = TFLG1_C1F_MASK;
  
  //
  // Enable the timer
  // 
  TSCR1_TEN = 1;
   
  //
  // Enable interrupts via macro provided by hidef.h
  //
  EnableInterrupts;
}

// Disable interrupts on channel 1. 
//--------------------------------------------------------------
void disable_interrupts()
{
  
  TIE_C1I = 0;
  
  // Disable interrupts via macro provided by hidef.h
  DisableInterrupts; 
}

// Enable the input capture interrupt on Channel 1. 
//--------------------------------------------------------------
void enable_interrupts()
{ 
  
  TIE_C1I = 1;
  
  // Enable interrupts via macro provided by hidef.h 
  EnableInterrupts; 
}


// Initializes SCI0 for 8N1, 9600 baud, polled I/O
// The value for the baud selection registers is determined
// using the formula:
//
// SCI0 Baud Rate = ( 2 MHz Bus Clock ) / ( 16 * SCI0BD[12:0] )
//--------------------------------------------------------------

// Output Compare Channel 1 Interrupt Service Routine
// Refreshes TC1 and clears the interrupt flag.
//          
// The first CODE_SEG pragma is needed to ensure that the ISR
// is placed in non-banked memory. The following CODE_SEG
// pragma returns to the default scheme. This is neccessary
// when non-ISR code follows. 
//
// The TRAP_PROC tells the compiler to implement an
// interrupt funcion. Alternitively, one could use
// the __interrupt keyword instead.
// 
// The following line must be added to the Project.prm
// file in order for this ISR to be placed in the correct
// location:
//		VECTOR ADDRESS 0xFFEC OC1_isr 
#pragma push
#pragma CODE_SEG __SHORT_SEG NON_BANKED
//--------------------------------------------------------------     
  
void interrupt 9 OC1_isr( void )
{
  //TC1     +=  TC1_VAL;  
  wait_time = WAIT_COMPLETE;      
  TFLG1   =   TFLG1_C1F_MASK;
  //(void)printf("interupt happned");
}
#pragma pop

/*
 * Header: the timer module for wait
 *
 * Params: void
 * Return: void
 */

void wait_cycle() 
{  
    //delay for 100 ms      
    wait_time = WAITING;
        
    // enable interrupt
    enable_interrupts();
        
    // wait till it happens
    while(WAIT_COMPLETE != wait_time) 
    {
    }
        
    // disable interupt
    disable_interrupts();
}

/*
 * Header: check the cycles
 *
 * Params: wait cycles is the number cycles needed to wait
 * Return: void
 */

UINT8 timer_check(UINT16 wait_cycles) 
{  
    UINT16 i;
    
    //delay for 100 ms      
    wait_time = WAITING;
        
    // enable interrupt
    enable_interrupts();
        
    // wait till it happens
    while(WAIT_COMPLETE != wait_time && i<wait_cycles) 
    {
      i++;
    }
        
    // disable interupt
    disable_interrupts();
    
    if(i == wait_cycles) 
    {  
      return 0;
    }
    else 
    {
      return 1;
    }
}