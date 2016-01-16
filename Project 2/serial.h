#ifndef _serial_
#define _serial_

#include <stdio.h>      /* Standard I/O Library */
#include <hidef.h>      /* common defines and macros */

#include "types.h"
#include "derivative.h" /* derivative-specific definitions */


void InitializeSerialPort(void);
void TERMIO_PutChar(INT8 ch);
UINT8 BufferEmpty(void);
UINT8 GetChar(void);

#endif