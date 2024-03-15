/* calc_LCD
 *
 * LCD functions for a calculator
 *
 * Only digits and '+', '-', '*' and '/' supported
 *
 ******************************************************************************/

#include <calc_LCD.h>
#include <driverlib.h>
#include "string.h"


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

// *** ASSUMING ***
// a b c d  e f g h   j k l m  n p q _

// LCD memory map for operations
const char op_char[4][2] =
{
    {0x03, 0x50},  /* "+" LCD segments a+b+c+d+e+f+k+q */
    {0x03, 0x00},  /* "-" */
    {0x03, 0xFA},  /* "*" */
    {0x00, 0x28}   /* "/" */
};

// [0][1]:[2][3]:[4][5]
const int pos[6] = {9, 5, 3, 18, 14, 7};

// LCD memory map for uppercase letters
const char alphabet[26][2] =
{
    {0xEF, 0x00},  /* "A" LCD segments a+b+c+e+f+g+m */
    {0xF1, 0x50},  /* "B" */
    {0x9C, 0x00},  /* "C" */
    {0xF0, 0x50},  /* "D" */
    {0x9F, 0x00},  /* "E" */
    {0x8F, 0x00},  /* "F" */
    {0xBD, 0x00},  /* "G" */
    {0x6F, 0x00},  /* "H" */
    {0x90, 0x50},  /* "I" */
    {0x78, 0x00},  /* "J" */
    {0x0E, 0x22},  /* "K" */
    {0x1C, 0x00},  /* "L" */
    {0x6C, 0xA0},  /* "M" */
    {0x6C, 0x82},  /* "N" */
    {0xFC, 0x00},  /* "O" */
    {0xCF, 0x00},  /* "P" */
    {0xFC, 0x02},  /* "Q" */
    {0xCF, 0x02},  /* "R" */
    {0xB7, 0x00},  /* "S" */
    {0x80, 0x50},  /* "T" */
    {0x7C, 0x00},  /* "U" */
    {0x0C, 0x28},  /* "V" */
    {0x6C, 0x0A},  /* "W" */
    {0x00, 0xAA},  /* "X" */
    {0x00, 0xB0},  /* "Y" */
    {0x90, 0x28}   /* "Z" */
};

void Init_LCD()
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
}

/*
 * Displays input character at given LCD digit/position
 * Only spaces, numeric digits, and uppercase letters are accepted characters
 */
void showChar(char c, int p)
{
    if (c >= '0' && c <= '9')
    {
        // Display digit
        LCDMEM[pos[p]] = digit[c-'0'][0];
        LCDMEM[pos[p]+1] = digit[c-'0'][1];
    }
    else if (c == '+')
    {
        LCDMEM[pos[p]] = op_char[0][0];
        LCDMEM[pos[p]+1] = op_char[0][1];
    }
    else if (c == '-')
    {
        LCDMEM[pos[p]] = op_char[1][0];
        LCDMEM[pos[p]+1] = op_char[1][1];
    }
    else if (c == '*')
    {
        LCDMEM[pos[p]] = op_char[2][0];
        LCDMEM[pos[p]+1] = op_char[2][1];
    }
    else if (c == '/')
    {
        LCDMEM[pos[p]] = op_char[3][0];
        LCDMEM[pos[p]+1] = op_char[3][1];
    }
    else if (c >= 'A' && c <= 'Z')
    {
        // Display alphabet
        LCDMEM[pos[p]] = alphabet[c-'A'][0];
        LCDMEM[pos[p]+1] = alphabet[c-'A'][1];
    }
    else
    {
        // Turn all segments
        LCDMEM[pos[p]] = 0xFF;
        LCDMEM[pos[p]+1] = 0xFF;
    }
}

/*
 * Clears memories to all 6 digits on the LCD
 */
void clearLCD()
{
    LCDMEM[pos[0]] = LCDBMEM[pos[0]] = 0;
    LCDMEM[pos[0]+1] = LCDBMEM[pos[0]+1] = 0;
    LCDMEM[pos[1]] = LCDBMEM[pos[1]] = 0;
    LCDMEM[pos[1]+1] = LCDBMEM[pos[1]+1] = 0;
    LCDMEM[pos[2]] = LCDBMEM[pos[2]] = 0;
    LCDMEM[pos[2]+1] = LCDBMEM[pos[2]+1] = 0;
    LCDMEM[pos[3]] = LCDBMEM[pos[3]] = 0;
    LCDMEM[pos[3]+1] = LCDBMEM[pos[3]+1] = 0;
    LCDMEM[pos[4]] = LCDBMEM[pos[4]] = 0;
    LCDMEM[pos[4]+1] = LCDBMEM[pos[4]+1] = 0;
    LCDMEM[pos[5]] = LCDBMEM[pos[5]] = 0;
    LCDMEM[pos[5]+1] = LCDBMEM[pos[5]+1] = 0;

    LCDM14 = LCDBM14 = 0x00;
    LCDM18 = LCDBM18 = 0x00;
    LCDM3 = LCDBM3 = 0x00;
}
