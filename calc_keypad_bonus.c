#include <calc_LCD.h>
#include <driverlib.h>

unsigned char read_keypad(void);

void displayCurrent(void);
void runCalculator(void);
int num_digits(long n);

void Init_GPIO(void);

// States for the calculator
enum c_states {
    FIRST_OP,
    OP,
    SECOND_OP,
    RESULT
} c_state = FIRST_OP;

// Variables to hold the operands and results
long first_op = 0;
long second_op = 0;
long result;
// Flag for debouncing keypad
bool pressed = false;

// Enum to hold the operation
enum operations {
    ADD,
    SUB,
    MULT,
    DIV
} operation;

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


int main(void)
{
    WDTCTL = WDTPW | WDTHOLD; // Stop WDT
    PM5CTL0 &= ~LOCKLPM5; // Enable GPIO pins

    // Initialize the input and output pins (GPIO)
    Init_GPIO();
    Init_LCD();

    Timer_A_initUpMode(TIMER_A0_BASE, &initUpParam_A0);

    __enable_interrupt();

    // Display new line at startup
    while (1) {}

}

// runs every 15ms
#pragma vector = TIMER0_A0_VECTOR
__interrupt void TIMER0_A0_ISR (void)
{
    runCalculator();  // Performs calculator functions
    displayCurrent(); // displays the current operand or result
}


/*
 * Reads the keypad. The connection is as follows:
 *
 * ************************  MSP  *****************************
 * GND  |  P9.0  |  P9.1  |  P8.4  |  P8.5  |  P8.6  |  P8.7  |
 *
 *
 * Pin1 |   2    |   3    |   4    |   5    |   6    |   7    |
 * ***********************  KEYPAD  ***************************
 * Col0 | Col 1  | Col 2  | Row 0  | Row 1  | Row 2  | Row 3  |
 */
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


// Returns the number of digits within "n"
int num_digits(long n)
{
    int count = 0;
    // Divide by 10 until 0 and count
    while(n != 0)
    {
        n /= 10;
        count++;
    }
    return count;
}

void runCalculator(void)
{
    // Get keypad input
    unsigned char input = read_keypad();

    switch(c_state)
    {
    case FIRST_OP:

        if(input >= '0' && input <= '9') // If the input is a number
        {
            if(num_digits(first_op) < 6) // If the current number is below 6 digits
            {
                // Add input to previously stored value
                first_op = first_op * 10 + (input - '0');
            }
        }
        if(input == '#')
        {
            first_op = 0;
            clearLCD();
        }
        if(input == '*')
        {
            operation = ADD;
            c_state = OP;    // move to next state
            clearLCD();
        }

        break;
    case OP:
        if(input == '*')
        {
            operation = (++operation) % 4;
        }
        if(input >= '0' && input <= '9')
        {
            second_op = (input - '0');
            c_state = SECOND_OP;
            clearLCD();
        }
        break;
    case SECOND_OP:
        if(input >= '0' && input <= '9') // If the input is a number
        {
            if(num_digits(second_op) < 6) // If the current number is below 6 digits
            {
                // Add input to previously stored value
                second_op = second_op * 10 + (input - '0');
            }
        }
        if(input == '#')
        {
            second_op = 0;
            clearLCD();
        }
        if(input == '*')
        {
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
                result = first_op / second_op;
                break;
            default:
                result = 0;
                break;
            }
            c_state = RESULT;    // move to next state
            clearLCD();
        }
        break;
    case RESULT:
        if(input >= '0' && input <= '9') // If the input is a number
        {
            first_op = (input - '0');
            c_state = FIRST_OP;
            clearLCD();
        }
        if(input == '#')
        {
            first_op = 0;
            c_state = FIRST_OP;
            clearLCD();
        }
        if(input == '*')
        {
            first_op = result;
            c_state = OP;
            clearLCD();
        }
        break;
    default:
        break;
    }
}

void displayCurrent(void)
{
    long to_display = 0; // number to display
    bool display_op = false;
    switch(c_state)
    {
    case FIRST_OP:
        to_display = first_op;
        break;
    case OP:
        display_op = true;
        break;
    case SECOND_OP:
        to_display = second_op;
        break;
    case RESULT:
        to_display = result;
        break;
    default: break;
    }

    if(display_op) // if displaying operator
    {
        switch(operation)
        {
        case ADD:
            showChar('+',5);
            break;
        case SUB:
            showChar('-',5);
            break;
        case MULT:
            showChar('*',5);
            break;
        case DIV:
            showChar('/',5);
            break;
        default: break;
        }
    }
    else // if displaying a number
    {
        if(to_display < 0)
        {
           to_display *= -1;    // convert to positive
           LCDMEM[10] |= 0x04;  // display NEG
        }

        if(to_display == 0) // show '0'
        {
            showChar('0', 5);
        }
        else
        {
            if(num_digits(to_display) <= 6)
            {
                int index = 5; // right most digit
                while(to_display != 0 && index >= 0) // for all digits
                {
                    showChar((char)(to_display%10 + '0'), index--); // strip rightmost digit
                    to_display /= 10; // shift digits right
                }
            }
            else // too long for display
            {
                showChar('O',1);
                showChar('V',2);
                showChar('E',3);
                showChar('R',4);
                showChar('F',5);
            }
        }
        if(c_state == RESULT)
        {
            LCDMEM[2] |= 0x02;
        }
    }

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

