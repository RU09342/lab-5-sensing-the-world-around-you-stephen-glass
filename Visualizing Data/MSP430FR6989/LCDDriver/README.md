# MSP430FR6989 Simple LCD Driver
This library is meant to make writing simple characters to the on-board LCD display of the MSP430FR6989 Launchpad simple.

## Dependencies
* msp430.h
This header file must be called before calling this one, otherwise you might get errors with certain macros not being defined. 

### Including the library
All you need to do is add the LCDDriver.c and LCDDriver.h files to your project and in the first few lines of your main file, you need to include the new .h file.
```c
#include <msp430.h>
#include "LCDDriver.h"
```

## Usage

### Functions
Currently, this library supports one main function which takes in an ASCII Alphanumeric character or space and displays it on the corresponding LCD segment.
```c
showChar(char c, int position)
```
#### Inputs
+ c is the input ASCII Character which you want to display ('A-Z', 'a-z', '0-9', ' ')
+ position is which LCD digit you want to put the character on starting with 1 on the left and 6 on the right.

#### Outputs
If you have a character within the accepted range of ASCII characters, you should see that character pop up on the screen after this line of code is called. If an ASCII character not within the range is passed in, it will turn all segments of that particular digit on to indicate an error. Please reference the [MSP430FR6989 Launch Pad Data Sheet](http://www.ti.com/lit/ug/slau627a/slau627a.pdf) for more information about which registers are being controlled in this library.

### Example
If you wanted to show whatever character you just received over a UART communication on the right most digit (Digit 6) of the LCD, you could simply add showChar() to your UART ISR.
```c
#pragma vector=USCI_A0_VECTOR
__interrupt void USCI_A0_ISR(void)
{
  switch(__even_in_range(UCA0IV,USCI_UART_UCTXCPTIFG))
  {
    case USCI_NONE: break;
    case USCI_UART_UCRXIFG:
      while(!(UCA0IFG&UCTXIFG));
      UCA0TXBUF = UCA0RXBUF;    // Echo back the character received
      showChar((UCA0RXBUF & 0x00FF), 6);  // Truncate and Display Character on LCD Digit 6
      break;
    case USCI_UART_UCTXIFG: break;
    case USCI_UART_UCSTTIFG: break;
    case USCI_UART_UCTXCPTIFG: break;
    default: break;
  }
}
```

## Copyright Disclaimer
```c
/* --COPYRIGHT--,BSD_EX
 * Copyright (c) 2014, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *******************************************************************************
 *
 *                       MSP430 CODE EXAMPLE DISCLAIMER
 *
 * MSP430 code examples are self-contained low-level programs that typically
 * demonstrate a single peripheral function or device feature in a highly
 * concise manner. For this the code may rely on the device's power-on default
 * register values and settings such as the clock configuration and care must
 * be taken when combining code from several examples to avoid potential side
 * effects. Also see www.ti.com/grace for a GUI- and www.ti.com/msp430ware
 * for an API functional library-approach to peripheral configuration.
 *
 * --/COPYRIGHT--*/
```