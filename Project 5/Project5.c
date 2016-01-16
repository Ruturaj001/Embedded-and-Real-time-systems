// This code measures and prints distance to obstacle using ultrasonic sensor
// Author Ruturaj Hagawane

#include <stdio.h>
#include <unistd.h>       /* for sleep() */
#include <stdint.h>       /* for uintptr_t */
#include <hw/inout.h>     /* for in*() and out*() functions */
#include <sys/neutrino.h> /* for ThreadCtl() */
#include <sys/mman.h>     /* for mmap_device_io() */
#include <time.h>
#include <sys/syspage.h>  /* for for cycles_per_second */
#include <pthread.h>

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
#define INIT_BIT 0x02

#define LOW 0x00
#define HIGH 0xFF

// gear status values
#define IN_REVERSE 0
#define NOT_IN_REVERSE 1

// maximum and minimum values sensor can detect
#define RANGE_LOWER_BOUND 1
#define RANGE_UPPER_BOUND 196

// handles for port A, port B and control register
uintptr_t ctrl_handle;
uintptr_t data_handle_A;
uintptr_t data_handle_B;

// Variable to store cycles per second by CPU
uint64_t cps;

// variable to store current gear status
int gear_status;

// This functions does required variable initialization and sets the ports
void setup()
{
	cps  = SYSPAGE_ENTRY(qtime)->cycles_per_sec;

	// Get a handle to the parallel port's Control register
	ctrl_handle = mmap_device_io( PORT_LENGTH, CTRL_ADDRESS );

	// Initialize the data acquisition port
	out8( ctrl_handle, INIT_BIT );

	// Get a handle to the parallel port's Data register
	data_handle_A = mmap_device_io( PORT_LENGTH, DATA_ADDRESS_A );
	data_handle_B = mmap_device_io( PORT_LENGTH, DATA_ADDRESS_B );
}

// This function calculates distance to obstacle
unsigned int ping()
{
	struct timespec delay;
	struct timespec rem;

	uint64_t before = 0;
	uint64_t after = 0;

	double round_trip_time = 0;
	double distance = 0;
	unsigned int rounded_distance = 0;

	// Speed of sound in inches per second
	double speed = 13544.08;

	// setting small delay to 10 micro seconds
	delay.tv_sec = 0;
	delay.tv_nsec = 10000;

	// Output a byte of highs to the data lines
	out8( data_handle_A, HIGH );

	nanosleep(&delay,&rem);

	// Output a byte of lows to the data lines
	out8( data_handle_A, LOW );

	// waits for echo to go high
	while ( 0 == (in8(data_handle_B)&0x01) ){}

	before = ClockCycles();

	// waits till echo is high
	while ( 1 == (in8(data_handle_B)&0x01) ){}

	after = ClockCycles();

	// measure how long echo was high
	round_trip_time = (double)(after - before) / (double)cps;

	// calculate distances to obstacle
	distance = speed * round_trip_time / 2;

	rounded_distance = (unsigned int)(distance+0.5);

	return rounded_distance;
}

// This thread checks if car is still in reverse and if yes finds distance
// to obstacle
void *parking_sensor(void* empty)
{
	struct timespec delay;
	struct timespec rem;
	unsigned int maxPingValue = RANGE_LOWER_BOUND;
	unsigned int minPingValue = RANGE_UPPER_BOUND;

	//setting delay to 90 mili seconds
	delay.tv_sec = 0;
	delay.tv_nsec = 90000000;

	while(IN_REVERSE == gear_status)
	{
		unsigned int pingValue = ping();
		if(pingValue >= RANGE_LOWER_BOUND && pingValue <= RANGE_UPPER_BOUND)
		{
			if(pingValue > maxPingValue)
			{
				maxPingValue = pingValue;
			}

			if(pingValue < minPingValue)
			{
				minPingValue = pingValue;
			}

			printf( "%d\n",pingValue);
		}
		else
		{
			printf( "*\n");
		}
		nanosleep(&delay,&rem);
	}

	//print max and min
	printf( "MAX PING VALUE: %d\n",maxPingValue);
	printf( "MIN PING VALUE: %d\n",minPingValue);
}

int main( )
{
	int privity_err;
	char input = 0;

	pthread_t parking_sensor_thread;

	// Give this thread root permissions to access the hardware
	privity_err = ThreadCtl( _NTO_TCTL_IO, NULL );
	if ( privity_err == -1 )
	{
		fprintf( stderr, "can't get root permissions\n" );
		return -1;
	}

	// set port and initialize required variable values
	setup();

	printf( "Press R/r to put car in reverse\n" );
	printf( "Press D/d when done\n" );

	//get user input the put in reverse
	do
	{
		input = getchar();
	}while('R' != input && 'r'!= input);

	// so that other thread will know we are in reverse
	gear_status = IN_REVERSE;

	printf("Starting parking sensor\n");

	// start parking sensor
	pthread_create(&parking_sensor_thread, NULL, &parking_sensor, NULL);

	// get user input when he is out of reverse
	do
	{
		input = getchar();
	}while('D' != input && 'd'!= input);

	// so that other thread will know we are no longer in reverse
	gear_status = NOT_IN_REVERSE;

	pthread_join(parking_sensor_thread,NULL);

	return 0;
}
