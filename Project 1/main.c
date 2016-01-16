/******************************************************************************
 * Histogram Generation Demo-Project 1
 *
 * Description:
 *
 * This demo configures the timer to a rate of 1 MHz and takes 1 KHz square 
 * wave as an Input Capture on Channel 1. The interrupts are captured on rising
 * edges and 1000 measurements are taken to calculate the inter-arrival times.
 * These readings are placed into corresponding buckets ranging from 950 micro-
 * seconds to 1050 microseconds. 
 *.  
 * 
 * The Output Compare Channel 0 Interrupt is used as a countdown timer in the 
 * post routine. If the timer expires first, then the board is faulty.  
 * 
 * Author:
 *  Maria Shoaib
 *  Ruturaj Hagawane
 *
 *****************************************************************************/


// system includes
#include <hidef.h>      /* common defines and macros */
#include <stdio.h>      /* Standard I/O Library */

// project includes
#include "types.h"
#include "derivative.h" /* derivative-specific definitions */

// Definitions

// Wait time used in POST to wait for signal
#define WAIT_TIME     ((UINT32) 20000)

// The lower limit and upper limit correspond to the lowest and highest inter-arrival
// time from our range of consideration.
#define LOWER_LIMIT 950
#define UPPER_LIMIT 1050

// Number of buckets required is calculated using upper limit and lower limit
#define NO_OF_BUCKETS UPPER_LIMIT - LOWER_LIMIT + 1

// Number of total samples of inter arrival time we are going to take
// and Number of readings we need for that (one more than required samples)
#define COUNT_INTER_ARRIVAL_TIME_SAMPLES 1000
#define NO_OF_READINGS COUNT_INTER_ARRIVAL_TIME_SAMPLES + 1

// Results of POST (Power-On Self Test)
#define SUCCESS 0
#define FAILURE 1

// Stores all reading taken on rising edge
UINT16 readings[NO_OF_READINGS];

// Buckets store the occurrence of a particular value.
UINT16 buckets[NO_OF_BUCKETS];

// Number of readings taken
UINT16 reading_number;

// Initializes SCI0 for 8N1, 9600 baud, polled I/O
// The value for the baud selection registers is determined
// using the formula:
//
// SCI0 Baud Rate = ( 2 MHz Bus Clock ) / ( 16 * SCI0BD[12:0] )
//--------------------------------------------------------------
void InitializeSerialPort(void)
{
    // Set baud rate to ~9600 (See above formula)
    SCI0BD = 13;          
    
    // 8N1 is default, so we don't have to touch SCI0CR1.
    // Enable the transmitter and receiver.
    SCI0CR2_TE = 1;
    SCI0CR2_RE = 1;
}


