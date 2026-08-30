# BS45F3332 Software UART TX

A small, standalone software UART transmitter for the **Holtek BS45F3332**.

It is intended as a compact serial debug output for situations where a hardware UART is unavailable or another peripheral pin must be reused.

## Configuration

| Item | Value |
|---|---|
| MCU | Holtek BS45F3332 |
| System clock | 1 MHz |
| TX pin | PA2 / IPCK |
| Baud rate | 9600 |
| Frame | 8N1 |
| Direction | TX only |
| Inter-byte gap | 5 ms |

## What this implementation does

The transmitter is implemented with GPIO bit-banging.

Before a byte is transmitted, the output value for all eight data bits is calculated. The time-critical UART frame is then sent as a fixed sequence:

```text
Start bit
Bit 0
Bit 1
Bit 2
Bit 3
Bit 4
Bit 5
Bit 6
Bit 7
Stop bit
```

There is no `if/else` inside the timed start/data/stop sequence. This keeps the execution path for transmitted zeroes and ones as consistent as possible.

Interrupts are disabled only while one UART frame is shifted out and are restored immediately afterward.

A 5 ms delay is inserted after every transmitted byte. This implementation is therefore intended for debug/status messages rather than high-throughput communication.

## Timing

The tested configuration uses:

```text
fSYS = 1 MHz
9600 baud
24 x _nop() per bit delay
```

The delay is specific to this MCU/toolchain configuration.

If the system clock, compiler settings, or optimization level changes, verify the bit timing again with an oscilloscope or logic analyzer.

## Pin configuration

PA2 is configured as a GPIO output:

```c
_pas05 = 0;
_pas04 = 0;
_pac2  = 0;
_papu2 = 0;
```

The UART idle state is HIGH.

PA2 is also shared with the **IPCK programming function**, so activity during programming may appear as meaningless characters on a connected serial terminal. This is expected.

## Wiring

```text
BS45F3332 PA2/IPCK ---- 1k to 4.7k ---- USB-UART RX
BS45F3332 GND ------------------------- USB-UART GND
```

The series resistor helps isolate the shared programming/debug line.

It is not a voltage-level converter. Make sure the USB-UART receiver accepts the MCU output voltage.

## Terminal settings

```text
Baud rate    : 9600
Data bits    : 8
Parity       : None
Stop bits    : 1
Flow control : None
```

## API

```c
void SoftUART_Init(void);
void SoftUART_SendByte(unsigned char data);
void SoftUART_SendString(const char *text);
void SoftUART_SendUInt16(unsigned int value);
void SoftUART_NewLine(void);
```

## Example

```c
SoftUART_Init();

SoftUART_SendString("BOOT");
SoftUART_NewLine();

SoftUART_SendString("ADC=");
SoftUART_SendUInt16(512U);
SoftUART_NewLine();
```

Terminal output:

```text
BOOT
ADC=512
```

## Implementation detail

For each byte, the eight GPIO values are prepared before transmission:

```c
v0 = ...
v1 = ...
...
v7 = ...
```

The actual frame then contains only port writes and fixed delays.

This is the part I wanted to keep predictable: no conditional branch is evaluated while the individual UART bits are being timed.

## Notes

- TX only; there is no receive implementation.
- No hardware UART peripheral is required.
- The implementation directly writes the Port A register.
- Other Port A outputs must therefore be considered when integrating it into a larger firmware.
- The timing has been tested with the 1 MHz project configuration used here.
- Recheck timing after changing clock frequency or compiler optimization.

## File

`BS45F3332_SoftUART_PA2.c`
