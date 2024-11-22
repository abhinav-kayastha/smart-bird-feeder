#include <iostream>
#include <sstream>
#include <cstring>
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "hardware/gpio.h"
#include "IPStack.h"

#include "hardware/timer.h"
extern "C" {
uint32_t read_runtime_ctr(void) {
    return timer_hw->timerawl;
}
}


#if 1
#define HTTP_SERVER        "3.224.58.169"
//#define HTTP_SERVER        "api.thingspeak.com"
#define BUFSIZE 2048
#endif

 /* Add a command to Talkback queue with curl (a command line utility)
  * curl -v -d "command_string=my%20fancy%20command&api_key=371DAWENQKI6J8DD" http://api.thingspeak.com/talkbacks/52920/commands
  * */
void test_task(void *param) {
    (void) param;
#if 0
    // this works
    const char *req = "POST /talkbacks/52920/commands/execute.json HTTP/1.1\r\n"
                      "Host: api.thingspeak.com\r\n"
                      "User-Agent: PicoW\r\n"
                      "Accept: */*\r\n"
                      "Content-Length: 24\r\n"
                      "Content-Type: application/x-www-form-urlencoded\r\n"
                      "\r\n"
                      "api_key=371DAWENQKI6J8DD";
#endif
#if 1
     // Execute (= get and remove) next command from talkback queue - tested to work
    const char *req = "POST /talkbacks/52920/commands/execute.json HTTP/1.1\r\n"
                      "Host: api.thingspeak.com\r\n"
                      "Content-Length: 24\r\n"
                      "Content-Type: application/x-www-form-urlencoded\r\n"
                      "\r\n"
                      "api_key=371DAWENQKI6J8DD";
#endif
#if 0
    // Update fields using a POST request and execute (= get and remove) next command from talkback queue - tested to work
    const char *req = "POST /update.json HTTP/1.1\r\n"
                      "Host: api.thingspeak.com\r\n"
                      "User-Agent: PicoW\r\n"
                      "Accept: */*\r\n"
                      "Content-Length: 65\r\n"
                      "Content-Type: application/x-www-form-urlencoded\r\n"
                      "\r\n"
                      "field1=370&api_key=1WWH2NWXSM53URR5&talkback_key=371DAWENQKI6J8DD";
#endif
#if 0
    // Update fields using a GET request - tested to work
    const char *req = "GET /update?api_key=1WWH2NWXSM53URR5&field1=440&field2=44.7 HTTP/1.1\r\n"
                      "Host: api.thingspeak.com\r\n"
                      "User-Agent: PicoW\r\n"
                      "Accept: */*\r\n"
                      "Content-Length: 0\r\n"
                      "Content-Type: application/x-www-form-urlencoded\r\n"
                      "\r\n";
#endif
#if 0
    // Update fields using a minimal GET request - tested to work
    const char *req = "GET /update?api_key=1WWH2NWXSM53URR5&field1=410&field2=45.7 HTTP/1.1\r\n"
                      "Host: api.thingspeak.com\r\n"
                      "\r\n";
#endif
#if 0
    // List all talkback commands - tested to work
    const char *req = "GET /talkbacks/52920/commands.json?api_key=371DAWENQKI6J8DD HTTP/1.1\r\n"
                      "Host: api.thingspeak.com\r\n"
                      "\r\n";
#endif
    printf("\nconnecting...\n");

    unsigned char *buffer = new unsigned char[BUFSIZE];
    // todo: Add failed connection handling
    //IPStack ipstack("SmartIotMQTT", "SmartIot"); // example
    IPStack ipstack(WIFI_SSID, WIFI_PASSWORD); // Set env in CLion CMAKE setting

    const uint led_pin = 22;
    // Initialize LED pin
    gpio_init(led_pin);
    gpio_set_dir(led_pin, GPIO_OUT);

    const uint button = 9;
    gpio_init(button);
    gpio_set_dir(button, GPIO_IN);
    gpio_pull_up(button);

     while(true) {
        //std::cout << "button event\n";
        gpio_put(led_pin, 1);
        vTaskDelay(300);
        gpio_put(led_pin, 0);
        vTaskDelay(300);
        if(gpio_get(button) == 0) {
            int rc = ipstack.connect(HTTP_SERVER, 80);
            if (rc == 0) {
                ipstack.write((unsigned char *) (req), strlen(req), 1000);
                auto rv = ipstack.read(buffer, BUFSIZE, 2000);
                buffer[rv] = 0;
                printf("rv=%d\n%s\n", rv, buffer);
                ipstack.disconnect();
            }
            else {
                printf("rc from TCP connect is %d\n", rc);
            }
        }
    }
}



int main()
{
    stdio_init_all();

    //xTaskCreate(blink_task, "LED_1", 256, (void *) &lp1, tskIDLE_PRIORITY + 1, nullptr);
    //xTaskCreate(gpio_task, "BUTTON", 256, (void *) nullptr, tskIDLE_PRIORITY + 1, nullptr);
    //xTaskCreate(serial_task, "UART1", 256, (void *) nullptr,
    //            tskIDLE_PRIORITY + 1, nullptr);

    xTaskCreate(test_task, "http", 1024, (void *) nullptr,
                tskIDLE_PRIORITY + 1, nullptr);


    vTaskStartScheduler();

    while(true){};
}
