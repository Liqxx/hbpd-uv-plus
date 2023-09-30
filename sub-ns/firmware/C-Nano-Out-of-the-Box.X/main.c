/*********************************************************************
 *
 *              Water Monitor C Source Code
 *
 *********************************************************************
 * FileName:        main.c
 * Dependencies:    .h
 * Date Started:	Nov 23, 2022
 * Processor:       AVR64DD32   
 * Compiler:        MPLAB XC8 Compiler
 *
 * Notes:
 * 
 * This software is a University of Victoria edited version of Microchip's
 * C-Nano-Out-of-the-Box.X example
 * .  The original source can be found
 * here:
 * 
 * https://github.com/microchip-pic-avr-examples/avr64dd32-cnano-out-of-the-box-code-mplab-mcc/tree/master/C-Nano-Out-of-the-Box.X
 * 
 *   (c) 2018 Microchip Technology Inc. and its subsidiaries. 
 *   
 *   Subject to your compliance with these terms, you may use Microchip software and any 
 *   derivatives exclusively with Microchip products. It is your responsibility to comply with third party 
 *   license terms applicable to your use of third party software (including open source software) that 
 *   may accompany Microchip software.
 *   
 *   THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER 
 *   EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY 
 *   IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS 
 *   FOR A PARTICULAR PURPOSE.
 *   
 *   IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, 
 *   INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND 
 *   WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP 
 *   HAS BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO 
 *   THE FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL 
 *   CLAIMS IN ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT 
 *   OF FEES, IF ANY, THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS 
 *   SOFTWARE.
 *
 * Revision History:
 *
 * Author           Date    		Comment
 *---------------------------------------------------------------------
 * Nicolas Braam	Nov 23, 2022	Started, then got onto other work
 *                  Dec 21, 2022	Frame work good
 *                  Dec 31, 2022    Everything working
 *                  Jan 2, 2023     Refining everything
 *                  Jan 3, 2023     Fix TCA to PC3, more refining, tested all, fixed some small things
 * 
 *
 * Description:
 *
 * This file contains code to be programmed to the Water Monitor
 * Light Source card.  This code assumes 24MHz Fosc.
 *
 ********************************************************************/



/**********************************************************************
 * Libraries:
 **********************************************************************/

#include "mcc_generated_files/mcc.h"
#include <util/delay.h>

/**********************************************************************
 * Constant Definitions:
 **********************************************************************/

#define PWR_DELAY           50                                                  // 50ms
#define RX_DELAY            100                                                 // 100ms
#define VREF_STARTUP_TIME   (50)                                                // VREF start-up time - microseconds
#define LSB_MASK            (0x03)                                              // Mask needed to get the 2 LSb for DAC Data Register
                                                                                /* TMR_CLK = F_CPU / PRESCALER = 4MHz / 4 = 1MHz */
/**********************************************************************
 * Variable Declarations:
 **********************************************************************/

volatile uint16_t adcVal = 0;                                                   // global variable for debug purposes
typedef enum {ACTIVE, STANDBY} programs_t;

static programs_t current_program = STANDBY;

/**********************************************************************
 * Function Prototypes:
 **********************************************************************/

static void WATMON_Initialize(void);                                            // Initialize device
static void CLI_Run(void);
static void CLI_Execute_Command(uint8_t);
static void Print_Menu(void);
static void USART_to_CDC(void);
static uint8_t Unblocking_Read(void);
static uint8_t Read_Parameter(void);                                            // Receive single character
static void Set_Bias_Requested(void);                                           // Receive four characters and write to DAC
static void Send_Bias_Read(void);                                               // Reads ADC and sends value out UART
static void BoardSetStatus(uint8_t);                                            // Enable and disable hardware routines
static void SetTrigger(void);                                                   // Sets trigger source
static void SetRate(void);                                                      // Sets internal trigger source rate
static void SetLED(void);                                                       // Set LED (xor), if any set, then set sync out
static void VREF_init(void);
static void DAC0_init(void);
static void DAC0_setVal(uint16_t val);
static void ADC0_init(void);
static uint16_t ADC0_read(void);
static void TCA0_init(char speed);

/**********************************************************************
 * Interrupt Code:
 **********************************************************************/



/**********************************************************************
 * Main Routine:
 **********************************************************************/

