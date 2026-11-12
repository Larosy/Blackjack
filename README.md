# Embedded Blackjack
### PIC18F4550 · C · Nextion UART interface

An embedded Blackjack-style game built around the PIC18F4550 microcontroller. The firmware combines keypad input, a Nextion display, status LEDs, a buzzer, and a timer-driven servo output.

The project explores how game logic and hardware peripherals can work together on a microcontroller.

## Implemented behavior

- **Hit:** draw a card and update the player's score.
- **Stand:** draw dealer cards until the dealer reaches at least 17.
- **Round result:** show a win, loss, or bust message on the display.
- **Lives:** losing a round reduces the remaining lives and activates feedback.
- **Restart:** after game over, the C key restores three lives.

This is a simplified implementation: the active game loop counts an ace as 1 and treats a tie as a loss. It does not implement the full casino ruleset.

## Hardware interface

The following assignments come from `blackjack.c`. Confirm them against the actual circuit before connecting hardware.

| Peripheral | Firmware assignment |
| --- | --- |
| Nextion display | UART TX on RC6, RX on RC7; configured for 9600 baud at 20 MHz |
| 4×4 keypad | PORTB: RB0–RB3 row outputs, RB4–RB7 column inputs |
| Buzzer | RC1 |
| Servo signal | RC2 |
| Life indicators | RD0–RD2 |

### Key mapping

| Key | Action |
| --- | --- |
| 7 | Hit |
| 8 | Stand |
| C | Restart after game over |

The active display code sends commands to `t_player`, `t_dealer`, `t_result`, and `n_lives`. A separate card-image helper references picture components but is not called by the current game loop.

## Build and run

The checked-in MPLAB X configuration targets **PIC18F4550** and **XC8 3.10**, with Microchip's **PIC18Fxxxx_DFP 1.7.171** device pack.

1. Clone this repository.
2. Open the project folder in MPLAB X IDE.
3. Select the installed XC8 toolchain and PIC18F4550 device support.
4. Build the project, reviewing any local toolchain-path settings.
5. Program a compatible board with an appropriate programmer and a circuit matching the firmware configuration.

The source configures a 20 MHz external oscillator. The saved simulator configuration uses a different oscillator setting; align simulator timing with the firmware before relying on UART or delay behavior.

## Repository contents

- `blackjack.c` — game logic and peripheral handling.
- `nbproject/` — MPLAB X project configuration.
- `Makefile` — generated build entry point.
- `build/`, `dist/` — checked-in build artifacts.

## Current documentation limits

A circuit schematic, Nextion HMI project, and hardware demonstration are not included in the repository. Reproducing the complete device requires those details. The servo timing and physical behavior need hardware validation; a successful build alone does not establish correct operation.

The firmware also uses a fixed random seed, so card sequences repeat after reset. Future improvements could include documenting the circuit, adding the HMI assets, improving card rules, and recording a hardware demo.

## Author

[Ozan Honça](https://www.linkedin.com/in/ozan-honca/) · [GitHub](https://github.com/Larosy)
