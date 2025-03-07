#include <xc.h>
#include <stdint.h>
#include <stdbool.h>

// Must define FCY before <libpic30.h> so __delay_ms() works
#define FCY 2000000UL
#include <libpic30.h>   // for __delay_ms()

#include <stdio.h>      // for sprintf
#include <string.h>     // for strstr

#include "mcc_generated_files/system.h"
#include "mcc_generated_files/uart1.h"
#include "mcc_generated_files/uart2.h"
#include "mcc_generated_files/adc1.h"
#include "mcc_generated_files/tmr1.h"
#include "mcc_generated_files/interrupt_manager.h"

// ----------------------------------------------------------------------
// SERVO & PIN CONFIG
// ----------------------------------------------------------------------
#define SERVO_PIN_HIGH() LATAbits.LATA0 = 1
#define SERVO_PIN_LOW()  LATAbits.LATA0 = 0

// servo_moved = 0 => servo is OPEN(0°); 1 => CLOSED(90°)
static int  servo_moved     = 0;

// We introduce a global lock mode. If true => IR sensor cannot open servo.
static bool g_lockMode      = false;

static int  last_ra7_state  = -1;
static int  last_rb12_state = -1;
static int  last_rb14_state = -1;

// ----------------------------------------------------------------------
// TMR1 Callback approach (MCC calls TMR1_CallBack() from its ISR):
// We'll keep track of "seconds_since_epoch" in that callback.
// ----------------------------------------------------------------------
static volatile uint32_t seconds_since_epoch = 0;

void TMR1_CallBack(void)
{
    // Increments once per second (assuming TMR1 is ~1s period)
    seconds_since_epoch++;
}

// ----------------------------------------------------------------------
// Minimal helper to write strings via UART1
// ----------------------------------------------------------------------
void UART1_WriteString(const char *str)
{
    while(*str)
    {
        while(!UART1_IsTxReady());
        UART1_Write(*str++);
    }
}

// ----------------------------------------------------------------------
// LoRa Communication (UART2)
// ----------------------------------------------------------------------

// Send a raw command to the module
static void LoRa_SendCommand(const char *cmd)
{
    UART1_WriteString("\r\n[LoRa] Sending Command: ");
    UART1_WriteString(cmd);
    UART1_WriteString("\r\n");

    // Write out each char
    while(*cmd)
    {
        while(!UART2_IsTxReady());
        UART2_Write(*cmd++);
    }
    // CR+LF
    UART2_WriteString("\r\n");
}

// The single place we read lines from the LoRa module.
// We also parse any downlink lines (like +CMSG: ... RX: "30") right here.
static void LoRa_ReadResponse(char *buffer, uint16_t max_length)
{
    UART1_WriteString("[LoRa] Waiting for Response...\r\n");

    uint16_t index        = 0;
    uint16_t timeoutCount = 0;
    const uint16_t TIMEOUT_LIMIT = 5000; // ~5 seconds

    // Read chars until no more or we time out
    while(index < (max_length - 1))
    {
        if(UART2_IsRxReady())
        {
            char c = UART2_Read();
            buffer[index++] = c;
            timeoutCount = 0;
        }
        else
        {
            __delay_ms(1);
            if(++timeoutCount >= TIMEOUT_LIMIT)
            {
                UART1_WriteString("ERROR: LoRa response timeout!\r\n");
                break;
            }
        }
    }

    buffer[index] = '\0'; // Terminate

    // Print entire response for debug
    UART1_WriteString("[LoRa] Response: ");
    UART1_WriteString(buffer);
    UART1_WriteString("\r\n");

    // -------------------------------------------------
    // **Here** we parse for the line containing e.g.
    // +CMSG: PORT: 10; RX: "30"
    // We'll search for 'RX: "'
    // then check if the next 2 chars are "30" or "31"
    // -------------------------------------------------
    {
        char *rxPtr = strstr(buffer, "RX: \"");
        if(rxPtr != NULL)
        {
            rxPtr += 5; // skip past 'RX: "'
            // Now rxPtr points to e.g. "30" or "31" or "Hello"

            // Check for '30' => user wants '0' => UNLOCK
            if(strncmp(rxPtr, "30", 2) == 0)
            {
                UART1_WriteString("[LoRa Downlink] Received 0 => UNLOCK mode\r\n");
                g_lockMode = false;
            }
            // Check for '31' => user wants '1' => LOCK
            else if(strncmp(rxPtr, "31", 2) == 0)
            {
                UART1_WriteString("[LoRa Downlink] Received 1 => LOCK mode\r\n");
                g_lockMode = true;
            }
            else
            {
                // Some other data
                UART1_WriteString("[LoRa Downlink] Received unrecognized data.\r\n");
            }
        }
    }
}

