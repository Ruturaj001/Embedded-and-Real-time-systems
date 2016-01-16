#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>       /* for sleep() */
#include <stdint.h>       /* for uintptr_t */
#include <hw/inout.h>     /* for in*() and out*() functions */
#include <sys/neutrino.h> /* for ThreadCtl() */
#include <sys/mman.h>     /* for mmap_device_io() */
#include <time.h>
#include <sys/syspage.h>  /* for for cycles_per_second */
#include <pthread.h>
#include <signal.h>
#include <sys/netmgr.h>

// IO port used here corresponds to a single register, which is
// one byte long
#define PORT_LENGTH 1

// The data acquisition port starts at 0x280.
// The 8th byte is port A
// the 9th byte is for port B
#define DATA_ADDRESS_A 0x288
#define DATA_ADDRESS_B 0x289

// Control address which sets port A and B as Input or output is at offset 0xB
#define CTRL_ADDRESS 0x28B

// bit 2 = 0 to set port B as output and 1 to set port B as input
// bit 4 = 0 to set port A as output and 1 to set port A as input
#define INIT_BIT 0x00

// values to set for port A and B
#define LOW 0x00
#define HIGH 0xFF

#define STEPPER_POSITIONS 6

// PWM channels to use for servo
// 0 represents port A and 1 represents port B
#define PWM_CHANNEL0 0
#define PWM_CHANNEL1 1

// Mnemonic defination
#define END ((unsigned char) 0x00 << 5)
#define MOV ((unsigned char) 0x01 << 5)
#define WAIT ((unsigned char) 0x02 << 5)
#define START_LOOP ((unsigned char) 0x04 << 5)
#define END_LOOP ((unsigned char) 0x05 << 5)
#define LOAD ((unsigned char) 0x06 << 5)

// constants to keep track of input validity
#define INVALID_INPUTS 0
#define VALID_INPUTS 1

// Recipe to run on each servo
#define STEPPER1_RECIPE 0
#define STEPPER2_RECIPE 4

typedef union {
	struct _pulse pulse;
}my_message_t;

// State of servo
typedef enum State
{
	BEGIN = 0,
			RUN,
			PAUSE,
			RECIPE_END,
			ERROR
};

// type of error encountered
// used in 2a to glow appropriate LEDs
// not used in QNX
typedef enum ERROR_ENCOUTERED
{
	NO_ERROR = 0,
			RECIPE_COMMAND_ERROR,
			NESTED_LOOP_ERROR
};

struct Stepper
{
	// pointer to script to be ran on stepper
	unsigned char recipe_number;

	// state
	enum State state;

	// Current position of stepper
	unsigned char position;

	// Program counter
	unsigned char PC;

	// Loop start position
	unsigned char LPS;

	// Loop counter
	unsigned char LPC;

	// PWM channel
	unsigned char pwm_channel;

	enum ERROR_ENCOUTERED error_encountered;

	//if paused then next step
	unsigned char next_move;
};

struct timespec SERVO1DELAY1;
struct timespec SERVO2DELAY1;

struct Stepper stepper1,stepper2;

int high_for_position[STEPPER_POSITIONS] = {100000,700000,1000000,1300000,1600000,2000000};
unsigned char **recipe;

// handles for port A, port B and control register
uintptr_t ctrl_handle;
uintptr_t data_handle_A;
uintptr_t data_handle_B;

/*
 * Initializes the recipe table
 *
 * Params: void
 * Return: void
 */

void InitializeRecipe()
{
	recipe = malloc(sizeof(unsigned char *) * 9);


	/* Verify default time delay */
	recipe[0] = malloc(sizeof(unsigned char) * 4);
	recipe[0][0] = MOV|0;
	recipe[0][1] = MOV|5;
	recipe[0][2] = MOV|0;
	recipe[0][3] = END;

	/* Verify default loop behavior */
	recipe[1] = malloc(sizeof(unsigned char) * 7);
	recipe[1][0] = MOV|3;
	recipe[1][1] = START_LOOP|0;
	recipe[1][2] = MOV|1;
	recipe[1][3] = MOV|4;
	recipe[1][4] = END_LOOP;
	recipe[1][5] = MOV|0;
	recipe[1][6] = END;

	/* Verify wait 0 */
	recipe[2] = malloc(sizeof(unsigned char) * 4);
	recipe[2][0] = MOV|2;
	recipe[2][1] = WAIT|0;
	recipe[2][2] = MOV|3;
	recipe[2][3] = END;

	/* 9.3 second delay */
	recipe[3] = malloc(sizeof(unsigned char) * 7);
	recipe[3][0] = MOV|2;
	recipe[3][1] = MOV|3;
	recipe[3][2] = WAIT|31;
	recipe[3][3] = WAIT|31;
	recipe[3][4] = WAIT|31;
	recipe[3][5] = MOV|4;
	recipe[3][6] = END;

	/* Every position */
	recipe[4] = malloc(sizeof(unsigned char) * 12);
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
	recipe[5] = malloc(sizeof(unsigned char) * 3);
	recipe[5][0] = END;
	recipe[5][1] = MOV|3;
	recipe[5][2] = END;

	/* Recipe command error */
	recipe[6] = malloc(sizeof(unsigned char) * 4);
	recipe[6][0] = MOV|0;
	recipe[6][1] = MOV|5;
	recipe[6][2] = MOV|6;
	recipe[6][3] = END;

	/* Nested Loop Error */
	recipe[7] = malloc(sizeof(unsigned char) * 2);
	recipe[7][0] = START_LOOP|1;
	recipe[7][1] = START_LOOP|1;
	recipe[7][2] = END;

	/* Recipe with new OPcode */
	recipe[8] = malloc(sizeof(unsigned char) * 3);
	recipe[8][0] = MOV|5;
	recipe[8][1] = WAIT|2;
	recipe[8][2] = LOAD|4;

}

