/******************************************************************************
 * Servo Motors, State Machine Demo - Project 1
 *
 * Description:
 *
 * This demo configures the timer to wait for 100 ms.It also configures PWM channel
 * 0 and channel 1 to produce waves of various duty cycles corresponding to 0-5  
 * positions in the range of 0 degrees to 160 degrees. These waves move the servo 
 * motors.This demo allows the motors to run, pause and end recipe of commands. It 
 * also takes input from the user and does appropriate actions.  
 *.  
 * 
 * 
 * Author:
 *  Ruturaj Hagawane
 *  Maria Shoaib
 *
 *****************************************************************************/


// system includes
#include <hidef.h>      /* common defines and macros */

// project includes
#include "types.h"
#include "derivative.h" /* derivative-specific definitions */
#include "serial.h"
#include "timer.h"
#include "pwm.h"
#include "stepper.h"
#include "led.h"
#include "POST.h"

// 
#define PWM_CHANNEL_STEPPER1 0
#define PWM_CHANNEL_STEPPER2 1

#define STEPPER1_RECIPE 4
#define STEPPER2_RECIPE 4

#define INVALID_INPUTS 0
#define VALID_INPUTS 1

void main(void) {
  
  UINT8 input_validity;
  UINT8 userInput1,userInput2,userInput3;
  UINT8 flag1, flag2;
  
  struct Stepper stepper1,stepper2;
  
	// set up serial port communication
  InitializeSerialPort();

  
  InitializePWM(); 
	
	//Intialize timer
	InitializeTimer();
	
	// allocate and fill receipe array
	InitializeRecipe();
	
	IntializeLED();
	
	set_stepper(&stepper1,PWM_CHANNEL_STEPPER1,STEPPER1_RECIPE);
	set_stepper(&stepper2,PWM_CHANNEL_STEPPER2,STEPPER2_RECIPE);
  
  if(1 == POST())
  {
    printf("POST failed\r\n");
    for(;;) 
    {
    }
    
  } 
  else 
  {
    printf("POST successful\r\n");  
  
    (void)printf(">");
    for(;;) {
      _FEED_COP();
   
      //check serial port for input
      if(1 == BufferEmpty()) 
      { 
        do
        {  
          userInput1 = GetChar();
          (void)printf("%c", userInput1);
          
          userInput2 = GetChar();
          (void)printf("%c", userInput2);
        
          //get <CR>
          userInput3 = GetChar();
          
          if( 'x' == userInput1 || 'X' == userInput1 || 'x' == userInput2 || 'X' == userInput2 ) 
          {
            input_validity = INVALID_INPUTS;  
          } 
          else
          {
            input_validity = VALID_INPUTS;
          }
          
        }while(INVALID_INPUTS == input_validity);
        
        // print <LF> and then '>'
        (void)printf("\r\n>");
        
        //set state of stepper1
        update_state(&stepper1,userInput1);
        
        //set state of stepper2
        update_state(&stepper2,userInput2);
      }
      
      // for stepper  1 take action according to state
      flag1 = take_action(&stepper1);
      
      // for stepper 2 take action acording to state  
  	  flag2 = take_action(&stepper2);
  	  
  	  glow_led(flag1,flag2);
  	  
  	  (void)wait_cycle();
    } 
  }
  
}
