#include "pwm.h"



/*
 * Header: Initialize PWM module for 20ms signals
 *
 * Params: void
 * Return: void
 */
void InitializePWM(void) 

{
  // set the polarity. If the corresponding bit is set the wave is high at first
  PWMPOL_PPOL0 = 1;
  PWMPOL_PPOL1 = 1;
  
  // Clock SA is the clock source for PWM channel 0 and channel 1
  PWMCLK_PCLK0 = 1;
  PWMCLK_PCLK1 = 1;
  
  // The clock A pre-scalar select are PCKA2, PCKA1, PCKA0-001 will divide the bus clock by 2
  PWMPRCLK_PCKA2 = 0;
  PWMPRCLK_PCKA1 = 0;
  PWMPRCLK_PCKA0 = 1;
  
  // Channel 0 operates in left aligned output mode  
  PWMCAE_CAE0 = 0;
  PWMCAE_CAE1 = 0;
  
  // Clock SA = Clock A / (2 * PWMSCLA)
  PWMSCLA = 40;
  
  // PWM0 Period = Channel Clock Period * PWMPER0 - calculate later
  PWMPER0 = 250; 
  PWMPER1 = 250;
   
  //   enables the PWM channel
  PWME_PWME0  = 1;
  PWME_PWME1  = 1;  
}
