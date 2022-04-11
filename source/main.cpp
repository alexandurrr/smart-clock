#include "mbed.h"
#include <algorithm>
#include <cstdio>
#include "DFRobot_RGBLCD.h"
#define JSON_NOEXCEPTION
#include "json.hpp"
#include <chrono>
#include "cert.h"
#include "wifi.h"
#include "HTS221Sensor.h"
#include <stdlib.h>     /* strtol */
#include <string>
#include <iostream>
static char buffer[2048];

using json = nlohmann::json;
DFRobot_RGBLCD lcd(16, 2, D14, D15);

float temp, humid;
// Buttons and their variables
InterruptIn change_screen(A5, PullUp);
InterruptIn Button_add_1_hour(A0, PullUp);
InterruptIn Button_add_1_min(A3, PullUp);
InterruptIn Button_stop_alarm(A2, PullUp);
InterruptIn Button_snooze_alarm(A1, PullUp);
// Sensor
DevI2C i2c(PB_11, PB_10);
HTS221Sensor sensor(&i2c);
// Buzzer
PwmOut piezo(D11);


int setHour = 0;
int setMin = 0;
int setSec = 0;
volatile int select_state = 0;
volatile int saved_hour = 0;
volatile int saved_min = 0;


void Select()
{
    select_state += 1;
    if (select_state==4){
        select_state = 0;
    }
}

void add_1_hour() {
    setHour += 1;
    if(setHour == 24){
        setHour = 0;
    }
}

void add_1_min() {
    setMin += 1;
    if(setMin == 60){
        setMin = 0;
    }
}

void stop_alarm(){
    setHour = 0;
    setMin = 0;
    piezo = 0.0;
}

void snooze_alarm(){
if(piezo>0.0){
piezo = 0.0;
setMin = setMin + 5;
if (setMin>=60) {
    setMin -= 60;
    setHour += 1;
}
if (setHour>=24) {
    setHour -= 24;
}
}
else if ((setHour>0 || setMin>0) && piezo==0.0) {
saved_hour = setHour;
saved_min = setMin;
setMin = 0;
setHour = 0;
}
else if (setHour==0 && setMin==0 && piezo==0.0) {
setHour = saved_hour;
setMin = saved_min;
}
}