// Query if joined
static bool LoRa_IsJoined(void)
{
    char response[128];
    LoRa_SendCommand("AT+JOIN?");
    __delay_ms(2000);
    LoRa_ReadResponse(response, sizeof(response));

    // If we see "Network joined"
    if(strstr(response, "Network joined") != NULL)
    {
        UART1_WriteString("[LoRa] Already Joined!\r\n");
        return true;
    }
    UART1_WriteString("[LoRa] Not Joined.\r\n");
    return false;
}

// Perform a fresh join
static void LoRa_JoinNetwork(void)
{
    char response[128];

    // 1) Set the AppKey
    LoRa_SendCommand("AT+KEY=APPKEY,\"31A7F455E86B5B0AEC7796583CAE2380\"");
    __delay_ms(1000);
    LoRa_ReadResponse(response, sizeof(response));

    // 2) Join
    LoRa_SendCommand("AT+JOIN");
    __delay_ms(5000);
    LoRa_ReadResponse(response, sizeof(response));
    if(strstr(response, "Network joined") != NULL)
    {
        UART1_WriteString("[LoRa] Join Successful!\r\n");
    }
    else
    {
        UART1_WriteString("[LoRa] Join Failed!\r\n");
    }
}

// Send a text message via LoRa
static void LoRa_SendMessage(const char *status)
{
    char cmd[80];
    sprintf(cmd, "AT+CMSG=\"%s\"", status);

    LoRa_SendCommand(cmd);
    __delay_ms(2000);

    // Read lines from module
    char response[128];
    LoRa_ReadResponse(response, sizeof(response));

    // If it has +CMSG: Done => success
    if(strstr(response, "+CMSG: Done") != NULL)
    {
        UART1_WriteString("[LoRa] Message Sent OK!\r\n");
    }
    else
    {
        UART1_WriteString("[LoRa] Message Send Failed!\r\n");
    }
}

// ----------------------------------------------------------------------
// SERVO Logic
// ----------------------------------------------------------------------
static void move_servo_90_degrees(void)
{
    if(servo_moved == 1)
    {
        UART1_WriteString("[SERVO] Already at 90°\r\n");
        return;
    }
    UART1_WriteString("[SERVO] Moving to 90°...\r\n");

    for(int i = 0; i < 50; i++)
    {
        SERVO_PIN_HIGH();
        __delay_us(1500);
        SERVO_PIN_LOW();
        __delay_ms(18);
    }
    servo_moved = 1;
    UART1_WriteString("[SERVO] Moved to 90°\r\n");

    // Optionally send message "CLOSED"
    LoRa_SendMessage("CLOSED");
}

static void move_servo_0_degrees(void)
{
    if(servo_moved == 0)
    {
        UART1_WriteString("[SERVO] Already at 0°\r\n");
        return;
    }
    UART1_WriteString("[SERVO] Moving to 0°...\r\n");

    for(int i = 0; i < 50; i++)
    {
        SERVO_PIN_HIGH();
        __delay_us(500);
        SERVO_PIN_LOW();
        __delay_ms(19);
    }
    servo_moved = 0;
    UART1_WriteString("[SERVO] Moved to 0°\r\n");

    // Optionally send message "OPEN"
    LoRa_SendMessage("OPEN");
}

