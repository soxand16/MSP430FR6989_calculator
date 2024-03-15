/* calc_LCD
 *
 * LCD functions for a calculator
 *
 * Only digits and '+', '-', '*' and '/' supported
 *
 ******************************************************************************/

#ifndef OUTOFBOX_MSP430FR6989_HAL_LCD_H_
#define OUTOFBOX_MSP430FR6989_HAL_LCD_H_



// Define word access definitions to LCD memories
#ifndef LCDMEMW
#define LCDMEMW    ((int*) LCDMEM) /* LCD Memory (for C) */
#endif

extern const char digit[10][2];
extern const char op_char[4][2];
extern const int pos[6];
extern const char alphabet[26][2];

void Init_LCD(void);
void showChar(char, int);
void clearLCD(void);


#endif /* OUTOFBOX_MSP430FR6989_HAL_LCD_H_ */
