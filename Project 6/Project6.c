#include <stdint.h>			/* for uintptr_t */
#include <hw/inout.h>		/* for in*() and out*() functions */
#include <sys/neutrino.h>	/* for ThreadCtl() */
#include <sys/mman.h>		/* for mmap_device_io() */
#include <math.h>

#define INIT_BIT 0x00

#define BASE 0x280
#define PORT_LENGTH 1
#define PORTA 0x288
#define PORTB 0x289

#define MIN_VOLTAGE -5
#define MAX_VOLTAGE 5

#define OUT_OF_RANGE_VOLTAGE_WARNING 0x01
#define AD_CONVERSION_FAULT_WARNING 0x02
#define POST_FAILED_WARNING 0x04

/*
 * This method wait for conversion to finish,
 * with timeout
 * returns
 * 0  if conversion completed
 * -1 if timeout
 */
int checkstatus(uintptr_t adc_gain_ctrl_handle)
{
	int i;
	for(i=0;i<10000;i++)
	{
		if((in8(adc_gain_ctrl_handle) & 0x80) ==0)
		{
			return 0;
		}
	}
	return -1;
}

/*
 * main method
 * it measures volts and sends it through port A
 * if error occurs, sends error code through port B
 */
int main(void)
{
	int privity_err;
	char LSB,MSB;
	int data;
	float voltage;
	int hardware_fault = 0;
    int post_error = 0;

	/* Root permissions to access the hardware */
	privity_err = ThreadCtl(_NTO_TCTL_IO, NULL);
	if (privity_err == -1)
	{
        return -1;
	}
	// Get a handle for required ports
	uintptr_t porta_handle = mmap_device_io(PORT_LENGTH, PORTA);
	uintptr_t portb_handle = mmap_device_io(PORT_LENGTH, PORTB);
	uintptr_t LSB_ctrl_handle = mmap_device_io(PORT_LENGTH, BASE);
	uintptr_t MSB_ctrl_handle = mmap_device_io(PORT_LENGTH, BASE + 1);
	uintptr_t adc_channel_ctrl_handle = mmap_device_io(PORT_LENGTH, BASE + 2);
	uintptr_t adc_gain_ctrl_handle = mmap_device_io(PORT_LENGTH, BASE + 3);
	uintptr_t port_ctrl_handel = mmap_device_io(PORT_LENGTH, BASE + 0x0B);

	//
	out8(port_ctrl_handel, INIT_BIT);

	for(;;)
	{
		// Selecting channel 4
		out8(adc_channel_ctrl_handle, 0x44);

		// Selecting the input range +10 to -10
		out8(adc_gain_ctrl_handle, 0x00);

		// Wait for analog input circuit to settle
		while(in8(adc_gain_ctrl_handle) & 0x20){}

		// Performs an A/D Conversion on current channel
		out8(LSB_ctrl_handle, 0x80);

		//Wait for conversions to finish
		if (checkstatus(adc_gain_ctrl_handle) == -1)
		{
			hardware_fault = 1;
		}

		//Read the data
		LSB = in8(LSB_ctrl_handle);
		MSB = in8(MSB_ctrl_handle);
		data = (MSB*256) + LSB;

		//Converting the data to voltage
		voltage = (float)(data * 10)/32768;

		// check for error
		if ((voltage < MIN_VOLTAGE) || (voltage > MAX_VOLTAGE) || (hardware_fault == 1))
		{
			if((voltage < MIN_VOLTAGE || voltage > MAX_VOLTAGE) && hardware_fault)
			{
				// if both errors occurred
				out8(portb_handle, OUT_OF_RANGE_VOLTAGE_WARNING + AD_CONVERSION_FAULT_WARNING);
			}
			else if(voltage < MIN_VOLTAGE || voltage > MAX_VOLTAGE)
			{
				// if just out of range error occurred
				out8(portb_handle, OUT_OF_RANGE_VOLTAGE_WARNING);
			}
			else
			{
				// if just hardware fault occurred
				out8(portb_handle, AD_CONVERSION_FAULT_WARNING);
			}
		}
		else
		{
			unsigned short motor_position = fabsf(((voltage + 5) / 10) * 22) + 5;
			out8(porta_handle, motor_position);
			out8(portb_handle, 0x00);
		}
	}
}
