#include <Arduino.h>

#if defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <coredecls.h>                  
#else
#include <WiFi.h>
#endif
#include <ESPHTTPClient.h>
#include <JsonListener.h>

// time
#include <time.h>                       
#include <sys/time.h>                   

#include "SSD1306Wire.h"
#include "OLEDDisplayUi.h"
#include "Wire.h"
#include "OpenWeatherMapCurrent.h"
#include "OpenWeatherMapForecast.h"
#include "WeatherStationFonts.h"
#include "WeatherStationImages.h"
#include <EEPROM.h>

#define EEPROM_SIZE 1  // Chỉ cần 1 byte để lưu chỉ số vị trí
#define LOCATION_INDEX_ADDR 0  // Địa chỉ trong EEPROM
