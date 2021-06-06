/**
 *  vaccine-notifier-esp8266
 *
 *  @author   abhijithvijayan <abhijithvijayan.in>
 *  @license  MIT License
 *  @title:   Vaccine Notifier for COWIN WebApp
 *
 */
#include <Arduino_JSON.h> // https://github.com/bblanchon/ArduinoJson
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <NTPClient.h> // https://github.com/arduino-libraries/NTPClient
#include <Ticker.h>
#include <WiFiUdp.h>

// Network
#define WIFI_SSID ""
#define WIFI_PASSWORD ""

#define TELEGRAM_CHAT_ID 12345
#define TELEGRAM_BOT_TOKEN ""

// General
#define SERIAL_DEBUG_PORT 115200

#define PING_INTERVAL 10000                                  // 1minute
const long utcOffsetInSeconds = ((5 * 60 * 60) + (30 * 60)); // UTC+5:30

WiFiEventHandler wifiConnectHandler;
WiFiEventHandler wifiDisconnectHandler;
Ticker wifiReconnectTimer;

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "in.pool.ntp.org", utcOffsetInSeconds);

long lastExecutionTime = 0;

String httpGETRequest(String requestURL) {
    Serial.println(String(requestURL));
    std::unique_ptr<BearSSL::WiFiClientSecure> client(
        new BearSSL::WiFiClientSecure);
    client->setInsecure();

    HTTPClient https;
    String payload = "{}";

    Serial.print("[HTTPS] begin...\n");
    if (https.begin(*client, requestURL)) {
        Serial.print("[HTTPS] GET...\n");
        // start connection and send HTTP header
        int httpCode = https.GET();

        // httpCode will be negative on error
        if (httpCode > 0) {
            // HTTP header has been send and Server response header has been
            // handled
            Serial.printf("[HTTPS] GET... code: %d\n", httpCode);

            // file found at server
            if (httpCode == HTTP_CODE_OK ||
                httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
                payload = https.getString();
                Serial.println(payload);
            }
        } else {
            Serial.printf("[HTTPS] GET... failed, error: %s\n",
                https.errorToString(httpCode).c_str());
        }
    }

    // Free resources
    https.end();

    return payload;
}

void httpPOSTRequest(String requestURL) {
    Serial.println(String(requestURL));

    std::unique_ptr<BearSSL::WiFiClientSecure> client(
        new BearSSL::WiFiClientSecure);
    client->setInsecure();

    HTTPClient https;
    String payload = "{}";

    Serial.print("[HTTPS] begin...\n");
    if (https.begin(*client, requestURL)) {
        https.addHeader("Content-Type", "application/json");

        Serial.print("[HTTPS] POST...\n");
        // start connection and send HTTP header
        int httpCode =
            https.POST(String("{\"chat_id\":\"") + String(TELEGRAM_CHAT_ID) +
                       String("\",\"text\":\"hello world\"}"));

        // httpCode will be negative on error
        if (httpCode > 0) {
            // HTTP header has been send and Server response header has been
            // handled
            Serial.printf("[HTTPS] POST... code: %d\n", httpCode);

            // file found at server
            if (httpCode == HTTP_CODE_OK ||
                httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
                payload = https.getString();
                Serial.println(payload);
            }
        } else {
            Serial.printf("[HTTPS] POST... failed, error: %s\n",
                https.errorToString(httpCode).c_str());
        }
    }

    // Free resources
    https.end();
}

String getCurrentDate() {
    unsigned long epochTime = timeClient.getEpochTime();
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
    int month      = ptm->tm_mon + 1; // tm_mon is month from 0..11
    int year       = ptm->tm_year + 1900;

    Serial.print("Epoch Time: ");
    Serial.println(epochTime);

    String formattedTime = timeClient.getFormattedTime();
    Serial.print("Formatted Time: ");
    Serial.println(formattedTime);

    Serial.print("Current day: ");
    Serial.println(day);
    Serial.print("Current month: ");
    Serial.println(month);
    Serial.print("Current year: ");
    Serial.println(year);

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
        timeClient.update();

        lastExecutionTime = now;
        String today      = getCurrentDate();
        Serial.println(today);
    }
}
