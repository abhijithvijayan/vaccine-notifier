/**
 *  vaccine-notifier-esp8266
 *
 *  @author   abhijithvijayan <abhijithvijayan.in>
 *  @license  MIT License
 *  @title:   Vaccine Notifier for COWIN WebApp
 *
 */
#include <Arduino_JSON.h>    // https://github.com/bblanchon/ArduinoJson
#include <ESP8266WiFi.h>
#include <Ticker.h>

#define PING_INTERVAL 60000 // 1minute

// Network
#define WIFI_SSID "........"
#define WIFI_PASSWORD "........"

// General
#define SERIAL_DEBUG_PORT 115200

WiFiEventHandler wifiConnectHandler;
WiFiEventHandler wifiDisconnectHandler;
Ticker wifiReconnectTimer;

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

long lastExecutionTime = 0;

void loop() {
  long now = millis();

  if (now - lastExecutionTime > PING_INTERVAL) {
      Serial.println("Hello World");
  }

}
