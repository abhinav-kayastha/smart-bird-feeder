#include <xc.h>
#include <stdint.h>
#include <stdbool.h>

// 1) Must define FCY before <libpic30.h> for __delay_ms() macros
#define FCY 2000000UL
#include <libpic30.h>

#include <stdio.h>   // for sprintf
#include <string.h>  // for strstr

#include "mcc_generated_files/system.h"
#include "mcc_generated_files/uart1.h"
#include "mcc_generated_files/uart2.h"
#include "mcc_generated_files/adc1.h"
#include "mcc_generated_files/tmr1.h"
#include "mcc_generated_files/interrupt_manager.h"

// ------------------- SERVO & PIN CONFIG -------------------
#define SERVO_PIN_HIGH() LATAbits.LATA0 = 1
#define SERVO_PIN_LOW()  LATAbits.LATA0 = 0

static int last_ra7_state = -1;
static int last_rb12_state = -1;
static int last_rb14_state = -1;
static int servo_moved = 0; // 0 = OPEN(0°), 1 = CLOSED(90°)

// ------------------- TMR1: 1s interrupt -> track time -------------------
static volatile uint32_t seconds_since_epoch = 0;
void __attribute__((__interrupt__, no_auto_psv)) _T1Interrupt(void)
{
    IFS0bits.T1IF = 0;
    seconds_since_epoch++;
}

// ------------------- UART1 Debug Print -------------------
void UART1_WriteString(const char *str)
{
    while(*str)
    {
        while(!UART1_IsTxReady());
        UART1_Write(*str++);
    }
}

// ------------------- LoRa Communication (UART2) -------------------
void LoRa_SendCommand(const char *cmd)
{
    UART1_WriteString("\r\n[LoRa] Sending Command: ");
    UART1_WriteString(cmd);
    UART1_WriteString("\r\n");

    while(*cmd)
    {
        while(!UART2_IsTxReady());
        UART2_Write(*cmd++);
    }
    // CR+LF
    UART2_WriteString("\r\n");
}

void LoRa_ReadResponse(char *buffer, uint16_t max_length)
{
    UART1_WriteString("[LoRa] Waiting for Response...\r\n");

    uint16_t index = 0;
    uint16_t timeoutCount = 0;
    const uint16_t TIMEOUT_LIMIT = 5000; // ~5 seconds

    while(index < (max_length - 1))
    {
        if(UART2_IsRxReady())
        {
            buffer[index++] = UART2_Read();
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
    buffer[index] = '\0';

    UART1_WriteString("[LoRa] Response: ");
    UART1_WriteString(buffer);
    UART1_WriteString("\r\n");
}

bool LoRa_IsJoined(void)
{
    char response[128];
    LoRa_SendCommand("AT+JOIN?");
    __delay_ms(2000);
    LoRa_ReadResponse(response, sizeof(response));

    if(strstr(response, "Network joined") != NULL)
    {
        UART1_WriteString("[LoRa] Already Joined!\r\n");
        return true;
    }
    UART1_WriteString("[LoRa] Not Joined.\r\n");
    return false;
}

void LoRa_JoinNetwork(void)
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

// Send a plain text message: "AT+CMSG="<status>""
void LoRa_SendMessage(const char *status)
{
    char cmd[80];
    sprintf(cmd, "AT+CMSG=\"%s\"", status);

    LoRa_SendCommand(cmd);
    __delay_ms(2000);

    char response[128];
    LoRa_ReadResponse(response, sizeof(response));

    if(strstr(response, "+CMSG: Done") != NULL)
    {
        UART1_WriteString("[LoRa] Message Sent OK!\r\n");
    }
    else
    {
        UART1_WriteString("[LoRa] Message Send Failed!\r\n");
    }
}

// ------------------- SERVO Logic -------------------
void move_servo_90_degrees(void)
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
        __delay_ms(18); // ~20 - 1.5 => ~18.5
    }
    servo_moved = 1;
    UART1_WriteString("[SERVO] Moved to 90°\r\n");

    // Send "CLOSED"
    LoRa_SendMessage("CLOSED");
}

void move_servo_0_degrees(void)
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
        __delay_ms(19); // ~20 - 0.5 => ~19.5
    }
    servo_moved = 0;
    UART1_WriteString("[SERVO] Moved to 0°\r\n");

    // Send "OPEN"
    LoRa_SendMessage("OPEN");
}

// ------------------- MAIN -------------------
int main(void)
{
    // 1) Initialize
    SYSTEM_Initialize();
    UART1_Initialize();
    UART2_Initialize();
    ADC1_Initialize(); 
    // TMR1_Initialize(); // If MCC doesn't auto-initialize
    // TMR1_Start();      // If MCC doesn't auto-start

    UART1_WriteString("\r\n[MAIN] Starting...\r\n");

    // 2) Check if joined
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
    ANSAbits.ANSA0 = 0;     
    TRISAbits.TRISA0 = 0;   // servo pin

    // 5) Add a battery send interval (ex: 600s)
    uint32_t lastBatteryTime = 0;
    const uint32_t BATT_INTERVAL_SEC = 10U;

    // 6) Main loop
    while(1)
    {
        int ra7_state = PORTAbits.RA7;
        int rb12_state = PORTBbits.RB12;
        int rb14_state = PORTBbits.RB14;

        // Switch => servo=90
        if(ra7_state != last_ra7_state || rb12_state != last_rb12_state)
        {
            char dbg[64];
            sprintf(dbg, "[SWITCH] RA7=%d, RB12=%d\r\n", ra7_state, rb12_state);
            UART1_WriteString(dbg);

            if(ra7_state == 0 && rb12_state == 0 && servo_moved == 0)
            {
                move_servo_90_degrees();
            }
            last_ra7_state = ra7_state;
            last_rb12_state = rb12_state;
        }

        // IR => if servo closed & IR=1 => wait 5s => open
        if(rb14_state != last_rb14_state)
        {
            char dbg[64];
            sprintf(dbg, "[IR] RB14 => %d\r\n", rb14_state);
            UART1_WriteString(dbg);
            last_rb14_state = rb14_state;
        }

        if(servo_moved && rb14_state == 1)
        {
            UART1_WriteString("[IR] RB14=HIGH => 5s wait...\r\n");
            int stable = 0;
            while(stable < 5000)
            {
                __delay_ms(100);
                if(PORTBbits.RB14 == 1) stable += 100;
                else
                {
                    UART1_WriteString("[IR] LOW => reset.\r\n");
                    stable = 0;
                    break;
                }
            }
            if(stable >= 5000)
            {
                move_servo_0_degrees();
            }
        }

        // ---- Battery raw reading every 600s (10 min) ----
        if((seconds_since_epoch - lastBatteryTime) >= BATT_INTERVAL_SEC)
        {
            lastBatteryTime = seconds_since_epoch;

            // 1) Get raw ADC reading from AN20
            uint16_t raw = ADC1_GetConversion(channel_AN20);

            // 2) Print the raw reading over UART1
            char dbgBuf[32];
            sprintf(dbgBuf, "[BATT] RAW=%u\r\n", (unsigned)raw);
            UART1_WriteString(dbgBuf);

            // 3) Send the same raw reading via LoRa
            char loraBuf[32];
            sprintf(loraBuf, "BATT=%u", (unsigned)raw);
            LoRa_SendMessage(loraBuf);
        }

        __delay_ms(250);
    }

    return 0;
}
