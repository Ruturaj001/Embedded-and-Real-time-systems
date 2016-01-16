#include <hidef.h>      /* common defines and macros */
#include <ctype.h>
#include <stdlib.h>
#include "types.h"
#include "derivative.h" /* derivative-specific definitions */

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

// Boolean Definitions to make the code more readable.
#define FALSE 0
#define TRUE 1

#define OFF 0
#define ON 1

UINT8 motor_switch = OFF;         //This variable will enable the logic for motor driving.
UINT8 motorPosition = 0;                 //This variable will set the position of servo.


/* Initializes PWM channel 1 port for 20ms period
 *
 */
void InitializePWM() 
{
    PWMCLK_PCLK1 = 1;
    PWMPOL_PPOL1 = 1; 
    PWMPRCLK_PCKA2 = 0;
    PWMPRCLK_PCKA1 = 0;
    PWMPRCLK_PCKA0 = 1;  
    PWMSCLA = 40;
    PWMPER1 = 250;
    PWME_PWME1 = 1;
}

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

// ISR when we have input on port B
//--------------------------------------------------------------   
#pragma push
#pragma CODE_SEG __SHORT_SEG NON_BANKED
//--------------------------------------------------------------      
void interrupt 9 OC1_isr( void ) 
{    
        TC1     +=  TC1_VAL;      
        TFLG1   =   TFLG1_C1F_MASK;   
   
}
#pragma pop
 
void main(void)
{
    UINT32 i=0;
  
    DDRB = 0x00;
    DDRA = 0x00;
  
    InitializeSerialPort();
    InitializeTimer();
    InitializePWM();

    for(;;) 
    {
        //detects if button was pressed
        if( (PORTB & 0x01) == 0) 
        {
            if(motor_switch == ON)
            {
                motor_switch = OFF;
            } 
            else
            {
                motor_switch = ON;
            }
            for(i=655340; i>0; --i)
            {
                // wait for some time  
            }
        }
        if(motor_switch == ON) 
        {
            // if button is on move servo accordingly
            PWMDTY1 = PORTA;
        } 
    }  
}