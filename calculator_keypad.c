#include <msp430fr6989.h>
#include <driverlib.h>

//Change based on LCD Memory locations
#define pos1 9   /* Digit A1 begins at S18 */
#define pos2 5   /* Digit A2 begins at S10 */
#define pos3 3   /* Digit A3 begins at S6  */
#define pos4 18  /* Digit A4 begins at S36 */
#define pos5 14  /* Digit A5 begins at S28 */
#define pos6 7   /* Digit A6 begins at S14 */

void Init_GPIO(void);
void init_LCD(void);
void clear_LCD(void);

unsigned char read_keypad(void);

void runCalculator(void);
void show_digit(int d, int position);
void displayResult(void);

Timer_A_initUpModeParam initUpParam_A0 =
{
        TIMER_A_CLOCKSOURCE_SMCLK,              // SMCLK Clock Source
        TIMER_A_CLOCKSOURCE_DIVIDER_1,          // SMCLK/4 = 2MHz
        30000,                                  // 15ms debounce period
        TIMER_A_TAIE_INTERRUPT_DISABLE,         // Disable Timer interrupt
        TIMER_A_CCIE_CCR0_INTERRUPT_ENABLE ,    // Enable CCR0 interrupt
        TIMER_A_DO_CLEAR,                       // Clear value
        true                                    // Start Timer
};

// LCD memory map for numeric digits
const char Digit[10][2] =
{
    {0xFC, 0x28},  /* "0" LCD segments a+b+c+d+e+f+k+q */
    {0x60, 0x20},  /* "1" */
    {0xDB, 0x00},  /* "2" */
    {0xF3, 0x00},  /* "3" */
    {0x67, 0x00},  /* "4" */
    {0xB7, 0x00},  /* "5" */
    {0xBF, 0x00},  /* "6" */
    {0xE4, 0x00},  /* "7" */
    {0xFF, 0x00},  /* "8" */
    {0xF7, 0x00}   /* "9" */
};

// States for the calculator
enum c_states {
    FIRST_OP,
    OP,
    SECOND_OP,
    RESULT
} c_state = FIRST_OP;

// Variables to hold the operands and results
int first_op = 0;
int second_op = 0;
int result;

// Enum to hold the operation
enum operations {
    ADD,
    SUB,
    MULT,
    DIV
} operation;


int main(void)
{
    WDTCTL = WDTPW | WDTHOLD; // Stop WDT
    PM5CTL0 &= ~LOCKLPM5; // Enable GPIO pins

    init_LCD();
    Init_GPIO();

    Timer_A_initUpMode(TIMER_A0_BASE, &initUpParam_A0);

    __enable_interrupt();

    while (1)
    {}

}

// runs every 15ms
#pragma vector = TIMER0_A0_VECTOR
__interrupt void TIMER0_A0_ISR (void)
{
    runCalculator();  // Performs calculator functions
}

void runCalculator(void)
{
    // Get serial input
    unsigned char input = read_keypad();

    switch(c_state)
    {
    case FIRST_OP:

        if(input >= '0' && input <= '9') // If the input is a number
        {
            first_op = (input - '0');
            displayResult();
            c_state = OP; // go to next state
        }

        break;
    case OP:
        if(input == '1')
        {
            operation = ADD;
            c_state = SECOND_OP;
            displayResult();
        }
        if(input == '2')
        {
            operation = SUB;
            c_state = SECOND_OP;
            displayResult();
        }
        if(input == '3')
        {
            operation = MULT;
            c_state = SECOND_OP;
            displayResult();
        }
        if(input == '4')
        {
            operation = DIV;
            c_state = SECOND_OP;
            displayResult();
        }
        break;
    case SECOND_OP:
        if(input >= '0' && input <= '9') // If the input is a number
        {

            second_op = (input - '0');
            displayResult();
            c_state = RESULT;
        }
        break;
    case RESULT:
        // perform operation
        switch(operation)
        {
        case ADD:
            result = first_op + second_op;
            break;
        case SUB:
            result = first_op - second_op;
            break;
        case MULT:
            result = first_op * second_op;
            break;
        case DIV:
            result = 0;
            if(second_op != 0)
                result = first_op / second_op;
            break;
        default:
            result = 0;
            break;
        }

        displayResult();

        result = 0;
        first_op = 0;
        second_op = 0;
        // return to first state
        c_state = FIRST_OP;

        break;
    default:
        break;
    }
}

