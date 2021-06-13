# vaccine-notifier

## ESP8266

#### Fetch the certificate store from Mozilla and upload it to your ESP’s SPIFFS

> <https://raw.githubusercontent.com/esp8266/Arduino/master/libraries/ESP8266WiFi/examples/BearSSL_CertStore/certs-from-mozilla.py>
> <https://www.instructables.com/Using-ESP8266-SPIFFS/>

### Tools

> <https://github.com/earlephilhower/arduino-esp8266littlefs-plugin/releases>
<!-- > <https://github.com/esp8266/arduino-esp8266fs-plugin/releases> -->
> <https://github.com/earlephilhower/arduino-esp8266littlefs-plugin/issues/15>

- Unzip the downloaded `.zip` folder to the `Arduino/tools` folder.
- Restart your Arduino IDE
- In the Tools menu, check that you have the option `ESP8266 LittleFS Data Upload`

## Micro Service CRON

> Checkout [readme.md](https://github.com/abhijithvijayan/vaccine-notifier/blob/main/micro/readme.md) in [micro/](https://github.com/abhijithvijayan/vaccine-notifier/tree/main/micro) folder

## LAMBDA Functions

> Checkout [readme.md](https://github.com/abhijithvijayan/vaccine-notifier/blob/main/lambda/readme.md) in [lambda/](https://github.com/abhijithvijayan/vaccine-notifier/tree/main/lambda) folder
