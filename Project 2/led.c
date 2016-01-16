
#include "led.h"


/*
 * Header: Turns on the leds
 *
 * Params: flag 1 PORTB, flag2 PORTA
 * Return: void
 */
void glow_led(UINT8 flag1, UINT8 flag2)
{
  PORTB = flag1;
  PORTA = flag2;
}

/*
 * Header: Initializes the leds
 *
 * Params: void
 * Return: void
 */

void IntializeLED(void)
{
  DDRA = 0xFF;
  DDRB = 0xFF;
  PORTA = 0x00;
  PORTB = 0x00;
}