void show_digit(int d, int position)
{
    // Write digits to the LCD
    LCDMEM[position] = Digit[d][0];
    LCDMEM[position+1] = Digit[d][1];
}

void displayResult(void)
{
    clear_LCD();
    int temp_res = result;
    if(temp_res < 0)
    {
        temp_res *= -1;
        LCDMEM[10] |= 0x04;  // display NEG
    }

    show_digit((temp_res) % 10,pos6);
    if(result / 10 != 0)
        show_digit((temp_res) / 10,pos5);

    show_digit((second_op) % 10,pos4);
    if(second_op / 10 != 0)
        show_digit((second_op) / 10,pos3);

    show_digit((first_op) % 10  ,pos2);
    if(first_op / 10 != 0)
        show_digit((first_op) / 10  ,pos1);

    // Display the 2 colons
    LCDM7 |= 0x04;
    LCDM20 |= 0x04;
}

/*
 * Reads the keypad and returns the character pressed (char)
 * The connection is as follows:
 *
 * ************************  MSP  *****************************
 * GND  |  P9.0  |  P9.1  |  P8.4  |  P8.5  |  P8.6  |  P8.7  |
 *
 *
 * Pin1 |   2    |   3    |   4    |   5    |   6    |   7    |
 * ***********************  KEYPAD  ***************************
 * Col0 | Col 1  | Col 2  | Row 0  | Row 1  | Row 2  | Row 3  |
 *
 * NOTE: The correct pins (P8 4-7) must be set as input pullup!
 * Also, this function is designed to be run only every 10-20 milliseconds
 * for proper debouncing.
 */
bool pressed = false; // for debouncing
unsigned char read_keypad(void)
{
    // *** assumes P9.0 and P9.1 are already low ***
    unsigned char data = (P8IN >> 4) & 0x0F;
    int key = 0;

    // loop down through the rows until key held is found
    while((data & 0x01) != 0)
    {
        data >>= 1;
        key += 3;
    }

    if(key < 12) // key actually pressed
    {
        P9OUT |= 0x02; // turn on column 2
        if((P8IN & 0xF0) == 0xF0) // no short detected
        {
            key += 2; // column 2 must have been pressed
        }
        else
        {
            P9OUT |= 0x01; // turn on column 1 too
            if((P8IN & 0xF0) == 0xF0)
            {
                key += 1; // column 1 must have been pressed
            }
            // else it must have been column 0, and we have accounted for that

            P9OUT &= ~ 0x01; // pull back to low
        }
        P9OUT &= ~0x02; // pull back to low
        /* KEY
         * 0  1  2
         * 3  4  5
         * 6  7  8
         * 9  10 11
         */
        if(!pressed)
        {
            pressed = true; // debouncing flag

            if(key <= 8)
                return ('0' + key + 1); // these numbers are just offset by 1
            if(key == 10)
                return ('0'); // this is a 0
            if(key == 9)
                return ('*'); // this is a '*'
            if(key == 11)
                return ('#'); // this is a '#'
        }
    }
    else
    {
        // no key is pressed
        pressed = false;
    }
    return 0;
}

void clear_LCD()
{
    LCDMEM[pos1] = LCDBMEM[pos1] = 0;
    LCDMEM[pos1+1] = LCDBMEM[pos1+1] = 0;
    LCDMEM[pos2] = LCDBMEM[pos2] = 0;
    LCDMEM[pos2+1] = LCDBMEM[pos2+1] = 0;
    LCDMEM[pos3] = LCDBMEM[pos3] = 0;
    LCDMEM[pos3+1] = LCDBMEM[pos3+1] = 0;
    LCDMEM[pos4] = LCDBMEM[pos4] = 0;
    LCDMEM[pos4+1] = LCDBMEM[pos4+1] = 0;
    LCDMEM[pos5] = LCDBMEM[pos5] = 0;
    LCDMEM[pos5+1] = LCDBMEM[pos5+1] = 0;
    LCDMEM[pos6] = LCDBMEM[pos6] = 0;
    LCDMEM[pos6+1] = LCDBMEM[pos6+1] = 0;

    LCDM14 = LCDBM14 = 0x00;
    LCDM18 = LCDBM18 = 0x00;
    LCDM3 = LCDBM3 = 0x00;
}