int main (void)
{  
    SYSTEM_Initialize();
    WATMON_Initialize();                                                        // Init specifics of Wat Mon
    VREF_init();
    DAC0_init();
    DAC0_setVal(1023);                                                          // Make sure set low to start
    ADC0_init();
    USART_to_CDC();
    Print_Menu();
	
    while(1)
    {
        CLI_Run();                                                              // Check for data, and do something with it
    }
}



/*********************************************************************
 * Function:        static void WATMON_Initialize(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          Microcontroller is initialized
 *
 * Side Effects:    All interrupts and circuit operation are ignored
 *
 * Overview:        Ports and Registers in the Microcontroller are
 *					initialized to the required initial states.
 *
 * Note:            None
 ********************************************************************/

static void WATMON_Initialize(void)
{
// Start up in Disabled mode
    
// Trig distribution OE's, all out, and low

PORTC.DIRSET = PIN1_bm;                                                         // OE7 = PC1, set output, set low
PORTC.OUTCLR = PIN1_bm;

PORTC.DIRSET = PIN0_bm;                                                         // OE6 = PC0, set output, set low
PORTC.OUTCLR = PIN0_bm;

PORTF.DIRSET = PIN5_bm;                                                         // OE5 = PF5, set output, set low
PORTF.OUTCLR = PIN5_bm;

PORTF.DIRSET = PIN4_bm;                                                         // OE4 = PF4, set output, set low
PORTF.OUTCLR = PIN4_bm;

PORTF.DIRSET = PIN3_bm;                                                         // OE3 = PF3, set output, set low
PORTF.OUTCLR = PIN3_bm;

PORTF.DIRSET = PIN2_bm;                                                         // OE2 = PF2, set output, set low
PORTF.OUTCLR = PIN2_bm;

PORTF.DIRSET = PIN1_bm;                                                         // OE1 = PF1, set output, set low
PORTF.OUTCLR = PIN1_bm;

PORTF.DIRSET = PIN0_bm;                                                         // OE0 = PF0, set output, set low
PORTF.OUTCLR = PIN0_bm;

// Trig control, all out, and low

PORTD.DIRSET = PIN3_bm;                                                         // CLK_SEL = PD3, set output, set low
PORTD.OUTCLR = PIN3_bm;

PORTC.DIRSET = PIN3_bm;                                                         // TRIG1 = PC3, set output, set low
PORTC.OUTCLR = PIN3_bm;

// Power control, all out, and low

PORTD.DIRSET = PIN2_bm;                                                         // BIAS_ENABLE = PD2, set output, set low
PORTD.OUTCLR = PIN2_bm;

PORTD.DIRSET = PIN7_bm;                                                         // 3V3_SW_ENABLE = PD7, set output, set low
PORTD.OUTCLR = PIN7_bm;

PORTC.DIRSET = PIN2_bm;                                                         // 5V_SW_ENABLE = PC2, set output, set low
PORTC.OUTCLR = PIN2_bm;

}


/*********************************************************************
 * Function:        static void CLI_Run(void)  
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    Unknown yet
 *
 * Overview:        Looks for data, if it's received, then call command routine
 *
 ********************************************************************/

static void CLI_Run(void)                                                       // Looks for data
{
    uint8_t ch = 0;
    
    ch = Unblocking_Read();
    
    if((ch != '\n') && (ch != '\r') && (ch != '\0'))                            // If data is valid, execute command
    {
        CLI_Execute_Command(ch);
    }
}


/*********************************************************************
 * Function:        static void CLI_Execute_Command(uint8_t command) 
 *
 * PreCondition:    None
 *
 * Input:           Command received from UART
 *
 * Output:          None
 *
 * Side Effects:    Unknown yet
 *
 * Overview:        Configures Board
 *
 ********************************************************************/


static void CLI_Execute_Command(uint8_t command)
{
    switch(command)
    {
        case 'E':
            BoardSetStatus('E');
            break;
        case 'D':
            BoardSetStatus('D');
            break;
        case 'T':
            SetTrigger();
            break;
        case 'R':
            SetRate();
            break;
        case 'L':
            SetLED();
            break;            
        case 'S':
            Set_Bias_Requested();
            break;
        case 'Q':
            Send_Bias_Read();
            break;
        default:
            printf("\n\rInvalid Command!\n\r");
            Print_Menu();
            break;
    }
}


