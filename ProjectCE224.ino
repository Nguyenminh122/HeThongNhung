#include "Lib.h"
#include "webpage.h"

ESP8266WebServer server(80);

const char* WIFI_SSID = "Redmi K60E";
const char* WIFI_PWD = "123456789";

#define TZ              6       
#define DST_MN          60      

// Setup
const int UPDATE_INTERVAL_SECS = 30 * 60; // Update every 30 minutes

// Display Settings
const int I2C_DISPLAY_ADDRESS = 0x3c;
#if defined(ESP8266)
const int SDA_PIN = D3;
const int SDC_PIN = D4;
#else
const int SDA_PIN = 5; //D3;
const int SDC_PIN = 4; //D4;
#endif

String OPEN_WEATHER_MAP_APP_ID = "7f56beffa2de944ebcadbd3468e22bc8";


String OPEN_WEATHER_MAP_LANGUAGE = "en";
const uint8_t MAX_FORECASTS = 4;

const boolean IS_METRIC = true;

// Adjust according to your language
const String WDAY_NAMES[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
const String MONTH_NAMES[] = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};

 SSD1306Wire     display(I2C_DISPLAY_ADDRESS, SDA_PIN, SDC_PIN);
 OLEDDisplayUi   ui( &display );

OpenWeatherMapCurrentData currentWeather;
OpenWeatherMapCurrent currentWeatherClient;

OpenWeatherMapForecastData forecasts[MAX_FORECASTS];
OpenWeatherMapForecast forecastClient;

#define TZ_MN           ((TZ)*60)
#define TZ_SEC          ((TZ)*3600)
#define DST_SEC         ((DST_MN)*60)
time_t now;

// flag changed in the ticker function every 10 minutes
bool readyForWeatherUpdate = false;

String lastUpdate = "--";

long timeSinceLastWUpdate = 0;

