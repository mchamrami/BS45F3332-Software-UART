
 * BS45F3332 Software UART TX
 *
 * MCU        : Holtek BS45F3332
 * fSYS       : 1 MHz
 * UART       : 9600 baud, 8N1, TX only
 * TX pin     : PA2 / IPCK
 * Toolchain  : Holtek C / HT-IDE3000 style register definitions
 *
 * Design:
 * - GPIO bit-banged UART transmitter.
 * - Data-bit output levels are calculated before the timed frame.
 * - No if/else is executed inside the start/data/stop bit sequence.
 * - Interrupts are disabled only during one UART frame.
 * - A 5 ms gap is inserted after each byte.
 *
 * Important:
 * PA2 is shared with the IPCK programming function. Programming
 * activity may therefore appear as noise on a connected terminal.
 */

#include "BS45F3332.h"


#define SOFT_UART_PA2_MASK          0x04U
#define SOFT_UART_BYTE_GAP_MS       5U


/*
 * Tested timing for fSYS = 1 MHz.
 *
 * Recalibrate this delay if the system clock, compiler,
 * or optimization settings are changed.
 */
#define SOFT_UART_BIT_DELAY()              \
do                                         \
{                                          \
    _nop(); _nop(); _nop(); _nop();        \
    _nop(); _nop(); _nop(); _nop();        \
    _nop(); _nop(); _nop(); _nop();        \
    _nop(); _nop(); _nop(); _nop();        \
    _nop(); _nop(); _nop(); _nop();        \
    _nop(); _nop(); _nop(); _nop();        \
} while(0)


/* ------------------------------------------------------------
 * Blocking millisecond delay
 *
 * Used only for the deliberate inter-byte debug gap.
 * ------------------------------------------------------------ */
static void SoftUART_DelayMs(unsigned int ms)
{
    unsigned int i;
    unsigned int j;

    for(i = 0U; i < ms; i++)
    {
        for(j = 0U; j < 60U; j++)
        {
            _nop();
        }

        _clrwdt();
    }
}


/* ------------------------------------------------------------
 * Initialize PA2 / IPCK as UART TX GPIO
 * ------------------------------------------------------------ */
void SoftUART_Init(void)
{
    /*
     * PA2 shared function = GPIO.
     * PAS05:PAS04 = 00
     */
    _pas05 = 0;
    _pas04 = 0;

    /*
     * PA2 = output
     * internal pull-up disabled
     */
    _pac2 = 0;
    _papu2 = 0;

    /*
     * UART idle = HIGH.
     *
     * Preserve the current state of the other PA latch bits.
     */
    _pa = (unsigned char)(_pa | SOFT_UART_PA2_MASK);
}


/* ------------------------------------------------------------
 * Send one byte
 *
 * Frame:
 *   1 start bit
 *   8 data bits, LSB first
 *   1 stop bit
 *
 * No conditional branch is executed inside the timed frame.
 * ------------------------------------------------------------ */