/*********************************************************************
 * Function:        static void Print_Menu(void) 
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    Unknown yet
 *
 * Overview:        Prints command menu
 *
 ********************************************************************/

static void Print_Menu(void)                                                    // Menu for print
{
    printf("\n\r");
    printf("Commands:\n\r");
    printf("E - (Enable) Board Active\n\r");
    printf("D - (Disable) Board Standby\n\r");
    printf("Tx - (Trigger) Enter Trigger Source: I - Internal, E - External\n\r");
    printf("Rx - (Rate) Enter Trigger Rate: S - 1.5kHz, F - 8MHz\r\n");
    printf("Lx - (LED) Enter LED number: 1-7 (465nm-235nm), or 0 for all off\r\n");
    printf("Sxxxx - (Set) Enter 10 bit Bias DAC Value: 0000-1023\r\n");
    printf("Q - (Query) Bias 12bit ADC Value is: \r\n");
}



/*********************************************************************
 * Function:        static void USART_to_CDC(void))
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    Unknown yet
 *
 * Overview:        Configures UART?
 *
 ********************************************************************/

static void USART_to_CDC(void)                                                  // Configure UART?
{
    PORTD.DIRSET = PIN4_bm;
    PORTMUX.USARTROUTEA = PORTMUX_USART0_ALT3_gc;
}


/*********************************************************************
 * Function:        static uint8_t Unblocking_Read(void) 
 *
 * PreCondition:    Port Configured
 *
 * Input:           None
 *
 * Output:          8 bits data received
 *
 * Side Effects:    Unknown yet
 *
 * Overview:        Looks for data received, if so, returns it
 *
 ********************************************************************/


static uint8_t Unblocking_Read(void)                                            // Read UART
{
    if(USART0_IsRxReady() != 0)
    {
        return USART0_Read();
    }
    else
    {
        return '\0';
    }
}


/*********************************************************************
 * Function:        static uint8_t Read_Parameter(void)  
 *
 * PreCondition:    Expect data
 *
 * Input:           None
 *
 * Output:          Value received
 *
 * Side Effects:    Unknown yet
 *
 * Overview:        Looks for data, if it's received, return
 *                  Times out after 5 sec
 *
 ********************************************************************/

static uint8_t Read_Parameter(void)                                              // Looks for data
{
    uint8_t Param = 0;
    uint8_t Count = 50;
    
    while(Count != 0)
    {
            Count -= 1;
            Param = Unblocking_Read();
            if((Param != '\n') && (Param != '\r') && (Param != '\0'))
            {   
                Count = 0;                                                  // exit if good data
            }
            else
            {            
                _delay_ms(RX_DELAY);                                        // wait and try again if needed
            }
     }

    return Param;
}


/*********************************************************************
 * Function:        static void Set_Bias_Requested(void)  
 *
 * PreCondition:    Expect data
 *
 * Input:           None
 *
 * Output:          Value received
 *
 * Side Effects:    Unknown yet
 *
 * Overview:        Receives DAC value requested and writes to Bias DAC
 *                  Creates error message if doesn't work
 *
 ********************************************************************/

static void Set_Bias_Requested(void)
{
        
    int sum, digit, j, i;
    char Rx[4];
    
    for(j=0; j<4; j++)							// Receive number in ASCII
	{
	Rx[j] = Read_Parameter();
	}
 
    if(current_program == ACTIVE)
    {    
        sum = 0;                                    // a2i function
        for (i = 0; i < 4; i++) 
        {
            digit = Rx[i] - 0x30;
            sum = (sum * 10) + digit;
        }

        DAC0_setVal(sum);

        printf("\r\nBias DAC Set\r\n");
    }
    else if(current_program == STANDBY)
    {
        printf("\r\nPlease Enable Board first: 'E' \n\r");
    }
}



/*********************************************************************
 * Function:        static void Send_Bias_Read(void) 
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    Unknown yet
 *
 * Overview:        Reads ADC and sends value out UART
 *
 ********************************************************************/

static void Send_Bias_Read(void)
{
    
    int a = 0;
    char s[20];

    adcVal = ADC0_read();
    
    a = adcVal;
    
    // Could actually do math and convert to a voltage....?
    
    printf("\n\r");
    sprintf(s, "Bias ADC = %d", a);                                            // Convert to ASCII
    printf(s);                                                                  // Print to port
    printf("\r\n");
          
}


