#include "stepper.h"

UINT8 duty_for_position[STEPPER_POSITIONS] = {5,9,13,17,20,24};
UINT8 **recipe;


/*
 * Header: Initializes the recipe table
 *
 * Params: void
 * Return: void
 */

void InitializeRecipe()
{
	recipe = malloc(sizeof(UINT8 *) * NO_OF_RECIPES);
	
	
	/* Verify default time delay */
	recipe[0] = malloc(sizeof(UINT8) * 4);
	recipe[0][0] = MOV|0;
	recipe[0][1] = MOV|5;
	recipe[0][2] = MOV|0;
	recipe[0][3] = END;
	
	/* Verify default loop behavior */
	recipe[1] = malloc(sizeof(UINT8) * 7);
	recipe[1][0] = MOV|3;
	recipe[1][1] = START_LOOP|0;
	recipe[1][2] = MOV|1;
	recipe[1][3] = MOV|4;
	recipe[1][4] = END_LOOP;
	recipe[1][5] = MOV|0;
	recipe[1][6] = END;
	
	/* Verify wait 0 */
	recipe[2] = malloc(sizeof(UINT8) * 4);
	recipe[2][0] = MOV|2;
	recipe[2][1] = WAIT|0;
	recipe[2][2] = MOV|3;
	recipe[2][3] = END;
	
	/* 9.3 second delay */
	recipe[3] = malloc(sizeof(UINT8) * 7);
	recipe[3][0] = MOV|2;
	recipe[3][1] = MOV|3;
	recipe[3][2] = WAIT|31;
	recipe[3][3] = WAIT|31;
	recipe[3][4] = WAIT|31;
	recipe[3][5] = MOV|4;
	recipe[3][6] = END; 
	
	/* Every position */
	recipe[4] = malloc(sizeof(UINT8) * 12);
	recipe[4][0] = MOV|0;
	recipe[4][1] = WAIT|10;
	recipe[4][2] = MOV|1;
	recipe[4][3] = WAIT|10;
	recipe[4][4] = MOV|2;
	recipe[4][5] = WAIT|10;
	recipe[4][6] = MOV|3;
	recipe[4][7] = WAIT|10;
	recipe[4][8] = MOV|4;
	recipe[4][9] = WAIT|10;
	recipe[4][10] = MOV|5;
	recipe[4][11] = END;
	
	
	/* Immediate end */
	recipe[5] = malloc(sizeof(UINT8) * 3);
	recipe[5][0] = END;
	recipe[5][1] = MOV|3;
	recipe[5][2] = END;
	
	/* Recipe command error */
	recipe[6] = malloc(sizeof(UINT8) * 4);
	recipe[6][0] = MOV|0;
	recipe[6][1] = MOV|5;
	recipe[6][2] = MOV|6;
	recipe[6][3] = END;
  
  /* Nested Loop Error */
  recipe[7] = malloc(sizeof(UINT8) * 2);
  recipe[7][0] = START_LOOP|1;
  recipe[7][1] = START_LOOP|1;
  recipe[7][2] = END; 

  /* Recipe with new OPcode */	
	recipe[8] = malloc(sizeof(UINT8) * 3);
	recipe[8][0] = MOV|5;
	recipe[8][1] = WAIT|2;
	recipe[8][2] = LOAD|4;
	
}

/*
 * Header: Initialization of stepper struct
 *
 * Params: stepper motor 1 or 2, pwm, current recipe - recipe being loaded
 * Return: void
 */

void set_stepper(struct Stepper *stepper, UINT8 pwm, UINT8 current_recipe) 
{
  stepper->pwm_channel = pwm;
  stepper->state = BEGIN;                                                                                                       
  stepper->recipe_number = current_recipe;  
  stepper->PC = 0;
  stepper->WC = 0;
  move(stepper,0);
}


/*
 * Header: move the motor
 *
 * Params: stepper motor 1 or 2, position is the current position 
 * Return: void
 */
void move(struct Stepper *stepper,UINT8 position)
{
  stepper->position = position;
  
  switch(stepper->pwm_channel) 
  { 
    case PWM_CHANNEL0:
      // For polarity equals 1 Duty cycle= [PWMDTY0/PWMPER0]* 100- calculate later 
      // 5 for 0, 16 for 90, 27 for 180
      if( position < STEPPER_POSITIONS) 
      {
        PWMDTY0 = duty_for_position[position];
      }
      break;
      
    case PWM_CHANNEL1:
      if( position < STEPPER_POSITIONS) 
      {
        PWMDTY1 = duty_for_position[position];
      }
      break;
  }
    
}

/*
 * Header: run the next instruction 
 *
 * Params: stepper motor 1 or 2
 * Return: void
 */
