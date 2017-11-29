/*
Stephen Glass
msp430fr2311_ldr.c : control a LED by using a light dependent resistor
MSP430FR2311
*/

#include <msp430.h>

#define FRSERIES        1
#define MCLK_FREQ_MHZ   8   // MCLK = 8MHz

void initGPIO();
void initializeTimer(int capture);
void initADC();
void initUART();

unsigned int ADC_Result;
char MSB    = 0;    // 8-bit integer
char LSB    = 0;    // 8-bit integer

int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;                                 // Stop WDT
     // Disable the GPIO power-on default high-impedance mode to activate
    // previously configured port settings
    #if defined(FRSERIES)
        PM5CTL0 &= ~LOCKLPM5;
    #endif

    initGPIO();                                              // Configure GPIO
    initUART();
    initADC();
    __delay_cycles(400);                                     // Delay for reference settling
    initializeTimer(25);                                     // 25 Hertz timer

    __bis_SR_register(LPM0_bits | GIE);                      // LPM0, ADC_ISR will force exit
    __no_operation();
}

void initGPIO(void)
{
    // Configure GPIO
    P1DIR |= BIT0;                                            // Set P1.0/LED to output direction
    P1OUT &= ~BIT0;                                           // P1.0 LED off
}

void initADC(void)
{
    // Configure ADC A2 pin
    P1SEL0 |= BIT2;
    P1SEL1 |= BIT2;

    // Configure ADC10
    ADCCTL0 |= ADCSHT_2 | ADCON;                              // ADCON, S&H=16 ADC clks
    ADCCTL1 |= ADCSHP;                                        // ADCCLK = MODOSC; sampling timer
    ADCCTL2 |= ADCRES;                                        // 10-bit conversion results
    ADCIE |= ADCIE0;                                          // Enable ADC conv complete interrupt
    ADCMCTL0 |= ADCINCH_2;                                    // A2 ADC input select; Vref=AVCC

    // Configure reference
    PMMCTL0_H = PMMPW_H;                                      // Unlock the PMM registers
    PMMCTL2 |= INTREFEN;                                      // Enable internal reference
}

void initializeTimer(int hertz) // Seconds = 1/Hertz, 10Hz = 0.1s
{
    TB0CCTL0 = CCIE;
    TB0CTL = TBSSEL_2 + MC_1 + ID_3; // SMCLK/8, UPMODE
    // CLK/HERTZ = CAPTURE
    // CLK = 1MHZ/8 = 125kHz
    int capture = (125000)/hertz;
    TB0CCR0 = capture; // (1000000/8)/(12500) = 10 Hz = 0.1 seconds
}

void initUART(void)
{
    // Enable and disable GPIO for UART
    P1DIR = 0xFF; P2DIR = 0xFF;
    P1REN = 0xFF; P2REN = 0xFF;
    P1OUT = 0x00; P2OUT = 0x00;

    __bis_SR_register(SCG0);                 // disable FLL
    CSCTL3 |= SELREF__REFOCLK;               // Set REFO as FLL reference source
    CSCTL1 = DCOFTRIMEN_1 | DCOFTRIM0 | DCOFTRIM1 | DCORSEL_3;// DCOFTRIM=3, DCO Range = 8MHz
    CSCTL2 = FLLD_0 + 243;                  // DCODIV = 8MHz
    __delay_cycles(3);
    __bic_SR_register(SCG0);                // enable FLL
    CSCTL4 = SELMS__DCOCLKDIV | SELA__REFOCLK; // set default REFO(~32768Hz) as ACLK source, ACLK = 32768Hz
    // default DCODIV as MCLK and SMCLK source

    // Configure UART pins
    P1SEL0 |= BIT6 | BIT7;                    // set 2-UART pin as second function

    // Configure UART
    UCA0CTLW0 |= UCSWRST;
    UCA0CTLW0 |= UCSSEL__SMCLK;

    // Baud Rate calculation
    // 8000000/(16*9600) = 52.083
    // Fractional portion = 0.083
    // User's Guide Table 17-4: UCBRSx = 0x49
    // UCBRFx = int ( (52.083-52)*16) = 1
    UCA0BR0 = 52;                             // 8000000/16/9600
    UCA0BR1 = 0x00;
    UCA0MCTLW = 0x4900 | UCOS16 | UCBRF_1;

    UCA0CTLW0 &= ~UCSWRST;                    // Initialize eUSCI
    UCA0IE |= UCRXIE;                         // Enable USCI_A0 RX interrupt
}

// Timer B0 interrupt service routine
#pragma vector=TIMER0_B0_VECTOR
__interrupt void Timer0_B0 (void)
{

    /* 
    LDR Testing 
    regular: 0x035B (859)
    light: 0x3E0 (992)
    finger cover: 0x0233 (563)
    covered shield: 0x006A (106)
    */

    ADCCTL0 |= ADCENC | ADCSC;                            // Sampling and conversion start
    // 3.3v / 1024 (2^10) = 0.003222
    // 0.0322 * 341 (0x155) = 1.09802
    // 1.09802 / 2 = 0.5V
    if (ADC_Result < 0x384) P1OUT &= ~BIT0;               // Clear P1.0 LED off (less than 900)
    else // If its greater than 900 (flashlight being shined)
    {
        MSB = ADC_Result >> 8;       // Bit Shifts 12 bits to the right by 8
        LSB = ADC_Result & 0xFF;     // ANDs the 12 bit value with 11111111, returning the LSB
        UCA0TXBUF = MSB;             // Transmits the MSB first
        while(!(UCA0IFG & UCTXIFG)); // Waits for the TX buffer to be cleared
        UCA0TXBUF = LSB;             // Transmits the LSB second
        P1OUT |= BIT0;                                   // Set P1.0 LED on
    }
}

// ADC interrupt service routine
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=ADC_VECTOR
__interrupt void ADC_ISR(void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(ADC_VECTOR))) ADC_ISR (void)
#else
#error Compiler not supported!
#endif
{
    switch(__even_in_range(ADCIV,ADCIV_ADCIFG))
    {
        case ADCIV_NONE:
            break;
        case ADCIV_ADCOVIFG:
            break;
        case ADCIV_ADCTOVIFG:
            break;
        case ADCIV_ADCHIIFG:
            break;
        case ADCIV_ADCLOIFG:
            break;
        case ADCIV_ADCINIFG:
            break;
        case ADCIV_ADCIFG:
            ADC_Result = ADCMEM0;
            __bic_SR_register_on_exit(LPM0_bits);              // Clear CPUOFF bit from LPM0
            break;
        default:
            break;
    }
}

/* Uncomment below if you want TX RX feedback for debugging */
/*#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=USCI_A0_VECTOR
__interrupt void USCI_A0_ISR(void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(USCI_A0_VECTOR))) USCI_A0_ISR (void)
#else
#error Compiler not supported!
#endif
{
  switch(__even_in_range(UCA0IV,USCI_UART_UCTXCPTIFG))
  {
    case USCI_NONE: break;
    case USCI_UART_UCRXIFG:
      while(!(UCA0IFG&UCTXIFG));
      UCA0TXBUF = UCA0RXBUF;
      __no_operation();
      break;
    case USCI_UART_UCTXIFG: break;
    case USCI_UART_UCSTTIFG: break;
    case USCI_UART_UCTXCPTIFG: break;
    default: break;
  }
}*/