/*********************************************************************
 * Function:        static void BoardSetStatus(uint8_t); 
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    Unknown yet
 *
 * Overview:        Enables or Disables Hardware (for low power mode)
 *                  - Function works, but haven't tested delays
 ********************************************************************/

static void BoardSetStatus(uint8_t Status)
{
    DAC0_setVal(1023);                                                          // Make sure set low to start
    switch(Status)
    {
        case 'E':
            PORTC.OUTSET = PIN2_bm;                                             // 5V_SW_ENABLE = PC2, set high
             _delay_ms(PWR_DELAY);
            PORTD.OUTSET = PIN7_bm;                                             // 3V3_SW_ENABLE = PD7, set high
            _delay_ms(PWR_DELAY);
            PORTD.OUTSET = PIN2_bm;                                             // BIAS_ENABLE = PD2, set high
            current_program = ACTIVE;
            printf("\r\nBoard Active\r\n");
            break;
        case 'D':
            PORTF.OUTCLR = PIN0_bm;                                           // LED 450nm
            PORTF.OUTCLR = PIN1_bm;                                           // LED 410nm
            PORTF.OUTCLR = PIN2_bm;                                             // LED 365nm
            PORTF.OUTCLR = PIN3_bm;                                             // LED 295nm
            PORTF.OUTCLR = PIN4_bm;                                             // LED 278nm
            PORTF.OUTCLR = PIN5_bm;                                             // LED 255nm
            PORTC.OUTCLR = PIN0_bm;                                             // LED 235nm
            PORTC.OUTCLR = PIN1_bm;                                             // DAQ Sync
            PORTD.OUTCLR = PIN3_bm;                                             // CLK_SEL = PD3, set low for external clock
            TCA0.SPLIT.CTRLB = 0x0;                                             // TRIG1 = PC3, turn tca off
             
            PORTD.OUTCLR = PIN2_bm;                                             // BIAS_ENABLE = PD2, set low
            _delay_ms(PWR_DELAY);
            PORTD.OUTCLR = PIN7_bm;                                             // 3V3_SW_ENABLE = PD7, set low
            _delay_ms(PWR_DELAY);
            PORTC.OUTCLR = PIN2_bm;                                             // 5V_SW_ENABLE = PC2, set low
            current_program = STANDBY;
            printf("\r\nBoard Standby\r\n");
            break;
    } 
}


/*********************************************************************
 * Function:        static void SetTrigger(void); 
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    Unknown yet
 *
 * Overview:        Reads and sets Trigger
 *                  Creates error message if doesn't work
 ********************************************************************/

static void SetTrigger(void)
{
    
    static uint8_t Trigger = 0;

    Trigger = Read_Parameter();
    
    if(current_program == ACTIVE)
    {
        switch(Trigger)
        {
            case 'I':
                PORTD.OUTSET = PIN3_bm;                                             // CLK_SEL = PD3, set high for internal clock
                printf("\r\nTrigger Source: Set Internal\r\n");
                break;
            case 'E':
                PORTD.OUTCLR = PIN3_bm;                                             // CLK_SEL = PD3, set low for external clock
                printf("\r\nTrigger Source: Set External\r\n");
                break;
            default:
                printf("\n\rInvalid Command!\n\r");
                Print_Menu();
                break;
        } 
    }
    else if(current_program == STANDBY)
    {
        printf("\r\nPlease Enable Board first: 'E' \n\r");
    }
}



/*********************************************************************
 * Function:        static void SetRate(void)); 
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    Unknown yet
 *
 * Overview:        Sets internal trigger source rate
 *                  
 ********************************************************************/


static void SetRate(void)                                                      // Sets internal trigger source rate
{
    static uint8_t Rate = 0;

    Rate = Read_Parameter();
    
    if(current_program == ACTIVE)
    {
        switch(Rate)
        {
            case 'S':
                TCA0_init('S');
                printf("\r\nTrigger Rate: Set 1.5kHz\r\n");
                break;
            case 'F':
                TCA0_init('F');
                printf("\r\nTrigger Rate: Set 8MHz\r\n");
                break;
            default:
                printf("\n\rInvalid Command!\n\r");
                Print_Menu();
                break;
        }
    }
    else if(current_program == STANDBY)
    {
        printf("\r\nPlease Enable Board first: 'E' \n\r");
    } 
}


