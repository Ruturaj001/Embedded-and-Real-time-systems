#include "post.h"

/*
 * Header: POST test on the outputl
 *
 * Params: void
 * Return: void
 */

UINT8 POST() 
{
  if(1 == timer_check(WAIT_CYCLES)) 
  {
    return 1;
  }
  
  return 0;
}