// ----------------------------------------------------------------------
// MAIN
// ----------------------------------------------------------------------
int main(void)
{
    // 1) Initialize MCC modules (system, UART, TMR1, etc.)
    SYSTEM_Initialize(); 
    TMR1_Start();  // so TMR1_Callback increments 'seconds_since_epoch'

    UART1_WriteString("\r\n[MAIN] Starting...\r\n");

    // 2) Check if LoRa is joined
    if(!LoRa_IsJoined())
    {
        LoRa_JoinNetwork();
    }

    // 3) Assume servo is open (0°)
    servo_moved = 0;
    UART1_WriteString("[MAIN] Monitoring RA7/RB12 => servo, RB14 => IR...\r\n");

    // 4) Configure pins for RA7, RB12, RB14, RA0
    TRISAbits.TRISA7 = 1;   // RA7 input
    TRISBbits.TRISB12 = 1;  // RB12 input
    TRISBbits.TRISB14 = 1;  // RB14 input
    ANSAbits.ANSA0   = 0;   
    TRISAbits.TRISA0 = 0;   // servo pin

    // 5) Battery reading interval
    uint32_t lastBatteryTime   = 0;
    const uint32_t BATT_PERIOD = 6000U;  // 600 => ~10 min if TMR1 is 1 second

    // 6) Main loop
    int last_ra7 = -1;
    int last_rb12= -1;
    int last_rb14= -1;

    while(1)
    {
        // Switch logic: if RA7 & RB12 both pressed => close servo
        int ra7_state  = PORTAbits.RA7;
        int rb12_state = PORTBbits.RB12;
        int rb14_state = PORTBbits.RB14;

        if((ra7_state != last_ra7) || (rb12_state != last_rb12))
        {
            char dbg[64];
            sprintf(dbg, "[SWITCH] RA7=%d, RB12=%d\r\n", ra7_state, rb12_state);
            UART1_WriteString(dbg);

            // If both pressed => close servo
            // (Lock mode does NOT prevent manual close.)
            if(ra7_state == 0 && rb12_state == 0 && servo_moved == 0)
            {
                move_servo_90_degrees();
            }
            last_ra7   = ra7_state;
            last_rb12  = rb12_state;
        }

        // IR logic => if servo is closed & IR=1 => wait 5s => open
        // BUT if g_lockMode == true, do NOT open
        if(rb14_state != last_rb14)
        {
            char dbg[64];
            sprintf(dbg, "[IR] RB14 => %d\r\n", rb14_state);
            UART1_WriteString(dbg);
            last_rb14 = rb14_state;
        }

        if(servo_moved && (rb14_state == 1))
        {
            UART1_WriteString("[IR] RB14=HIGH => 5s wait...\r\n");
            int stable = 0;
            while(stable < 5000)
            {
                __delay_ms(100);
                if(PORTBbits.RB14 == 1) 
                    stable += 100;
                else
                {
                    UART1_WriteString("[IR] LOW => reset.\r\n");
                    stable = 0;
                    break;
                }
            }
            // If still high after 5s => open, but only if not locked
            if(stable >= 5000)
            {
                if(!g_lockMode)
                {
                    move_servo_0_degrees();
                }
                else
                {
                    UART1_WriteString("[LOCK] IR tried to open, but servo is locked.\r\n");
                }
            }
        }

        // Battery reading every BATT_PERIOD seconds
        if((seconds_since_epoch - lastBatteryTime) >= BATT_PERIOD)
        {
            lastBatteryTime = seconds_since_epoch;

            uint16_t raw = ADC1_GetConversion(channel_AN20);
            char dbgBuf[32];
            sprintf(dbgBuf, "[BATT]%u\r\n", (unsigned)raw);
            UART1_WriteString(dbgBuf);

            // Also send via LoRa
            char loraBuf[32];
            sprintf(loraBuf, "[BATT=%u", (unsigned)raw);
            LoRa_SendMessage(loraBuf);
        }

        __delay_ms(200);
    }

    return 0;
}
