#include <msp430fr6989.h>
#include <driverlib.h>
#include <math.h>
// TODO: ADD LCD FUNCTIONS!
//#include <hal_LCD.h>
#define pos1 9   /* Digit A1 begins at S18 */
#define pos2 5   /* Digit A2 begins at S10 */
#define pos3 3   /* Digit A3 begins at S6  */
#define pos4 18  /* Digit A4 begins at S36 */
#define pos5 14  /* Digit A5 begins at S28 */
#define pos6 7   /* Digit A6 begins at S14 */
// LCD memory map for numeric digits
const char digit[10][2] =
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
// Set by the ISR, cleared in function routines
volatile unsigned char receive_interrupt_flag = 0;
volatile unsigned char ready_for_new_data_flag = 0;
// Configure UART for 115200 baud, 8 MHz SMCLK source
// Use this link to calculate baud rate parameters
// http://software-dl.ti.com/msp430/msp430_public_sw/mcu/msp430/MSP430BaudRateConverter/index.html
EUSCI_A_UART_initParam eusci_a1_uart_param =
{
     EUSCI_A_UART_CLOCKSOURCE_SMCLK,
     0x04,
     0x05,
     0x55,
     EUSCI_A_UART_NO_PARITY,
     EUSCI_A_UART_LSB_FIRST,
     EUSCI_A_UART_ONE_STOP_BIT,
     EUSCI_A_UART_MODE,
     EUSCI_A_UART_OVERSAMPLING_BAUDRATE_GENERATION
};

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Setup circular buffer type

typedef struct {
    unsigned char * const buffer;
    int head;
    int tail;
    const int maxlen;
} circ_bbuf_t;

circ_bbuf_t c_buffer;

// Circular buffer definition macro
#define CIRC_BBUF_DEF(x,y)                \
    unsigned char x##_data_space[y];            \
    circ_bbuf_t x = {                     \
        .buffer = x##_data_space,         \
        .head = 0,                        \
        .tail = 0,                        \
        .maxlen = y                       \
    }

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// create the buffer with length of 32 bytes using the circular buffer macro https://embedjournal.com/implementing-circular-buffer-embedded-c/
CIRC_BBUF_DEF(c_buffer, 32);

// TODO: Variables for calculator
int firstInput = 0;
char asmd = 0;
int secondInput = 0;
int Solution = 0;
int Round = 1; //rounding precision
int ansChar = 0;
int ansChar2 = 0;
int digit = 1;
int answer = 0;

char buffer = 0;//Used to pass the value in the buffer to that of my input variables
// TODO: State machine enum
enum calculations{input, hold, addition, subtraction, multiplication, division}operation = input;
enum numbers{numberOne, asmord, numberTwo}data = numberOne;

void init_LCD(void);
void calculations(void);

//setup function prototypes
void Init_GPIO(void);
void Init_Clock(void);
void Init_UART(void);

void receive_data(void);
void send_char(unsigned char);
void read_buffer(void);

int circ_bbuf_push(circ_bbuf_t *, unsigned char);
int circ_bbuf_pop(circ_bbuf_t *, unsigned char *);

void show_digit(int, int);
void displayToLCD(void);

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------

