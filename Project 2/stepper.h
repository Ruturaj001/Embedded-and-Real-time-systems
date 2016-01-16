#ifndef _stepper_
#define _stepper_

#include <hidef.h>      /* common defines and macros */
#include <stdlib.h>

#include "serial.h"
#include "derivative.h" /* derivative-specific definitions */
#include "types.h"

#define PWM_CHANNEL0 0
#define PWM_CHANNEL1 1

#define STEPPER_POSITIONS 6

#define NO_OF_RECIPES 9

#define END ((UINT8) 0x00 << 5)
#define MOV ((UINT8) 0x01 << 5) 
#define WAIT ((UINT8) 0x02 << 5) 
#define START_LOOP ((UINT8) 0x04 << 5)
#define END_LOOP ((UINT8) 0x05 << 5) 
#define LOAD ((UINT8) 0x06 << 5)

#define COMMAND_ERROR_LED ((UINT8) 0x01 << 3)
#define NESTED_ERROR_LED ((UINT8) 0x01 << 2)
#define RECIPE_END_LED ((UINT8) 0x01 << 1)
#define RECIPE_PAUSE_LED ((UINT8) 0x01)


typedef enum State 
{  
  BEGIN = 0,
  RUN,
  PAUSE,
  RECIPE_END, 
  ERROR
};

typedef enum ERROR_ENCOUTERED 
{
  NO_ERROR = 0,
  RECIPE_COMMAND_ERROR,
  NESTED_LOOP_ERROR
};

struct Stepper 
{
  // pointer to script to be ran on stepper
  UINT8 recipe_number;
  
  // state
  enum State state;
  
  // Current position of stepper
  UINT8 position;
  
  // Program counter
  UINT8 PC; 
  
  // Wait counter
  UINT8 WC;
  
  // Loop start position
  UINT8 LPS;
  
  // Loop counter
  UINT8 LPC;
  
  // PWM channel
  UINT8 pwm_channel;
  
  enum ERROR_ENCOUTERED error_encountered;
  
  //if paused then next step
  UINT8 next_move;
};

void set_stepper(struct Stepper *stepper, UINT8 pwm, UINT8 recipe_no);
void move(struct Stepper *stepper,UINT8 position);
UINT8 take_action(struct Stepper *stepper);
void run_next_command(struct Stepper *stepper);
void InitializeRecipe(void);
void update_state(struct Stepper *stepper, UINT8 input);

#endif