/*********************************************************************
 * Function:        static void SetLED(void); 
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    Unknown yet
 *
 * Overview:        Set LED (xor), if any set, then set sync out
 *                  If none set, turn off sync out
 *                  
 ********************************************************************/


static void SetLED(void)                                                       // Set LED (xor), if any set, then set sync out
{

    static uint8_t LED = 0;

    LED = Read_Parameter();
    
    if(('9' >= LED) && (LED >= '0'))                                            // If valid command, start by disabling everything
    {
        PORTF.OUTCLR = PIN0_bm;                                                 // LED 450nm
        PORTF.OUTCLR = PIN1_bm;                                                 // LED 410nm
        PORTF.OUTCLR = PIN2_bm;                                                 // LED 365nm
        PORTF.OUTCLR = PIN3_bm;                                                 // LED 295nm
        PORTF.OUTCLR = PIN4_bm;                                                 // LED 278nm
        PORTF.OUTCLR = PIN5_bm;                                                 // LED 255nm
        PORTC.OUTCLR = PIN0_bm;                                                 // LED 235nm
        PORTC.OUTCLR = PIN1_bm;                                                 // DAQ Sync
    }
    
    if(current_program == ACTIVE)
    {
        switch(LED)
        {
            case '0':                                                               
                printf("\r\nLEDs off\r\n");                                         // Leave all off
                break;
            case '1':
                PORTF.OUTSET = PIN0_bm;                                           // LED 450nm
                PORTC.OUTSET = PIN1_bm;                                             // DAQ Sync        
                printf("\r\n450nm LED on\r\n");
                break;
            case '2':
                PORTF.OUTSET = PIN1_bm;                                           // LED 410nm
                PORTC.OUTSET = PIN1_bm;                                             // DAQ Sync         
                printf("\r\n410nm LED on\r\n");
                break;    
            case '3':
                PORTF.OUTSET = PIN2_bm;                                             // LED 365nm
                PORTC.OUTSET = PIN1_bm;                                             // DAQ Sync      
                printf("\r\n365nm LED on\r\n");
                break;
            case '4':
                PORTF.OUTSET = PIN3_bm;                                             // LED 295nm
                PORTC.OUTSET = PIN1_bm;                                             // DAQ Sync       
                printf("\r\n295nm LED on\r\n");
                break;
            case '5':
                PORTF.OUTSET = PIN4_bm;                                             // LED 278nm
                PORTC.OUTSET = PIN1_bm;                                             // DAQ Sync    
                printf("\r\n278nm LED on\r\n");
                break;
            case '6':
                PORTF.OUTSET = PIN5_bm;                                             // LED 255nm
                PORTC.OUTSET = PIN1_bm;                                             // DAQ Sync
                printf("\r\n255nm LED on\r\n");
                break;
            case '7':
                PORTC.OUTSET = PIN0_bm;                                             // LED 235nm
                PORTC.OUTSET = PIN1_bm;                                             // DAQ Sync
                printf("\r\n235nm LED on\r\n");
                break;      
            default:
                printf("\n\rInvalid Command!\n\r");                                 // If invalid, do nothing
                Print_Menu();
                break;
        }
    }
    else if(current_program == STANDBY)
    {
        printf("\r\nPlease Enable Board first: 'E' \n\r");
    } 
    
}


/*********************************************************************
 * Function:        static void VREF_init(void); 
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    Unknown yet
 *
 * Overview:        Configures VREF for DAC0 and ADC0 to VDD (3.3V)
 *                  
 ********************************************************************/

static void VREF_init(void)
{
    VREF.DAC0REF = VREF_REFSEL_VDD_gc /* Select 3.3V VDD Voltage Reference for DAC */
                 | VREF_ALWAYSON_bm;    /* Set the Voltage Reference in Always On mode */   
    VREF.ADC0REF = VREF_REFSEL_VDD_gc /* Select 3.3V VDD Voltage Reference for ADC */
                 | VREF_ALWAYSON_bm;    /* Set the Voltage Reference in Always On mode */
    /* Wait VREF start-up time */
    _delay_us(VREF_STARTUP_TIME);
}