/*
 * Initialization of stepper struct
 *
 * Params: stepper motor 1 or 2, pwm, current recipe - recipe being loaded
 * Return: void
 */

void set_stepper(struct Stepper *stepper, unsigned char pwm, unsigned char current_recipe)
{
	stepper->pwm_channel = pwm;
	stepper->state = RUN;
	stepper->recipe_number = current_recipe;
	stepper->PC = 0;
	stepper->position = 0;
}


/*
 * moves the motor by changing pulse width
 *
 * Params: stepper motor 1 or 2, position is the current position
 * Return: void
 */
void move(struct Stepper *stepper,unsigned char position)
{
    stepper->position = position;

    // The change in delay will result in change in pulse width of respective
    // servo
	switch(stepper->pwm_channel)
	{
	case PWM_CHANNEL0:
		if( position < STEPPER_POSITIONS)
		{
			SERVO1DELAY1.tv_nsec = high_for_position[position];
		}
		break;

	case PWM_CHANNEL1:
		if( position < STEPPER_POSITIONS)
		{
			SERVO2DELAY1.tv_nsec = high_for_position[position];
		}
		break;
	}

}

/*
 * runs the next instruction
 *
 * Params: stepper motor 1 or 2
 * Return: void
 */
void run_next_command(struct Stepper *stepper)
{
	unsigned char command = recipe[stepper->recipe_number][stepper->PC];
	unsigned char opcode = command & 0xE0;
	unsigned char parameter = command & 0x1F;
	struct timespec temp;
	int diff = 0;

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
            // scale the wait depending upon difference in current position and next position
			//printf("Moving to %d \r\n",parameter);
			if(parameter > stepper->position)
			{
				diff = parameter - stepper->position;
			}
			else
			{
				diff = stepper->position - parameter;
			}
			move(stepper,parameter);
			temp.tv_sec = (diff * 200)/1000;
			temp.tv_nsec = ((diff * 200)%1000) * 1000000;
			clock_nanosleep(CLOCK_REALTIME,0,&temp,NULL);
			stepper->PC++;
		}
		break;

	case WAIT:
		temp.tv_sec = parameter / 10;
		temp.tv_nsec = (parameter % 10) * 100000000;
		clock_nanosleep(CLOCK_REALTIME,0,&temp,NULL);
		//printf("Waiting for %d seconds and %d nano seconds\r\n",parameter / 10,(parameter % 10) * 100000000);
		stepper->PC++;
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
 * takes appropriate action
 *
 * Params: stepper motor 1 or 2
 * Return: void
 */
void take_action(struct Stepper *stepper)
{
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
		break;

	case RECIPE_END:
		break;

	case ERROR:
		break;
	}
}

/*
 * updates the state of servo based on user input and current state
 * (as per state diagram)
 *
 * Params: stepper motor 1 or 2, user input
 * Return: void
 */

void update_state(struct Stepper *stepper, unsigned char input)
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

// This functions does required variable initialization and sets the ports
void setup()
{
	// Get a handle to the parallel port's Control register
	ctrl_handle = mmap_device_io( PORT_LENGTH, CTRL_ADDRESS );

	// Initialize the data acquisition port
	out8( ctrl_handle, INIT_BIT );

	// Get a handle to the parallel port's Data register
	data_handle_A = mmap_device_io( PORT_LENGTH, DATA_ADDRESS_A );
	data_handle_B = mmap_device_io( PORT_LENGTH, DATA_ADDRESS_B );

    // initialize stepper struct variables
	set_stepper(&stepper1,PWM_CHANNEL_STEPPER1,STEPPER1_RECIPE);
	set_stepper(&stepper2,PWM_CHANNEL_STEPPER2,STEPPER2_RECIPE);

    // allocate memoery and intialize recipe
	InitializeRecipe();
}