//declaring prototypes
void drawProgress(OLEDDisplay *display, int percentage, String label);
void updateData(OLEDDisplay *display);
void drawDateTime(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
void drawCurrentWeather(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
void drawForecast(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
void drawForecastDetails(OLEDDisplay *display, int x, int y, int dayIndex);
void drawHeaderOverlay(OLEDDisplay *display, OLEDDisplayUiState* state);
void setReadyForWeatherUpdate();


FrameCallback frames[] = { drawDateTime, drawCurrentWeather, drawForecast };
int numberOfFrames = 3;

OverlayCallback overlays[] = { drawHeaderOverlay };
int numberOfOverlays = 1;

void handleRoot() {
    // Lấy thời gian hiện tại
    time_t now = time(nullptr);
    struct tm* timeInfo = localtime(&now);

    char timeString[30];
    strftime(timeString, sizeof(timeString), "%Y-%m-%d %H:%M:%S", timeInfo);
    String currentTime = String(timeString);

    String temperature = String(currentWeather.temp, 1) + (IS_METRIC ? "°C" : "°F");
    String currentLocation = locationNames[currentLocationIndex];

    String locationOptions = "";
    for (int i = 0; i < sizeof(locations) / sizeof(locations[0]); i++) {
        locationOptions += "<option value='" + String(i) + "'";
        if (i == currentLocationIndex) {
            locationOptions += " selected";
        }
        locationOptions += ">" + locationNames[i] + "</option>";
    }

    // Tạo bản sao của PAGE_HTML và thay thế các placeholders
    String htmlContent = PAGE_HTML;
    htmlContent.replace("{{current_time}}", currentTime);
    htmlContent.replace("{{temperature}}", temperature);
    htmlContent.replace("{{current_location}}", currentLocation);
    htmlContent.replace("{{location_options}}", locationOptions);  // Thay thế với các tùy chọn địa điểm

    server.send(200, "text/html", htmlContent);
}

void handleSetLocation() {
    if (server.hasArg("location")) {
        int newLocationIndex = server.arg("location").toInt();

        // Kiểm tra chỉ số location hợp lệ
        if (newLocationIndex >= 0 && newLocationIndex < (sizeof(locations) / sizeof(locations[0]))) {
            currentLocationIndex = newLocationIndex;
            OPEN_WEATHER_MAP_LOCATION_ID = locations[currentLocationIndex];
            Serial.println("Location updated to: " + locationNames[currentLocationIndex]);

            // Lưu vị trí vào EEPROM
            EEPROM.write(LOCATION_INDEX_ADDR, currentLocationIndex);
            EEPROM.commit();  // Ghi dữ liệu vào bộ nhớ
        } else {
            Serial.println("Invalid location index. Keeping current location.");
        }
    }

    // Thông báo cập nhật và reset
    delay(1000);

    ESP.restart();  // Reset ESP để cập nhật vị trí
}


void setup() {
  Serial.begin(115200);

  EEPROM.begin(EEPROM_SIZE);  // Khởi tạo EEPROM

  Serial.println();
  Serial.println();

  int storedLocationIndex = EEPROM.read(LOCATION_INDEX_ADDR);


if (storedLocationIndex >= 0 && storedLocationIndex < (sizeof(locations) / sizeof(locations[0]))) {
        currentLocationIndex = storedLocationIndex;
        OPEN_WEATHER_MAP_LOCATION_ID = locations[currentLocationIndex];
        Serial.println("Loaded location from EEPROM: " + locationNames[currentLocationIndex]);
    } else {
        // Nếu giá trị không hợp lệ, đặt về mặc định
        currentLocationIndex = 0;  // Hà Nội là vị trí mặc định
        OPEN_WEATHER_MAP_LOCATION_ID = locations[currentLocationIndex];
        Serial.println("Invalid location in EEPROM. Defaulting to: " + locationNames[currentLocationIndex]);
    }
  // initialize dispaly
  display.init();
  display.clear();
  display.display();

  //display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.setContrast(255);

  WiFi.begin(WIFI_SSID, WIFI_PWD);
  Serial.print("Connecting to WiFi...");

  int counter = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    display.clear();
    display.drawString(64, 10, "Connecting to WiFi");
    display.drawXbm(46, 30, 8, 8, counter % 3 == 0 ? activeSymbole : inactiveSymbole);
    display.drawXbm(60, 30, 8, 8, counter % 3 == 1 ? activeSymbole : inactiveSymbole);
    display.drawXbm(74, 30, 8, 8, counter % 3 == 2 ? activeSymbole : inactiveSymbole);
    display.display();

    counter++;
  }
  Serial.println("Connected to WiFi.");
  Serial.println("ESP8266 IP address: ");
  Serial.println(WiFi.localIP());
   Serial.println(WiFi.localIP());
    server.on("/", handleRoot);
    server.on("/setLocation", HTTP_POST, handleSetLocation);
    server.begin();
    Serial.println("Web server started");
  // Get time from network time service
  configTime(TZ_SEC, DST_SEC, "pool.ntp.org");

  ui.setTargetFPS(30);

  ui.setActiveSymbol(activeSymbole);
  ui.setInactiveSymbol(inactiveSymbole);

  // You can change this to
  // TOP, LEFT, BOTTOM, RIGHT
  ui.setIndicatorPosition(BOTTOM);

  // Defines where the first frame is located in the bar.
  ui.setIndicatorDirection(LEFT_RIGHT);

  // You can change the transition that is used
  // SLIDE_LEFT, SLIDE_RIGHT, SLIDE_TOP, SLIDE_DOWN
  ui.setFrameAnimation(SLIDE_LEFT);

  ui.setFrames(frames, numberOfFrames);

  ui.setOverlays(overlays, numberOfOverlays);

  // Inital UI takes care of initalising the display too.
  ui.init();

  Serial.println("");

  updateData(&display);

}

void loop() {

  if (millis() - timeSinceLastWUpdate > (1000L*UPDATE_INTERVAL_SECS)) {
    setReadyForWeatherUpdate();
    timeSinceLastWUpdate = millis();
  }
  if (readyForWeatherUpdate && ui.getUiState()->frameState == FIXED) {
    updateData(&display);
    readyForWeatherUpdate = false;
  }

  int remainingTimeBudget = ui.update();

  if (remainingTimeBudget > 0) {
    // You can do some work here
    // Don't do stuff if you are below your
    // time budget.
    delay(remainingTimeBudget);
  }
  server.handleClient();

}

void drawProgress(OLEDDisplay *display, int percentage, String label) {
  display->clear();
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->setFont(ArialMT_Plain_10);
  display->drawString(64, 10, label);
  display->drawProgressBar(2, 28, 124, 10, percentage);
  display->display();
}

void updateData(OLEDDisplay *display) {
   drawProgress(display, 10, "Updating time...");
   
   // Cập nhật thông tin thời tiết
   drawProgress(display, 30, "Updating weather...");
   currentWeatherClient.setMetric(IS_METRIC);
   currentWeatherClient.setLanguage(OPEN_WEATHER_MAP_LANGUAGE);
   currentWeatherClient.updateCurrentById(&currentWeather, OPEN_WEATHER_MAP_APP_ID, OPEN_WEATHER_MAP_LOCATION_ID);
   
   drawProgress(display, 50, "Updating forecasts...");
   forecastClient.setMetric(IS_METRIC);
   forecastClient.setLanguage(OPEN_WEATHER_MAP_LANGUAGE);
   uint8_t allowedHours[] = {12};
   forecastClient.setAllowedHours(allowedHours, sizeof(allowedHours));
   forecastClient.updateForecastsById(forecasts, OPEN_WEATHER_MAP_APP_ID, OPEN_WEATHER_MAP_LOCATION_ID, MAX_FORECASTS);

   readyForWeatherUpdate = false;
   
   // Hiển thị thông tin thời tiết trên màn hình OLED
   display->clear();
   
    //delay(3000);   
   display->display();
   
   // Hoàn thành quá trình cập nhật
   drawProgress(display, 100, "Done...");
   delay(1000);
}



void drawDateTime(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  now = time(nullptr);
  struct tm* timeInfo;
  timeInfo = localtime(&now);
  char buff[16];
  display->drawString(90, 38,locationNames[currentLocationIndex]);

  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->setFont(ArialMT_Plain_10);
  String date = WDAY_NAMES[timeInfo->tm_wday];

  sprintf_P(buff, PSTR("%s, %02d/%02d/%04d"), WDAY_NAMES[timeInfo->tm_wday].c_str(), timeInfo->tm_mday, timeInfo->tm_mon+1, timeInfo->tm_year + 1900);
  display->drawString(64 + x, 5 + y, String(buff));
  display->setFont(ArialMT_Plain_24);

  sprintf_P(buff, PSTR("%02d:%02d:%02d"), timeInfo->tm_hour, timeInfo->tm_min, timeInfo->tm_sec);
  display->drawString(64 + x, 15 + y, String(buff));
  display->setTextAlignment(TEXT_ALIGN_LEFT);
}

void drawCurrentWeather(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  display->setFont(ArialMT_Plain_10);
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->drawString(64 + x, 38 + y, currentWeather.description);

  display->setFont(ArialMT_Plain_24);
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  String temp = String(currentWeather.temp, 1) + (IS_METRIC ? "°C" : "°F");
  display->drawString(60 + x, 5 + y, temp);

  display->setFont(Meteocons_Plain_36);
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->drawString(32 + x, 0 + y, currentWeather.iconMeteoCon);
}


void drawForecast(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  drawForecastDetails(display, x, y, 0);
  drawForecastDetails(display, x + 44, y, 1);
  drawForecastDetails(display, x + 88, y, 2);
}

void drawForecastDetails(OLEDDisplay *display, int x, int y, int dayIndex) {
  time_t observationTimestamp = forecasts[dayIndex].observationTime;
  struct tm* timeInfo;
  timeInfo = localtime(&observationTimestamp);
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->setFont(ArialMT_Plain_10);
  display->drawString(x + 20, y, WDAY_NAMES[timeInfo->tm_wday]);

  display->setFont(Meteocons_Plain_21);
  display->drawString(x + 20, y + 12, forecasts[dayIndex].iconMeteoCon);
  String temp = String(forecasts[dayIndex].temp, 0) + (IS_METRIC ? "°C" : "°F");
  display->setFont(ArialMT_Plain_10);
  display->drawString(x + 20, y + 34, temp);
  display->setTextAlignment(TEXT_ALIGN_LEFT);
}

void drawHeaderOverlay(OLEDDisplay *display, OLEDDisplayUiState* state) {
  now = time(nullptr);
  struct tm* timeInfo;
  timeInfo = localtime(&now);
  char buff[14];
  sprintf_P(buff, PSTR("%02d:%02d"), timeInfo->tm_hour, timeInfo->tm_min);

  display->setColor(WHITE);
  display->setFont(ArialMT_Plain_10);
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->drawString(0, 54, String(buff));
  display->setTextAlignment(TEXT_ALIGN_RIGHT);
  String temp = String(currentWeather.temp, 1) + (IS_METRIC ? "°C" : "°F");
  display->drawString(128, 54, temp);
  display->drawHorizontalLine(0, 52, 128);
}

void setReadyForWeatherUpdate() {
  Serial.println("Setting readyForUpdate to true");
  readyForWeatherUpdate = true;
}