void run_next_command(struct Stepper *stepper) 
{
    UINT8 command = recipe[stepper->recipe_number][stepper->PC];
    UINT8 opcode = command & 0xE0;
    UINT8 parameter = command & 0x1F;
    
    //printf("command %d opcode %d parameter %d\r\n",command,opcode,parameter);
    
    switch(opcode) 
    {
      case MOV:
        //check parameter validity
        if(STEPPER_POSITIONS <= parameter ) 
        {
          //printf("Recipe command error encountered");
          stepper->error_encountered = RECIPE_COMMAND_ERROR;
          stepper->state = ERROR;    
        } 
        else
        {
          //printf("Moving to %d \r\n",parameter);
          move(stepper,parameter);
          stepper->PC++;
        }
        break;
        
      case WAIT:
        if(0 == stepper->WC)
        {
          stepper->WC = parameter;
        }
        else
        {
          stepper->WC--;
          if(0 == stepper->WC)
          {
            stepper->PC++;
          }
        }
        //printf("Waiting for %d \r\n",stepper->WC);
        break;
      
      case START_LOOP: 
        if(0 != stepper->LPC)
        {
          stepper->error_encountered = NESTED_LOOP_ERROR;
          stepper->state = ERROR;    
        }
        else
        {  
          //printf("Loop started for %d times \r\n",parameter);
          stepper->LPC = parameter;
          stepper->LPS = stepper->PC + 1;
          stepper->PC++;
        }
        break;
      
      case END_LOOP:
        if(0 == stepper->LPC)
        {
          //printf("Loop completed \r\n");
          stepper->PC++;
        } 
        else
        {
          stepper->LPC--;
          stepper->PC = stepper->LPS;
          //printf("will Loop %d more times \r\n",stepper->LPC); 
        }
        break;
      
      case END:
        stepper->state = RECIPE_END;
        //printf("Done \r\n");
        break;
        
      case LOAD:
        if(NO_OF_RECIPES >= parameter)
        {
          stepper->PC = 0;
          stepper->recipe_number = parameter;
          stepper->state = RUN;
        } 
        else 
        {
          stepper->error_encountered = RECIPE_COMMAND_ERROR;
          stepper->state = ERROR;
        }
        break;
        
      
      default:
        stepper->error_encountered = RECIPE_COMMAND_ERROR;
        stepper->state = ERROR;
        break;
    }
}
  
/*
 * Header: take appropriate action 
 *
 * Params: stepper motor 1 or 2
 * Return: void
 */
UINT8 take_action(struct Stepper *stepper) 
{
  UINT8 LED_FLAGS = 0x00;
   
  switch(stepper->state)
  {
    case RUN:
      run_next_command(stepper);
      break;
    
    case BEGIN:
      //use next move
      if('L' == stepper->next_move || 'l' == stepper->next_move ) 
      {
        if(STEPPER_POSITIONS - 1 > stepper->position)
        {
          move(stepper,stepper->position + 1);
        }
      }
      
      //use next move
      if('R' == stepper->next_move || 'r' == stepper->next_move ) 
      {
        if( 0 < stepper->position)
        {
          move(stepper,stepper->position - 1);
        }
      }
        
      stepper->next_move = '\0';
      break;
        
    case PAUSE:
      //use next move
      if('L' == stepper->next_move || 'l' == stepper->next_move ) 
      {
        if(STEPPER_POSITIONS - 1 > stepper->position)
        {
          move(stepper,stepper->position + 1);
        }
      }
        
      //use next move
      if('R' == stepper->next_move || 'r' == stepper->next_move ) 
      {
        if( 0 < stepper->position)
        {
          move(stepper,stepper->position - 1);
        }
      }
      stepper->next_move = '\0';
      //glow LED
      LED_FLAGS = RECIPE_PAUSE_LED;
      break;
    
    case RECIPE_END:
      //glow LED
      LED_FLAGS = RECIPE_END_LED;
      break;
    
    case ERROR:
      //glow appropriate LED using error encountered variable
      if(RECIPE_COMMAND_ERROR == stepper->error_encountered)
      {  
        LED_FLAGS = COMMAND_ERROR_LED;
      }
      else if(NESTED_LOOP_ERROR == stepper->error_encountered) 
      {
        LED_FLAGS = NESTED_ERROR_LED;
      }
      break;
  }
  return LED_FLAGS;
}

/*
 * Header: update the state
 *
 * Params: stepper motor 1 or 2, user input
 * Return: void
 */

void update_state(struct Stepper *stepper, UINT8 input) 
{
  switch(stepper->state)
  {
    case BEGIN:
      switch(input)
      {
        case 'L':
        case 'l':
        case 'R':
        case 'r':
          stepper->next_move = input;
          break;
          
        case 'P':
        case 'p':
        case 'N':
        case 'n':
          break;
          
        case 'C':
        case 'c':
        case 'B':
        case 'b':
          stepper->state = RUN;
          break;
      }
      break;
    
    case RUN:
      switch(input)
      {
        case 'L':
        case 'l':
        case 'R':
        case 'r':
        case 'C':
        case 'c':
        case 'N':
        case 'n':
          break;
          
        case 'P':
        case 'p':
          stepper->state = PAUSE;
          break;
          
        case 'B':
        case 'b':
          stepper->PC = 0;
          stepper->state = RUN;
          break;
      }
      break;
    
    case PAUSE:
      switch(input)
      {
        case 'L':
        case 'l':
        case 'R':
        case 'r':
          stepper->next_move = input;
          break;
          
        case 'P':
        case 'p':
        case 'N':
        case 'n':
          break;
             
        case 'C':
        case 'c':
          stepper->state = RUN;
          break;
          
        case 'B':
        case 'b':
          stepper->PC = 0;
          stepper->state = RUN;
          break;
      }
      break;
    
    case RECIPE_END:
      switch(input)
      {
        case 'L':
        case 'l':
        case 'R':
        case 'r':
        case 'N':
        case 'n':
        case 'C':
        case 'c':
          break;
          
        case 'B':
        case 'b':
          stepper->PC = 0;
          stepper->state = RUN;
          break;
      }
      break;
      
    case ERROR:
      switch(input) 
      {
        case 'L':
        case 'l':
        case 'R':
        case 'r':
        case 'N':
        case 'n':
        case 'C':
        case 'c':
          break;
        
        case 'B':
        case 'b':
          stepper->PC = 0;
          stepper->state = RUN;
          break;
      }
      break;
  }
}