void init_LCD()
{
    LCD_C_initParam initParams = {0};
    initParams.clockSource = LCD_C_CLOCKSOURCE_ACLK;
    initParams.clockDivider = LCD_C_CLOCKDIVIDER_1;
    initParams.clockPrescalar = LCD_C_CLOCKPRESCALAR_16;
    initParams.muxRate = LCD_C_4_MUX;
    initParams.waveforms = LCD_C_LOW_POWER_WAVEFORMS;
    initParams.segments = LCD_C_SEGMENTS_ENABLED;

    LCD_C_init(LCD_C_BASE, &initParams);
    // LCD Operation - VLCD generated internally, V2-V4 generated internally, v5 to ground

    /*  'FR6989 LaunchPad LCD1 uses Segments S4, S6-S21, S27-S31 and S35-S39 */
    LCD_C_setPinAsLCDFunctionEx(LCD_C_BASE, LCD_C_SEGMENT_LINE_4,
                                LCD_C_SEGMENT_LINE_4);
    LCD_C_setPinAsLCDFunctionEx(LCD_C_BASE, LCD_C_SEGMENT_LINE_6,
                                LCD_C_SEGMENT_LINE_21);
    LCD_C_setPinAsLCDFunctionEx(LCD_C_BASE, LCD_C_SEGMENT_LINE_27,
                                LCD_C_SEGMENT_LINE_31);
    LCD_C_setPinAsLCDFunctionEx(LCD_C_BASE, LCD_C_SEGMENT_LINE_35,
                                LCD_C_SEGMENT_LINE_39);

    LCD_C_setVLCDSource(LCD_C_BASE, LCD_C_VLCD_GENERATED_INTERNALLY,
                        LCD_C_V2V3V4_GENERATED_INTERNALLY_NOT_SWITCHED_TO_PINS,
                        LCD_C_V5_VSS);

    // Set VLCD voltage to 3.20v
    LCD_C_setVLCDVoltage(LCD_C_BASE,
                         LCD_C_CHARGEPUMP_VOLTAGE_3_02V_OR_2_52VREF);

    // Enable charge pump and select internal reference for it
    LCD_C_enableChargePump(LCD_C_BASE);
    LCD_C_selectChargePumpReference(LCD_C_BASE,
                                    LCD_C_INTERNAL_REFERENCE_VOLTAGE);

    LCD_C_configChargePump(LCD_C_BASE, LCD_C_SYNCHRONIZATION_ENABLED, 0);

    // Clear LCD memory
    LCD_C_clearMemory(LCD_C_BASE);

    //Turn LCD on
    LCD_C_on(LCD_C_BASE);

    LCD_C_selectDisplayMemory(LCD_C_BASE, LCD_C_DISPLAYSOURCE_MEMORY);
}

void Init_GPIO()
{
    // Set all GPIO pins to output low to prevent floating
    // input and reduce power consumption
    GPIO_setOutputLowOnPin(GPIO_PORT_P1, 0xFF);
    GPIO_setOutputLowOnPin(GPIO_PORT_P2, 0xFF);
    GPIO_setOutputLowOnPin(GPIO_PORT_P3, 0xFF);
    GPIO_setOutputLowOnPin(GPIO_PORT_P4, 0xFF);
    GPIO_setOutputLowOnPin(GPIO_PORT_P5, 0xFF);
    GPIO_setOutputLowOnPin(GPIO_PORT_P6, 0xFF);
    GPIO_setOutputLowOnPin(GPIO_PORT_P7, 0xFF);
    GPIO_setOutputLowOnPin(GPIO_PORT_P8, 0xFF);
    GPIO_setOutputLowOnPin(GPIO_PORT_P9, 0xFF);

    GPIO_setAsOutputPin(GPIO_PORT_P1, 0xFF);
    GPIO_setAsOutputPin(GPIO_PORT_P2, 0xFF);
    GPIO_setAsOutputPin(GPIO_PORT_P3, 0xFF);
    GPIO_setAsOutputPin(GPIO_PORT_P4, 0xFF);
    GPIO_setAsOutputPin(GPIO_PORT_P5, 0xFF);
    GPIO_setAsOutputPin(GPIO_PORT_P6, 0xFF);
    GPIO_setAsOutputPin(GPIO_PORT_P7, 0xFF);
    GPIO_setAsOutputPin(GPIO_PORT_P8, 0xFF);
    GPIO_setAsOutputPin(GPIO_PORT_P9, 0xFF);

    // Keypad inputs
    GPIO_setAsInputPinWithPullUpResistor(GPIO_PORT_P8, GPIO_PIN4 | GPIO_PIN5 | GPIO_PIN6 | GPIO_PIN7);


    // Disable the GPIO power-on default high-impedance mode
    // to activate previously configured port settings
    PMM_unlockLPM5();
}