int main(void){

    init_LCD();
    // Stop watchdog timer
    WDT_A_hold(__MSP430_BASEADDRESS_WDT_A__);
    // Disable the GPIO power-on default high-impedance mode
    // to activate previously configured port settings
    PMM_unlockLPM5();
    // Module configurations
    Init_GPIO();
    Init_Clock();
    Init_UART();
    // Clear any spurious interrupts
    GPIO_clearInterrupt(GPIO_PORT_P1, GPIO_PIN1);
    GPIO_clearInterrupt(GPIO_PORT_P1, GPIO_PIN2);
    EUSCI_A_UART_clearInterrupt(EUSCI_A1_BASE, EUSCI_A_UART_RECEIVE_INTERRUPT_FLAG);
    EUSCI_A_UART_clearInterrupt(EUSCI_A1_BASE, EUSCI_A_UART_TRANSMIT_INTERRUPT);
    // Enable interrupts intrinsic
    __enable_interrupt();
    // Sync up UART
    EUSCI_A_UART_transmitBreak(EUSCI_A1_BASE);

    while (1)
    {
        read_buffer();
        receive_data();
        displayToLCD();
    }
}
// TODO: Function for calculator state machine
//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void calculations(){
    switch (operation){
    case input:

        switch (data){
        case numberOne:
            if(buffer >= '0' && buffer <='9'){
                firstInput = firstInput * 10 + (buffer - '0');
            }
            else{
                data = asmord;
            }
            break;

        case asmord:
            if(buffer == '+' || buffer == '-' || buffer == '*' || buffer == '/'){
                asmd = buffer;
                data = numberTwo;
            }
            else{
                data = asmord;
            }
            break;

        case numberTwo:
            if(buffer >= '0' && buffer <='9'){
                secondInput = secondInput * 10 + (buffer - '0');
            }
            else{
                break;//If the buffer doesn't have a number in it the state machine will exit
            }
        }
        operation = hold;//Advances to the operator selection state

        break;
    case hold:
        if(asmd == '+'){
            operation = addition;
        }
        else{
            if(asmd == '-'){
                operation = subtraction;
            }
            else{
                if(asmd == '*'){
                    operation = multiplication;
                }
                else{
                    if(asmd == '/'){
                        operation = division;
                    }
                }
            }
        }

        break;

    case addition:
        Solution = roundf((firstInput + secondInput) * Round) / Round;
        operation = input;

        //Wipe the variables that way input state machine works properly
        firstInput = 0;
        asmd = 0;
        secondInput = 0;
        Solution = 0;

        break;

    case subtraction:
        Solution = roundf((firstInput - secondInput) * Round) / Round;
        operation = input;

        //Wipe the variables that way input state machine works properly
        firstInput = 0;
        asmd = 0;
        secondInput = 0;
        Solution = 0;

        break;

    case multiplication:
        Solution = roundf((firstInput * secondInput) * Round) / Round;
        operation = input;

        if(Solution < 0){
            ansChar = abs(Solution);
            ansChar2 = abs(Solution);
            digit = 1;
            send_char('-');
            while(ansChar2 > 9){
                ansChar2 = ansChar2 / pow(10, digit);
                digit ++;
            }
            while(ansChar >=0){
                if(ansChar > 9){
                    answer = ansChar / pow(10, digit);
                    digit --;
                    send_char(answer + 30);
                    ansChar = ansChar % pow(10, digit);
                }
                else{
                    answer = ansChar % pow(10, digit);
                    send_char(answer + 30);
                }
            }
        }
        else{
            ansChar = Solution;
            ansChar2 = Solution;
            digit = 1;
            while(ansChar2 > 9){
                ansChar2 = ansChar2 / pow(10, digit);
                digit ++;
            }
            while(ansChar >= 0){
                if(ansChar > 9){
                    answer = ansChar / pow(10, digit);
                    digit --;
                    send_char(answer + 30);
                    ansChar = ansChar % pow(10, digit);
                }
                else{
                    answer = ansChar % pow(10, digit);
                    send_char(answer + 30);
                }
            }
        }

        //Wipe variables
        firstInput = 0;
        asmd = 0;
        secondInput = 0;
        Solution = 0;

        break;

    case division:
        if(secondInput == 0){
            //Division by 0 not allowed
        }
        else{
            Solution = firstInput / secondInput;
        }
        Solution = roundf((firstInput / secondInput) * Round) / Round;
        operation = input;
        //Wipe the variables that way input state machine works properly
        firstInput = 0;
        asmd = 0;
        secondInput = 0;
        Solution = 0;
        break;
    }
}

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// TODO: Function to handle LCD display

void displayToLCD(){
    show_digit(firstInput % 10, pos2);   //this splting up the seconds minutes and hours into 2 diffrent numbers using modulus to be incremented on the screen
    show_digit(firstInput / 10, pos1);
    show_digit(secondInput % 10, pos4);
    show_digit(secondInput / 10, pos3);
    show_digit(Solution % 10, pos6);
    show_digit(Solution / 10, pos5);

    if(Solution <= 0){
         //Display negative on LCD
    }

    LCDM7 |= 0x04;//these are to turn on the colons
    LCDBM7 |= 0x04;
    LCDM20 |= 0x04;
    LCDBM20 |= 0x04;
}

//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void show_digit(int d, int position){

    LCDMEM[position] = digit[d][0];     //this displays the number that is called display time
    LCDMEM[position + 1] = digit[d][1];

}

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Echo what the user types back to the serial port so they can see it

