/**
 *  vaccine-notifier-esp8266
 *
 *  @author   abhijithvijayan <abhijithvijayan.in>
 *  @license  MIT License
 *  @title:   Vaccine Notifier for COWIN WebApp
 *
 */
#include <CertStoreBearSSL.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <FS.h>
#include <LittleFS.h>
#include <NTPClient.h> // https://github.com/arduino-libraries/NTPClient
#include <Ticker.h>
#include <WiFiClientSecureBearSSL.h>
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

// A single, global CertStore which can be used by all
// connections.  Needs to stay live the entire time any of
// the WiFiClientBearSSLs are present.
BearSSL::CertStore certStore;

WiFiEventHandler wifiConnectHandler;
WiFiEventHandler wifiDisconnectHandler;
Ticker wifiReconnectTimer;

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "in.pool.ntp.org", utcOffsetInSeconds);

long lastExecutionTime = 0;

String httpGETRequest(String requestURL) {
    std::unique_ptr<BearSSL::WiFiClientSecure> client(
        new BearSSL::WiFiClientSecure);
    // client->setInsecure();
    // Integrate the cert store with this connection
    client->setCertStore(&certStore);

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
    String requestURL =
        "https://nj9ky893a2.execute-api.ap-south-1.amazonaws.com/api/v1/"
        "current_date";

    return httpGETRequest(requestURL);
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

    // Set time via NTP, as required for x.509 validation
    timeClient.begin();

    int numCerts = certStore.initCertStore(
        LittleFS, PSTR("/certs.idx"), PSTR("/certs.ar"));
    Serial.printf("Number of CA certs read: %d\n", numCerts);

    if (numCerts == 0) {
        Serial.printf("No certs found. Did you run certs.py and "
                      "upload the LittleFS directory before running?\n");
        return; // Can't connect to anything w/o certs!
    }
}

/**
 *  WiFi Disconnect event handler
 */
void onWifiDisconnect(const WiFiEventStationModeDisconnected &event) {
    Serial.println("[WiFi] Disconnected.");

    wifiReconnectTimer.once(2, connectToWifi);
}

void setup() {
    pinMode(LED_BUILTIN, OUTPUT); // Initialize the LED_BUILTIN pin as an output

    // initialize serial for debugging
    Serial.begin(SERIAL_DEBUG_PORT);

    wifiConnectHandler    = WiFi.onStationModeGotIP(onWifiConnect);
    wifiDisconnectHandler = WiFi.onStationModeDisconnected(onWifiDisconnect);

    LittleFS.begin();

    // initialize & connect to WiFi
    connectToWifi();
}

void loop() {
    long now = millis();
    if (now - lastExecutionTime > PING_INTERVAL) {
        lastExecutionTime = now;
        timeClient.update();

        String today = getCurrentDate();
        Serial.println(today);
   }
}