int main() {

    // Get pointer to default network interface
    NetworkInterface *network = NetworkInterface::get_default_instance();

    if (!network) {
        printf("Failed to get default network interface\n");
        while (1);
    }

    // Connect to network
    printf("Connecting to the network...\n");
    nsapi_size_or_error_t result = network->connect();

    // Check if the connection is successful
    if (result != 0) {
        printf("Failed to connect to network: %d\n", result);
        while (1);
    }

    printf("Connection to network successful!\n");

    // Create and allocate resources for socket.
    // TLSSocket is used for HTTPS/SSL
    TLSSocket *socket = new TLSSocket();

    // Set the root certificate of the web site.
    // See include/cert.h for how to download the cert.
    result = socket->set_root_ca_cert(cert);
    if (result != 0) {
        printf("Error: socket->set_root_ca_cert() returned %d\n", result);
        return result;
    }

    socket->open(network);

    // Create destination address
    SocketAddress address;

    // Get IP address of host by name
    result = network->gethostbyname("timezoneapi.io", &address);

    // Check result
    if (result != 0) {
        printf("Failed to get IP address of host: %d\n", result);
        while (1);
    }

    printf("Got address of host\n");

    // Set server port number, 443 for HTTPS/SSL
    address.set_port(443);

    // Connect to server at the given address
    result = socket->connect(address);

    // Check result
    if (result != 0) {
        printf("Failed to connect to server: %d\n", result);
        while (1);
    }

    printf("Connection to server successful!\n");

    // Create HTTP request
    char request[] =    "GET /api/timezone/?Europe/Paris&token=amuiEtPKIzdoMmwIlmzE HTTP/1.1\r\n"
                        "Host: timezoneapi.io\r\n"
                        "Connection: close\r\n"
                        "\r\n";

    // Send request
    result = send_request(socket, request);

    // Check result
    if (result != 0) {
        printf("Failed to send request: %d\n", result);
        while (1);
    }

    // We need to read the response into memory. The destination is called a buffer.
    // If you make this buffer static it will be placed in BSS and won't use stack memory.
    static char buffer[8192];
    result = read_response(socket, buffer, sizeof(buffer));

    // Check result
    if (result != 0) {
        printf("Failed to read response: %d\n", result);
        
        while (1);
    }


    socket->close();
   delete socket;

    // Find the start and end of the JSON data.
    // If the json response is an array you need to replace this with [ and ]
    char* json_begin = strchr(buffer, '{');
    char* json_end = strrchr(buffer, '}');

    // Check if we actually got JSON in the response
    if (json_begin == nullptr || json_end == nullptr) {
        printf("Failed to find JSON in response\n");
        while (1);
    }

    // End the string after the end of the JSON data in case the response contains trailing data
    json_end[1] = 0;

    // Parse response as JSON, starting from the first {
    json document = json::parse(json_begin);

    // Get IP address from JSON object
    std::string hour_24_wolz;
    document["data"]["datetime"]["hour_24_wolz"].get_to(hour_24_wolz);

    std::string minutes;
    document["data"]["datetime"]["minutes"].get_to(minutes);

    std::string day_abbr;
    document["data"]["datetime"]["day_abbr"].get_to(day_abbr);

    std::string day;
    document["data"]["datetime"]["day"].get_to(day);
    
    std::string month_full;
    document["data"]["datetime"]["month_full"].get_to(month_full);

    std::string year;
    document["data"]["datetime"]["year"].get_to(year);

        std::string seconds;
    document["data"]["datetime"]["seconds"].get_to(seconds);

    int m = std::stoi(document["data"]["datetime"]["minutes"].get_ref<std::string&>());
    
    int h = std::stoi(document["data"]["datetime"]["hour_24_wolz"].get_ref<std::string&>());

    int s = std::stoi(document["data"]["datetime"]["seconds"].get_ref<std::string&>());

    int min = m;
    int hour = h;
    int sec = s;

    lcd.init();
    sensor.init(nullptr);
    sensor.enable();

    change_screen.mode(PullUp);
    
    network->disconnect();

    change_screen.rise(&Select);


    Button_add_1_hour.rise(&add_1_hour);
    Button_add_1_min.rise(&add_1_min);   
    Button_stop_alarm.rise(&stop_alarm);
    Button_snooze_alarm.rise(&snooze_alarm);

    while (true) 
    {

        if ((hour == setHour) && (min == setMin) && (hour != 0) && (min != 0))  {
        piezo.period(1.0/50.0);
        piezo=0.5;
        wait_us(1000000);
        piezo.period(1.0/100.0);
        piezo=0.5;
        wait_us(1000000);
        
    }
        

    if (select_state==0)
	{   

        sec += 1;
        if (sec == 60)
        {
        sec -= 60;
        min += 1;
        }
        if (min == 60)
        {
        min -= 60;
        hour += 1;
        }
        if (hour == 24)
        {
        hour -=24;
        }
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.printf("%02d:%02d:%02d", hour, min, sec);
        lcd.setCursor(0, 1);
        lcd.printf("%s:%s/%s/%s", day_abbr.c_str(), day.c_str(), month_full.c_str(), year.c_str());
        ThisThread::sleep_for(1s);
    } 
	if (select_state==1) 
	{

        sec += 1;
        if (sec == 60)
        {
        sec -= 60;
        min += 1;
        }
        if (min == 60)
        {
        min -= 60;
        hour += 1;
        }
        if (hour == 24)
        {
        hour -=24;
        }        
        sensor.get_temperature(&temp);
        sensor.get_humidity(&humid);
        lcd.clear();
        lcd.setCursor(0,1);
        lcd.printf("Temp: %.2f C", temp);
        lcd.setCursor(1,0);
        lcd.printf("Hum: %.2f%%", humid);
        ThisThread::sleep_for(1s);
        } 
	if (select_state==2)
	    {
        sec += 1;
        if (sec == 60)
        {
        sec -= 60;
        min += 1;
        }
        if (min == 60)
        {
        min -= 60;
        hour += 1;
        }
        if (hour == 24)
        {
        hour -=24;
        }         
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.printf("Set Alarm:");
        lcd.setCursor(0,1);
        lcd.printf("%02d:%02d", setHour, setMin);   
        ThisThread::sleep_for(1s);
        }
    if (select_state==3) 
	    {
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.printf("Weather screen");
        lcd.setCursor(0,1);
        lcd.printf("test text");
        ThisThread::sleep_for(1s);
        }
    }

}