void read_buffer(){

    int status;
    unsigned char buffer_data;

    status = circ_bbuf_pop(&c_buffer, &buffer_data);

    if(status == 0) // read something from the buffer
    {
        // TODO: buffer_data now contains the character!
        // TODO: Handle it here, or pass it to another function (for cleanliness)
        buffer = buffer_data;

        // Echo to the terminal
        send_char(buffer_data);

        if(buffer_data == '\r')
        {
            send_char('\n');
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Receives data given an interrupt flag, stores it in the circular buffer, refer to driverlib doc for details

void receive_data(){

    if(receive_interrupt_flag == 1)
    {
        receive_interrupt_flag = 0;
        unsigned char ch;
        ch = EUSCI_A_UART_receiveData(EUSCI_A1_BASE);

        circ_bbuf_push(&c_buffer, ch);
    }
}

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Sends a char to the serial port

void send_char(unsigned char input_char){

    // Wait until we can send again. Not the best way to do things...
    while(ready_for_new_data_flag == 0);

    int i;
    for(i = 0; i < 1000; i++);//wait a while

    // Reset flag and send data
    ready_for_new_data_flag == 0;
    EUSCI_A_UART_transmitData(EUSCI_A1_BASE, input_char);

}

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Pushes data onto the circular buffer

int circ_bbuf_push(circ_bbuf_t *c, unsigned char data){
    int next;

    next = c->head + 1;  // next is where head will point to after this write.
    if (next >= c->maxlen)
        next = 0;

    if (next == c->tail)  // if the head + 1 == tail, circular buffer is full
        return -1;

    c->buffer[c->head] = data;  // Load data and then move
    c->head = next;             // head to next data offset.
    return 0;  // return success to indicate successful push.
}

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Pulls data off of the circular buffer

int circ_bbuf_pop(circ_bbuf_t *c, unsigned char *data){
    int next;

    if (c->head == c->tail)  // if the head == tail, we don't have any data
        return -1;

    next = c->tail + 1;  // next is where tail will point to after this read.
    if(next >= c->maxlen)
        next = 0;

    *data = c->buffer[c->tail];  // Read data and then move
    c->tail = next;              // tail to next offset.
    return 0;  // return success to indicate successful pull.
}

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Initializes the EUSCI module

void Init_UART(){

    if (STATUS_FAIL == EUSCI_A_UART_init(EUSCI_A1_BASE, &eusci_a1_uart_param)) {
        return; // should probably handle this error
    }

    EUSCI_A_UART_enable(EUSCI_A1_BASE);

    // Enable USCI_A1 RX interrupts
    EUSCI_A_UART_enableInterrupt(EUSCI_A1_BASE, EUSCI_A_UART_RECEIVE_INTERRUPT);
    EUSCI_A_UART_enableInterrupt(EUSCI_A1_BASE, EUSCI_A_UART_TRANSMIT_INTERRUPT);
}

/*
 * GPIO Initialization
 */
void Init_GPIO()
{
    // Set all GPIO pins to output low to prevent floating input and reduce power consumption
    GPIO_setOutputLowOnPin(GPIO_PORT_P1, 0xFF);
    GPIO_setOutputLowOnPin(GPIO_PORT_P2, 0xFF);
    GPIO_setOutputLowOnPin(GPIO_PORT_P3, 0xFF);
    GPIO_setOutputLowOnPin(GPIO_PORT_P4, 0xFF);
    GPIO_setOutputLowOnPin(GPIO_PORT_P5, 0xFF);
    GPIO_setOutputLowOnPin(GPIO_PORT_P6, 0xFF);
    GPIO_setOutputLowOnPin(GPIO_PORT_P7, 0xFF);
    GPIO_setOutputLowOnPin(GPIO_PORT_P8, 0xFF);
    GPIO_setOutputLowOnPin(GPIO_PORT_P9, 0xFF);

    // Set P4.1 and P4.2 as Primary Module Function Input, LFXT.
    GPIO_setAsPeripheralModuleFunctionInputPin(
           GPIO_PORT_PJ,
           GPIO_PIN4 + GPIO_PIN5,
           GPIO_PRIMARY_MODULE_FUNCTION
           );

    // Set P3.4 as Primary Module Function Input, EUSCI_A1_TXD.
    GPIO_setAsPeripheralModuleFunctionOutputPin(
           GPIO_PORT_P3,
           GPIO_PIN4,
           GPIO_PRIMARY_MODULE_FUNCTION
           );

    // Set P3.5 as Primary Module Function Input, EUSCI_A1_RXD.
    GPIO_setAsPeripheralModuleFunctionInputPin(
           GPIO_PORT_P3,
           GPIO_PIN5,
           GPIO_PRIMARY_MODULE_FUNCTION
           );

    // Disable the GPIO power-on default high-impedance mode
    // to activate previously configured port settings
    PMM_unlockLPM5();
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

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
/*
 * Clock System Initialization
 */

void Init_Clock(){
    // Set DCO frequency to default 8MHz
    CS_setDCOFreq(CS_DCORSEL_0, CS_DCOFSEL_6);

    // Configure MCLK and SMCLK to default 8MHz
    CS_initClockSignal(CS_MCLK, CS_DCOCLK_SELECT, CS_CLOCK_DIVIDER_1);
    CS_initClockSignal(CS_SMCLK, CS_DCOCLK_SELECT, CS_CLOCK_DIVIDER_1);

    // Initializes the XT1 crystal oscillator
    CS_turnOnLFXT(CS_LFXT_DRIVE_3);
}

// see the MSP430 FR6x8x family user's guide for details on this interrupt vector
#pragma vector = USCI_A1_VECTOR
__interrupt void USCI_A1_ISR(void) {
    switch(__even_in_range(UCA1IV, 18)) {
        case 0x00: // Vector 0: No interrupts
            break;
        case 0x02:  // Vector 2: UCRXIFG
            receive_interrupt_flag = 1;

            break;
        case 0x04: // Vector 4: UCTXIFG
            ready_for_new_data_flag = 1;

            break;
        case 0x06: // Vector 6: UCSTTIFG

            break;
        case 0x08: // Vector 8: UCTXCPTIFG

            break;
        default:
            break;
    }

}
