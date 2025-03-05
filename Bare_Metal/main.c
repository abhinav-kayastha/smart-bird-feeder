#include <xc.h>
#include <stdint.h>
#include <stdbool.h>
#define FCY 2000000UL  // Instruction clock frequency (2 MHz)
#include <libpic30.h>
#include <stdio.h>
#include <string.h>

#include "mcc_generated_files/system.h"
#include "mcc_generated_files/uart1.h"  // Debugging via UART1
#include "mcc_generated_files/uart2.h"  // LoRa communication via UART2
#include "mcc_generated_files/interrupt_manager.h"
#include "mcc_generated_files/tmr1.h"

// ------------------- CONFIG: Base epoch in Unix time (e.g., Mar 15 2023)
#define EPOCH_BASE 1678900000UL

// ------------------- Timestamp Variables -------------------
static volatile uint32_t seconds_since_epoch = 0;

// ------------------- SERVO & PIN CONFIG -------------------
#define SERVO_PIN_HIGH() LATAbits.LATA0 = 1
#define SERVO_PIN_LOW()  LATAbits.LATA0 = 0

static int last_ra7_state = -1;
static int last_rb12_state = -1;
static int last_rb14_state = -1;

static int servo_moved = 0; // 0 = OPEN(0°), 1 = CLOSED(90°)

// ------------------- UART1 DEBUG -------------------
void UART1_WriteString(const char *str) {
    while (*str) {
        while (!UART1_IsTxReady());
        UART1_Write(*str++);
    }
}

// ------------------- LoRa Communication (UART2) -------------------
void LoRa_SendCommand(const char *cmd) {
    UART1_WriteString("\r\n[LoRa] Sending Command: ");
    UART1_WriteString(cmd);
    UART1_WriteString("\r\n");

    while (*cmd) {
        while (!UART2_IsTxReady());
        UART2_Write(*cmd++);
    }
    // CR+LF
    UART2_WriteString("\r\n");
}

void LoRa_ReadResponse(char *buffer, uint16_t max_length) {
    UART1_WriteString("[LoRa] Waiting for Response...\r\n");

    uint16_t index = 0;
    uint16_t timeoutCount = 0;
    const uint16_t TIMEOUT_LIMIT = 5000; // ~5 seconds

    while (index < (max_length - 1)) {
        if (UART2_IsRxReady()) {
            buffer[index++] = UART2_Read();
            timeoutCount = 0; 
        } else {
            __delay_ms(1);
            timeoutCount++;
            if (timeoutCount >= TIMEOUT_LIMIT) {
                UART1_WriteString("ERROR: LoRa response timeout!\r\n");
                break;
            }
        }
    }
    buffer[index] = '\0';

    UART1_WriteString("[LoRa] Response: ");
    UART1_WriteString(buffer);
    UART1_WriteString("\r\n");
}

bool LoRa_IsJoined(void) {
    char response[128];
    LoRa_SendCommand("AT+JOIN?");
    __delay_ms(2000);
    LoRa_ReadResponse(response, sizeof(response));

    if (strstr(response, "Network joined") != NULL) {
        UART1_WriteString("[LoRa] Already Joined!\r\n");
        return true;
    }
    UART1_WriteString("[LoRa] Not Joined.\r\n");
    return false;
}

void LoRa_JoinNetwork(void) {
    char response[128];

    // Set the AppKey
    LoRa_SendCommand("AT+KEY=APPKEY,\"31A7F455E86B5B0AEC7796583CAE2380\"");
    __delay_ms(1000);
    LoRa_ReadResponse(response, sizeof(response));

    // Join
    LoRa_SendCommand("AT+JOIN");
    __delay_ms(5000);
    LoRa_ReadResponse(response, sizeof(response));

    if (strstr(response, "Network joined") != NULL) {
        UART1_WriteString("[LoRa] Join Successful!\r\n");
    } else {
        UART1_WriteString("[LoRa] Join Failed!\r\n");
    }
}

// Return current Unix time
uint32_t get_unix_time(void) {
    return EPOCH_BASE + seconds_since_epoch;
}

// Send LoRa message with accurate Unix time
void LoRa_SendMessage(const char *status) {
    char cmd[80];
    uint32_t now = get_unix_time();

    // Format:  "<unix_time>,<status>"
    sprintf(cmd, "AT+CMSG=\"%lu,%s\"", (unsigned long)now, status);

    LoRa_SendCommand(cmd);
    __delay_ms(2000);

    char response[128];
    LoRa_ReadResponse(response, sizeof(response));

    if (strstr(response, "+CMSG: Done") != NULL) {
        UART1_WriteString("[LoRa] Message Sent OK!\r\n");
    } else {
        UART1_WriteString("[LoRa] Message Send Failed!\r\n");
    }
}