// Initializes I/O and timer settings to capture rising edge wave.
//--------------------------------------------------------------       
void InitializeTimer(void)
{
  // Set the timer prescaler to %2, since the bus clock is at 2 MHz,
  // and we want the timer running at 1 MHz
  TSCR2_PR0 = 1;
  TSCR2_PR1 = 0;
  TSCR2_PR2 = 0;
  
  // Channel 1  
  // Enable input capture on Channel 1
  TIOS_IOS1 = 0;
  
  // Clear the input capture Interrupt Flag (Channel 1) 
  TFLG1 = TFLG1_C1F_MASK;
  
  //
  // Enable the timer
  // 
  TSCR1_TEN = 1;
 
  //Set the rising edge interrupts
  TCTL4_EDG1A=1;
  TCTL4_EDG1B=0; 
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

// Input Capture Channel 1 Interrupt Service Routine
// clears the interrupt flag, stores new reading 
#pragma push
#pragma CODE_SEG __SHORT_SEG NON_BANKED
//--------------------------------------------------------------       
void interrupt 9 OC1_isr( void )
{
  if(TFLG1_C1F==1)
  {
    // interrupt flag clear 
    TFLG1   =   TFLG1_C1F_MASK;
    
    // Store reading and increment reading count
    readings[reading_number]=TC1;  
    reading_number++; 
  }
}
#pragma pop


// This function is called by printf in order to
// output data. Our implementation will use polled
// serial I/O on SCI0 to output the character.
//
// Remember to call InitializeSerialPort() before using printf!
//
// Parameters: character to output
//--------------------------------------------------------------       
void TERMIO_PutChar(INT8 ch)
{
    // Poll for the last transmit to be complete
    do
    {
      // Nothing  
    } while (SCI0SR1_TC == 0);
    
    // write the data to the output shift register
    SCI0DRL = ch;
}


// Polls for a character on the serial port.
//
// Returns: Received character
//--------------------------------------------------------------       
UINT8 GetChar(void)
{ 
  // Poll for data
  do
  {
    // Nothing
  } while(SCI0SR1_RDRF == 0);
   
  // Fetch and return data from SCI0
  return SCI0DRL;
}

// Calculates inter arrival time using readings
// Creates Histogram using inter arrival times and prints line
// by line
//-------------------------------------------------------------- 
void print_histogram(void) 
{
  int counter=0;
  
  // Initialize all buckets
  for(counter=0;counter<NO_OF_BUCKETS;counter++) 
  {
    buckets[counter] = 0;
  }
  
  for (counter=0;counter<COUNT_INTER_ARRIVAL_TIME_SAMPLES;counter++) 
  { 
    // calculate inter arrival time using readings
    int interArrivalTime = readings[counter+1] - readings[counter];
    
    // if reading falls under required criteria then increment bucket for
    // respective inter arrival time
    if(interArrivalTime >= LOWER_LIMIT && interArrivalTime <= UPPER_LIMIT ) 
    {
       buckets[interArrivalTime - LOWER_LIMIT]++;
    }
  }
  
  // Prints inter arrival time and number of samples at that value
  (void)printf("\r\nInter Arrival Time\tCount"); 
  for(counter=0;counter<NO_OF_BUCKETS;counter++) 
  {
    if(buckets[counter] != 0) 
    { 
      (void)printf("\r\n%u\t\t\t%u",LOWER_LIMIT+counter,buckets[counter]);  
      (void)GetChar();  
    }
  }
  
}

// Power On Self Test
// Checks if channel 1 is working
// 
// Returns: SUCCESS if it's working otherwise Failure
//-------------------------------------------------------------- 
UINT8 post() 
{
  UINT32 timer;
  reading_number=0;
  
  // We need to check if rising edge is detected through Channel 1
  // so we enable interrupt  
  enable_interrupts();
  
  for(timer = 0; timer < WAIT_TIME;timer++) 
  {
    //wait for Interrupt
    if(reading_number > 0) 
    {
      // if we get Interrupts from rising edge, POST is successful
      disable_interrupts();
      return SUCCESS;
    }
      
  }
  // if we do not get Interrupt from rising edge, POST has failed
  disable_interrupts();
  return FAILURE; 
}

// Entry point of our application code
//--------------------------------------------------------------       
void main(void)
{

  UINT8 userInput;
  
  // set up serial port communication
  InitializeSerialPort();
  InitializeTimer();
  
  if(SUCCESS == post()) 
  {
    // If POST is successful continue with main task 
    (void)printf("Power-on Self Test Successful!\r\n");
    while( 'N' != userInput && 'n' != userInput ) 
    {
    
      // Prompt user 
      (void)printf("\r\nPress any key when inputs are ready\r\n");
      
      // Fetch and echo use input
      userInput = GetChar();
      (void)printf("%c", userInput);
      
      // reset reading number
      reading_number=0;
      
      // to collect inputs we need Interrupts enabled
      enable_interrupts();
      
      // Take readings through ISR
      while(reading_number<NO_OF_READINGS) 
      {
      }
      
      // After gathering required readings, we don't need Interrupts
      disable_interrupts();
      
      // prompt user when he needs output 
      (void)printf("\r\nPress any key when you want to see output\r\n"); 
      
      // Fetch and echo use input
      userInput = GetChar();
      (void)printf("%c", userInput);
      
      print_histogram();
      
      // prompt user when he needs output 
      (void)printf("\r\nPress any key to continue or N/n to quit\r\n");
      
      // Fetch and echo use input
      userInput = GetChar();
      (void)printf("%c", userInput); 
      
    }
    
  } 
  else 
  {
     // If POST is failed print error message and exit
    (void)printf("\r\nPower-on Self Test failed!\r\n");
    return;
  }
}