// This thread controls PWM for 2nd servo
void *PWM1(void* empty)
{
	struct sigevent         event;
	struct itimerspec       itime;
	timer_t                 timer_id;
	int                     chid, rcvid;
	my_message_t            msg;

	chid = ChannelCreate(0);

    // following code is used to get kick every pulse period time
    // which is 20ms
	event.sigev_notify = SIGEV_PULSE;
	event.sigev_coid = ConnectAttach(ND_LOCAL_NODE, 0,
			chid,
			_NTO_SIDE_CHANNEL, 0);
	event.sigev_priority = getprio(0);
	event.sigev_code = _PULSE_CODE_MINAVAIL;
	timer_create(CLOCK_REALTIME, &event, &timer_id);

    // 20 ms to nano seconds
	itime.it_value.tv_sec = 0;
	itime.it_value.tv_nsec = 20000000;
	itime.it_interval.tv_sec = 0;
	itime.it_interval.tv_nsec = 20000000;
	timer_settime(timer_id, 0, &itime, NULL);

	SERVO1DELAY1.tv_sec = 0;
	SERVO1DELAY1.tv_nsec = 100000;

	while(1)
	{
		rcvid = MsgReceive(chid, &msg, sizeof(msg), NULL);
		if (rcvid == 0)
		{
            // make pulse high for appropriate time
			out8( data_handle_A, HIGH );
			InterruptDisable();
			nanospin(&SERVO1DELAY1);
			InterruptEnable();
			out8( data_handle_A, LOW );
		}
	}
}

// This thread controls PWM for 2nd servo
void *PWM2(void* empty)
{
	struct sigevent         event;
	struct itimerspec       itime;
	timer_t                 timer_id;
	int                     chid, rcvid;
	my_message_t            msg;

	chid = ChannelCreate(0);

    // following code is used to get kick every pulse period time
    // which is 20ms
	event.sigev_notify = SIGEV_PULSE;
	event.sigev_coid = ConnectAttach(ND_LOCAL_NODE, 0,
			chid,
			_NTO_SIDE_CHANNEL, 0);
	event.sigev_priority = getprio(0);
	event.sigev_code = _PULSE_CODE_MINAVAIL;
	timer_create(CLOCK_REALTIME, &event, &timer_id);

    // 20 ms to nano seconds
	itime.it_value.tv_sec = 0;
	itime.it_value.tv_nsec = 20000000;
	itime.it_interval.tv_sec = 0;
	itime.it_interval.tv_nsec = 20000000;
	timer_settime(timer_id, 0, &itime, NULL);

	SERVO2DELAY1.tv_sec = 0;
	SERVO2DELAY1.tv_nsec = 100000;

	while(1)
	{
		rcvid = MsgReceive(chid, &msg, sizeof(msg), NULL);
		if (rcvid == 0)
		{
            // make pulse high for appropriate time
			out8( data_handle_B, HIGH );
			InterruptDisable();
			nanospin(&SERVO2DELAY1);
			InterruptEnable();
			out8( data_handle_B, LOW );
		}
	}
}

// to control first servo
void *servo1(void* empty)
{
	struct timespec temp;
	temp.tv_sec = 0;
	temp.tv_nsec = 100000000;
	while(1)
	{
		take_action(&stepper1);
		clock_nanosleep(CLOCK_REALTIME,0,&temp,NULL);
	}
}

// to control second servo
void *servo2(void* empty)
{
	struct timespec temp;
	temp.tv_sec = 0;
	temp.tv_nsec = 100000000;
	while(1)
	{
		take_action(&stepper2);
		clock_nanosleep(CLOCK_REALTIME,0,&temp,NULL);
	}
}

int main( )
{
	int privity_err;
	unsigned char userInput1,userInput2;
	int input_validity;

	pthread_t pwm_thread1,pwm_thread2;
	pthread_t servo1_thread,servo2_thread;

	// Give this thread root permissions to access the hardware
	privity_err = ThreadCtl( _NTO_TCTL_IO, NULL );
	if ( privity_err == -1 )
	{
		fprintf( stderr, "can't get root permissions\n" );
		return -1;
	}

	// set port and initialize required variable values
	setup();

	// start parking sensor
	pthread_create(&pwm_thread1, NULL, &PWM1, NULL);
	pthread_create(&pwm_thread2, NULL, &PWM2, NULL);
	pthread_create(&servo1_thread, NULL, &servo1, NULL);
	pthread_create(&servo2_thread, NULL, &servo2, NULL);

	for(;;)
	{
		(void)printf(">");
		do
		{
			userInput1 = getchar();

			userInput2 = getchar();

			getchar();

			if( 'x' == userInput1 || 'X' == userInput1 || 'x' == userInput2 || 'X' == userInput2 )
			{
				input_validity = INVALID_INPUTS;
			}
			else
			{
				input_validity = VALID_INPUTS;
			}

		}while(INVALID_INPUTS == input_validity);

		//set state of stepper1
		update_state(&stepper1,userInput1);

		//set state of stepper2
		update_state(&stepper2,userInput2);

	}

	pthread_join(pwm_thread1,NULL);
	pthread_join(pwm_thread2,NULL);
	pthread_join(servo1_thread,NULL);
	pthread_join(servo2_thread,NULL);

	return 0;
}