// ------------------- SERVO LOGIC -------------------
void move_servo_90_degrees(void) {
    if (servo_moved == 1) {
        UART1_WriteString("[SERVO] Already at 90°\r\n");
        return;
    }
    UART1_WriteString("[SERVO] Moving to 90°...\r\n");
    for (int i = 0; i < 50; i++) {
        SERVO_PIN_HIGH();
        __delay_us(1500);
        SERVO_PIN_LOW();
        __delay_ms(20 - 1.5);
    }
    servo_moved = 1;
    UART1_WriteString("[SERVO] Moved to 90°\r\n");

    // LoRa message "CLOSED"
    LoRa_SendMessage("CLOSED");
}

void move_servo_0_degrees(void) {
    if (servo_moved == 0) {
        UART1_WriteString("[SERVO] Already at 0°\r\n");
        return;
    }
    UART1_WriteString("[SERVO] Moving to 0°...\r\n");
    for (int i = 0; i < 50; i++) {
        SERVO_PIN_HIGH();
        __delay_us(500);
        SERVO_PIN_LOW();
        __delay_ms(20 - 0.5);
    }
    servo_moved = 0;
    UART1_WriteString("[SERVO] Moved to 0°\r\n");

    // LoRa message "OPEN"
    LoRa_SendMessage("OPEN");
}

// ------------------- TMR1 ISR: 1s interrupt to increment 'seconds_since_epoch' -------------------
// 1) In MCC, configure TMR1 for 1 second period
// 2) Enable TMR1 interrupt
// 3) Add TMR1 ISR in interrupt_manager.c or pin_manager.c (MCC) or here manually
//    For example:

void __attribute__((__interrupt__, no_auto_psv)) _T1Interrupt(void)
{
    IFS0bits.T1IF = 0;  // Clear TMR1 flag
    seconds_since_epoch++;  // Increment global seconds
}

// ------------------- MAIN -------------------
int main(void) {
    SYSTEM_Initialize();
    UART1_Initialize();  
    UART2_Initialize();  

    // Make sure TMR1 is configured for 1-second interrupts in MCC
    // TMR1_Initialize();  // If not auto-called by MCC
    // TMR1_Start();       // If not auto-started by MCC

    UART1_WriteString("\r\n[MAIN] Starting...\r\n");

    // 1) Check if joined
    if (!LoRa_IsJoined()) {
        // 2) Attempt to join
        LoRa_JoinNetwork();
    }

    // Debug: Assume servo starts at 0° for demonstration
    servo_moved = 0; 
    UART1_WriteString("[MAIN] Setup done. Monitoring RA7/RB12 (Switches), RB14 (IR Sensor)...\r\n");

    // Configure pins
    TRISAbits.TRISA7 = 1;   // RA7 input
    TRISBbits.TRISB12 = 1;  // RB12 input
    TRISBbits.TRISB14 = 1;  // RB14 input
    ANSAbits.ANSA0 = 0;     
    TRISAbits.TRISA0 = 0;  

    while (1) {
        int ra7_state = PORTAbits.RA7;
        int rb12_state = PORTBbits.RB12;
        int rb14_state = PORTBbits.RB14;

        // Check RA7/RB12 => move servo to 90
        if (ra7_state != last_ra7_state || rb12_state != last_rb12_state) {
            char dbg[50];
            sprintf(dbg, "[SWITCH] RA7=%d, RB12=%d\r\n", ra7_state, rb12_state);
            UART1_WriteString(dbg);

            if (ra7_state == 0 && rb12_state == 0 && servo_moved == 0) {
                move_servo_90_degrees();
            }

            last_ra7_state = ra7_state;
            last_rb12_state = rb12_state;
        }

        // Check RB14 => if servo closed & IR is HIGH for 5s => open
        if (rb14_state != last_rb14_state) {
            char dbg[50];
            sprintf(dbg, "[IR] RB14 changed => %d\r\n", rb14_state);
            UART1_WriteString(dbg);
            last_rb14_state = rb14_state;
        }

        if (servo_moved && rb14_state == 1) {
            UART1_WriteString("[IR] RB14=HIGH, waiting 5s...\r\n");
            int stable = 0;
            while (stable < 5000) {
                __delay_ms(100);
                if (PORTBbits.RB14 == 1) {
                    stable += 100;
                } else {
                    UART1_WriteString("[IR] Went LOW, reset.\r\n");
                    stable = 0;
                    break;
                }
            }
            if (stable >= 5000) {
                move_servo_0_degrees();
            }
        }

        __delay_ms(250); // main loop delay
    }

    return 0;
}