/*********************************************************************
 * Function:        static void DAC0_init(void); 
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    Unknown yet
 *
 * Overview:        Enables DAC0 (the only DAC) PD6
 *                  
 ********************************************************************/

static void DAC0_init(void)
{
    /* Disable digital input buffer */
    PORTD.PIN6CTRL &= ~PORT_ISC_gm;
    PORTD.PIN6CTRL |= PORT_ISC_INPUT_DISABLE_gc;
    /* Disable pull-up resistor */
    PORTD.PIN6CTRL &= ~PORT_PULLUPEN_bm;   
    DAC0.CTRLA = DAC_ENABLE_bm          /* Enable DAC */
               | DAC_OUTEN_bm           /* Enable output buffer */
               | DAC_RUNSTDBY_bm;       /* Enable Run in Standby mode */
}


/*********************************************************************
 * Function:        static void DAC0_setVal(uint16_t val); 
 *
 * PreCondition:    None
 *
 * Input:           DAC value to be set
 *
 * Output:          None
 *
 * Side Effects:    Unknown yet
 *
 * Overview:        Sets DAC Value
 *                  
 ********************************************************************/


static void DAC0_setVal(uint16_t value)
{
    /* Store the two LSbs in DAC0.DATAL */
    DAC0.DATAL = (value & LSB_MASK) << 6;
    /* Store the eight MSbs in DAC0.DATAH */
    DAC0.DATAH = value >> 2;
}


/*********************************************************************
 * Function:        static void ADC0_init(void); 
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    Unknown yet
 *
 * Overview:        Enables ADC
 *                  
 ********************************************************************/


static void ADC0_init(void)
{
    
    ADC0.CTRLC = ADC_PRESC_DIV2_gc;     // CLK_PER divided by 2
    ADC0.CTRLA = ADC_ENABLE_bm          // Enable ADC
               | ADC_RESSEL_12BIT_gc;   // Use 12-bit resolution
    
    ADC0.MUXPOS = ADC_MUXNEG_AIN1_gc;   // Select ADC channel as PD1: AIN1
}


/*********************************************************************
 * Function:        static uint16_t ADC0_read(void); 
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    Unknown yet
 *
 * Overview:        Reads ADC
 *                  
 ********************************************************************/


static uint16_t ADC0_read(void)
{
    /* Start conversion */
    ADC0.COMMAND = ADC_STCONV_bm;
    /* Wait until ADC conversion is done */
    while(!(ADC0.INTFLAGS & ADC_RESRDY_bm))
    {
        ;
    }
    /* The interrupt flag is cleared when the conversion result is accessed */
    return ADC0.RES;
}


/*********************************************************************
 * Function:        static void TCA0_init(char speed)
 *
 * PreCondition:    None
 *
 * Input:           speed - (S or F)
 *
 * Output:          None
 *
 * Side Effects:    Unknown yet
 *
 * Overview:        Initializes TCA0
 *                  
 ********************************************************************/


static void TCA0_init(char speed)
{
     
    /* set waveform output on PORT C */
    PORTMUX.TCAROUTEA = PORTMUX_TCA0_PORTC_gc;

    TCA0.SPLIT.CTRLD = TCA_SPLIT_SPLITM_bm;                 

    TCA0.SPLIT.CTRLB = TCA_SPLIT_HCMP0EN_bm;     /* enable compare channel 0 for the higher byte */

    if (speed == 'S')
    {
        /* set PWM frequency 1.5kHz and duty cycle (50%) */
        TCA0.SPLIT.HPER = 250;       
        TCA0.SPLIT.HCMP0 = 125;

        TCA0.SPLIT.CTRLA = TCA_SPLIT_CLKSEL_DIV64_gc    /* set clock source (sys_clk/4) */
                        | TCA_SPLIT_ENABLE_bm;         /* start timer */
    }
    else if (speed == 'F')
    {
        /* set PWM frequency 8MHz and duty cycle (50%) */
        TCA0.SPLIT.HPER = 2;       
        TCA0.SPLIT.HCMP0 = 1;   

        TCA0.SPLIT.CTRLA = TCA_SPLIT_CLKSEL_DIV1_gc    /* set clock source (sys_clk/1) */
                        | TCA_SPLIT_ENABLE_bm;         /* start timer */  
    }
}

/**
    End of File
*/
