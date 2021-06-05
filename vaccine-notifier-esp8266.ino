/**
 *  vaccine-notifier-esp8266
 *
 *  @author   abhijithvijayan <abhijithvijayan.in>
 *  @license  MIT License
 *  @title:   Vaccine Notifier for COWIN WebApp
 *
 */
#include <Arduino_JSON.h> // https://github.com/bblanchon/ArduinoJson
#include <ESP8266WiFi.h>
#include <NTPClient.h> // https://github.com/arduino-libraries/NTPClient
#include <Ticker.h>
#include <WiFiUdp.h>

#define PING_INTERVAL 10000                                  // 1minute
const long utcOffsetInSeconds = ((5 * 60 * 60) + (30 * 60)); // UTC+5:30

// Network
#define WIFI_SSID "........"
#define WIFI_PASSWORD "........"

// General
#define SERIAL_DEBUG_PORT 115200

WiFiEventHandler wifiConnectHandler;
WiFiEventHandler wifiDisconnectHandler;
Ticker wifiReconnectTimer;

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

long lastExecutionTime = 0;

unsigned long getTime() {
    timeClient.update();

    return timeClient.getEpochTime();
}

String getCurrentDate() {
    unsigned long epochTime = getTime();
    // Get a time structure
    /**
     *  Get a time structure
     *  http://www.cplusplus.com/reference/ctime/tm/
     *
     *  tm_sec: seconds after the minute
     *  tm_min: minutes after the hour
     *  tm_hour: hours since midnight
     *  tm_mday: day of the month
     *  tm_year: years since 1900
     */
    struct tm *ptm = gmtime((time_t *)&epochTime);
    int day        = ptm->tm_mday;
    int month      = ptm->tm_mon + 1;
    int year       = ptm->tm_year + 1900;

    return String(day) + '-' + String(month) + '-' + String(year);
}

/**
 *  Handle WiFi Connectivity
 */
void connectToWifi() {
    Serial.println();
    Serial.print("[WiFi] Connecting to ");
    Serial.println(WIFI_SSID);

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

/**
 *  WiFi Connect event handler
 */
void onWifiConnect(const WiFiEventStationModeGotIP &event) {
    Serial.println("");
    Serial.println("[WiFi] Connected.");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());

    // Get internet time
    timeClient.begin();
}

/**
 *  WiFi Disconnect event handler
 */
void onWifiDisconnect(const WiFiEventStationModeDisconnected &event) {
    Serial.println("[WiFi] Disconnected.");

    wifiReconnectTimer.once(2, connectToWifi);
}

void setup() {
    pinMode(BUILTIN_LED, OUTPUT); // Initialize the BUILTIN_LED pin as an output

    // initialize serial for debugging
    Serial.begin(SERIAL_DEBUG_PORT);

    wifiConnectHandler    = WiFi.onStationModeGotIP(onWifiConnect);
    wifiDisconnectHandler = WiFi.onStationModeDisconnected(onWifiDisconnect);

    // initialize & connect to WiFi
    connectToWifi();
}

void loop() {
    long now = millis();

    if (now - lastExecutionTime > PING_INTERVAL) {
        lastExecutionTime = now;
        Serial.println(getCurrentDate());
    }
}