void SoftUART_SendByte(unsigned char data)
{
    unsigned char base;
    unsigned char v0;
    unsigned char v1;
    unsigned char v2;
    unsigned char v3;
    unsigned char v4;
    unsigned char v5;
    unsigned char v6;
    unsigned char v7;
    unsigned char v_stop;
    unsigned char emi_backup;

    /*
     * Preserve the current state of the other Port A bits.
     */
    base =
        (unsigned char)
        (
            _pa &
            (unsigned char)(~SOFT_UART_PA2_MASK)
        );

    /*
     * Precompute all eight data-bit GPIO values before
     * entering the time-critical part of the frame.
     */
    v0 = (unsigned char)
         (base | ((data & 0x01U) ? SOFT_UART_PA2_MASK : 0U));

    v1 = (unsigned char)
         (base | ((data & 0x02U) ? SOFT_UART_PA2_MASK : 0U));

    v2 = (unsigned char)
         (base | ((data & 0x04U) ? SOFT_UART_PA2_MASK : 0U));

    v3 = (unsigned char)
         (base | ((data & 0x08U) ? SOFT_UART_PA2_MASK : 0U));

    v4 = (unsigned char)
         (base | ((data & 0x10U) ? SOFT_UART_PA2_MASK : 0U));

    v5 = (unsigned char)
         (base | ((data & 0x20U) ? SOFT_UART_PA2_MASK : 0U));

    v6 = (unsigned char)
         (base | ((data & 0x40U) ? SOFT_UART_PA2_MASK : 0U));

    v7 = (unsigned char)
         (base | ((data & 0x80U) ? SOFT_UART_PA2_MASK : 0U));

    v_stop = (unsigned char)(base | SOFT_UART_PA2_MASK);

    /*
     * Prevent an interrupt from disturbing bit timing.
     */
    emi_backup = _emi;
    _emi = 0;

    /*
     * Start bit = LOW
     */
    _pa = base;
    SOFT_UART_BIT_DELAY();

    /*
     * Data bits, LSB first
     */
    _pa = v0;
    SOFT_UART_BIT_DELAY();

    _pa = v1;
    SOFT_UART_BIT_DELAY();

    _pa = v2;
    SOFT_UART_BIT_DELAY();

    _pa = v3;
    SOFT_UART_BIT_DELAY();

    _pa = v4;
    SOFT_UART_BIT_DELAY();

    _pa = v5;
    SOFT_UART_BIT_DELAY();

    _pa = v6;
    SOFT_UART_BIT_DELAY();

    _pa = v7;
    SOFT_UART_BIT_DELAY();

    /*
     * Stop bit = HIGH
     */
    _pa = v_stop;
    SOFT_UART_BIT_DELAY();

    /*
     * Restore previous global interrupt state.
     */
    _emi = emi_backup;

    /*
     * Intentional debug-channel gap between bytes.
     */
    SoftUART_DelayMs(SOFT_UART_BYTE_GAP_MS);
}


/* ------------------------------------------------------------
 * Send zero-terminated ASCII string
 * ------------------------------------------------------------ */
void SoftUART_SendString(const char *text)
{
    while(*text != '\0')
    {
        SoftUART_SendByte((unsigned char)*text);
        text++;
    }
}


/* ------------------------------------------------------------
 * Send unsigned 16-bit decimal value
 *
 * This conversion avoids / and % so a small 8-bit target does
 * not need to pull unnecessary division helpers into ROM.
 * ------------------------------------------------------------ */
void SoftUART_SendUInt16(unsigned int value)
{
    unsigned char digit;
    unsigned char started;
    unsigned int divisor;

    divisor = 10000U;
    started = 0U;

    while(divisor != 0U)
    {
        digit = 0U;

        while(value >= divisor)
        {
            value -= divisor;
            digit++;
        }

        if(
            (digit != 0U) ||
            (started != 0U) ||
            (divisor == 1U)
        )
        {
            started = 1U;

            SoftUART_SendByte(
                (unsigned char)('0' + digit)
            );
        }

        if(divisor == 10000U)
        {
            divisor = 1000U;
        }
        else if(divisor == 1000U)
        {
            divisor = 100U;
        }
        else if(divisor == 100U)
        {
            divisor = 10U;
        }
        else if(divisor == 10U)
        {
            divisor = 1U;
        }
        else
        {
            divisor = 0U;
        }
    }
}


/* ------------------------------------------------------------
 * Send CR + LF
 * ------------------------------------------------------------ */
void SoftUART_NewLine(void)
{
    SoftUART_SendByte('\r');
    SoftUART_SendByte('\n');
}


/*
 * Minimal usage example
 *
 * void main(void)
 * {
 *     _wdtc = 0b10101011;
 *
 *     // fSYS = 1 MHz
 *     _cks2 = 0;
 *     _cks1 = 1;
 *     _cks0 = 1;
 *
 *     SoftUART_Init();
 *
 *     SoftUART_SendString("BOOT");
 *     SoftUART_NewLine();
 *
 *     SoftUART_SendString("ADC=");
 *     SoftUART_SendUInt16(512U);
 *     SoftUART_NewLine();
 *
 *     while(1)
 *     {
 *         _clrwdt();
 *     }
 * }
 */
