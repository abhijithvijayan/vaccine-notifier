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
#include <time.h>

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

void setupLocalTime() {
    // see https://github.com/esp8266/Arduino/issues/4637
    time_t now;
    now = time(nullptr); // if there's no time, this will have a value of 28800;
                         // Thu Jan  1 08:00:00 1970
    Serial.print("Initial time:");
    Serial.println(now);
    Serial.println(ctime(&now));

    int myTimezone     = +5.5;
    int dst            = 0;
    int SecondsPerHour = 3600;
    int MAX_TIME_RETRY = 60;
    int i              = 0;

    // it is unlikely that the time is already set since we have no battery;
    // if no time is avalable, then try to set time from the network
    if (now <= 1500000000) {
        // try to set network time via ntp packets
        configTime(0, 0, "0.in.pool.ntp.org",
            "pool.ntp.org"); // see
                             // https://github.com/esp8266/Arduino/issues/4749#issuecomment-390822737

        // Starting in 2007, most of the United States and Canada observe DST
        // from the second Sunday in March to the first Sunday in November.
        // example setting Pacific Time:
        setenv("TZ", "PST8PDT,M3.2.0/02:00:00,M11.1.0/02:00:00",
            1); // see
                // https://users.pja.edu.pl/~jms/qnx/help/watcom/clibref/global_data.html
        //                     | month 3, second sunday at 2:00AM
        //                                    | Month 11 - firsst Sunday, at
        //                                    2:00am
        // Mm.n.d
        //   The dth day(0 <= d <= 6) of week n of month m of the year(1 <= n <=
        //   5, 1 <= m <= 12, where
        //     week 5 means "the last d day in month m", which may occur in the
        //     fourth or fifth week). Week 1 is the first week in which the dth
        //     day occurs.Day zero is Sunday.

        tzset();
        Serial.print("Waiting for time(nullptr).");
        i = 0;
        while (!time(nullptr)) {
            Serial.print(".");
            delay(1000);
            i++;
            if (i > MAX_TIME_RETRY) {
                Serial.println(
                    "Gave up waiting for time(nullptr) to have a valid value.");
                break;
            }
        }
    }
    Serial.println("");

    // wait and determine if we have a valid time from the network.
    now = time(nullptr);
    i   = 0;
    Serial.print("Waiting for network time.");
    while (now <= 1500000000) {
        Serial.print(".");
        delay(1000); // allow a few seconds to connect to network time.
        i++;
        now = time(nullptr);
        if (i > MAX_TIME_RETRY) {
            Serial.println("Gave up waiting for network time(nullptr) to have "
                           "a valid value.");
            break;
        }
    }
    Serial.println("");

    // get the time from the system
    char *tzvalue;
    tzvalue = getenv("TZ");
    Serial.print("Network time:");
    Serial.println(now);
    Serial.println(ctime(&now));
    Serial.print("tzvalue for timezone = ");
    Serial.println(tzvalue);

    // TODO - implement a web service that returns current epoch time to use
    // when NTP unavailable (insecure SSL due to cert date validation)

    // some networks may not allow ntp protocol (e.g. guest networks) so we may
    // need to fudge the time
    if (now <= 1500000000) {
        Serial.println(
            "Unable to get network time. Setting to fixed value. \n");
        // set to RTC text value
        // see https://www.systutorials.com/docs/linux/man/2-settimeofday/
        //
        // struct timeval {
        //	time_t      tv_sec;     /* seconds */
        //	suseconds_t tv_usec;    /* microseconds */
        //};
        timeval tv = {5, 0};
        //
        // struct timezone {
        //	int tz_minuteswest;     /* minutes west of Greenwich */
        //	int tz_dsttime;         /* type of DST correction */
        //};
        timezone tz = {myTimezone * 60, 0};

        // int settimeofday(const struct timeval *tv, const struct timezone
        // *tz);
        settimeofday(&tv, &tz);
    }

    now = time(nullptr);
    Serial.println("Final time:");
    Serial.println(now);
    Serial.println(ctime(&now));
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
    setupLocalTime();

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
