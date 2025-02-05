#include <xc.h>
#include <stdint.h>
#define FCY 2000000UL  // Instruction clock frequency (2 MHz)
#include <libpic30.h>

#include "mcc_generated_files/system.h"
#include "mcc_generated_files/uart2.h"

// UART Write function
void UART2_WriteString(const char *str) {
    while (*str) {
        while (!UART2_IsTxReady());
        UART2_Write(*str++);
    }
}

// Servo Control Macros
#define SERVO_PIN_HIGH() LATAbits.LATA0 = 1
#define SERVO_PIN_LOW()  LATAbits.LATA0 = 0

// Last known states
int last_ra7_state = -1;
int last_rb12_state = -1;
int last_rb14_state = -1;  // Track RB14 state (IR Sensor)

int servo_moved = 0;       // Track if the servo has been moved

// Servo Movement Functions
void move_servo_90_degrees() {
    UART2_WriteString("Servo Moved to 90° (Switches PRESSED)\r\n");
    for (int i = 0; i < 50; i++) {
        SERVO_PIN_HIGH();
        __delay_us(1500);       // 1.5 ms HIGH for 90°
        SERVO_PIN_LOW();
        __delay_ms(20 - 1.5);   // LOW for the rest of the 20 ms period
    }
    servo_moved = 1; // Mark that the servo has been moved
}

void move_servo_0_degrees() {
    UART2_WriteString("Servo Returned to 0° (After RB14 HIGH for 5s)\r\n");
    for (int i = 0; i < 50; i++) {
        SERVO_PIN_HIGH();
        __delay_us(500);        // 0.5 ms HIGH for 0°
        SERVO_PIN_LOW();
        __delay_ms(20 - 0.5);   // LOW for the rest of the 20 ms period
    }
    servo_moved = 0; // Reset the servo moved flag
}

int main(void) {
    SYSTEM_Initialize();
    UART2_Initialize();

    // ? Configure Pins
    TRISAbits.TRISA7 = 1;         // Set RA7 as input
    TRISBbits.TRISB12 = 1;        // Set RB12 as input
    TRISBbits.TRISB14 = 1;        // Set RB14 as input (IR Sensor)
    ANSAbits.ANSA0 = 0;           // Disable analog on RA0
    TRISAbits.TRISA0 = 0;         // Set RA0 as output (Servo)

    // ? Enable Internal Pull-ups
    //CNPU1bits.CN9PUE = 1;         // Pull-up for RA7 (CN9)
    //CNPU2bits.CN14PUE = 1;        // Pull-up for RB12 (CN14)
    //CNPU2bits.CN12PUE = 1;        // Pull-up for RB14 (CN12 - IR Sensor)

    UART2_WriteString("RA7, RB12, RB14 (IR Sensor) Test Started\r\n");

    while (1) {
        int ra7_state = PORTAbits.RA7;    // Read RA7
        int rb12_state = PORTBbits.RB12;  // Read RB12
        int rb14_state = PORTBbits.RB14;  // Read RB14 (IR Sensor)

        // ? Check for RA7 and RB12 changes
        if (ra7_state != last_ra7_state || rb12_state != last_rb12_state) {
            UART2_WriteString("RA7: ");
            UART2_Write(ra7_state + '0');
            UART2_WriteString(", RB12: ");
            UART2_Write(rb12_state + '0');
            UART2_WriteString("\r\n");

            // **Trigger servo when BOTH switches are LOW (pressed)**
            if (ra7_state == 0 && rb12_state == 0 && servo_moved == 0) {
                move_servo_90_degrees();
            }

            last_ra7_state = ra7_state;
            last_rb12_state = rb12_state;
        }

        // ? Check for RB14 (IR Sensor) changes
        if (rb14_state != last_rb14_state) {
            UART2_WriteString("RB14 (IR Sensor) Changed: ");
            UART2_Write(rb14_state + '0');  // Convert to ASCII
            UART2_WriteString("\r\n");

            last_rb14_state = rb14_state;
        }

        // ?? Wait for RB14 to be HIGH for 5 seconds before returning servo
        if (servo_moved && rb14_state == 1) {
            UART2_WriteString("RB14 HIGH detected, starting 5-second timer...\r\n");

            int stable_high_duration = 0;
            int debounce_check = 0;

            // Check RB14 remains HIGH for 5 continuous seconds
            while (stable_high_duration < 5000) {
                __delay_ms(100);
                debounce_check = PORTBbits.RB14;

                if (debounce_check == 1) {
                    stable_high_duration += 100;  // Accumulate time if still HIGH
                } else {
                    stable_high_duration = 0;    // Reset if it goes LOW
                    UART2_WriteString("RB14 went LOW, timer reset.\r\n");
                    break;
                }
            }

            // If RB14 stayed HIGH for 5 seconds, return the servo
            if (stable_high_duration >= 5000) {
                move_servo_0_degrees();
            }
        }

        __delay_ms(100);  // Debounce delay
    }

    return 0;
}