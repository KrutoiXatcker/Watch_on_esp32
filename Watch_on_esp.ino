#include <WiFi.h>
#include <WebServer.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <time.h>

// Настройки Wi-Fi
const char* ssid = "ESP32-AP";
const char* password = "password";

// Настройки OLED-дисплея
#define OLED_WIDTH 128
#define OLED_HEIGHT 64
#define OLED_ADDR 0x3C
#define OLED_SDA 21
#define OLED_SCL 22

Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);

WebServer server(80);

// Переменные времени
int year, month, day, hour, minute, second;
unsigned long lastUpdate = 0;
bool timeSet = false;

void handleRoot() {
  String html = "<html><body>";
  html += "<h1>Set ESP32 Time (YYYY:MM:DD:HH:MM:SS)</h1>";
  html += "<form method='post' action='/update'>";
  html += "<input type='text' name='time' placeholder='2024:05:20:12:30:00'><br>";
  html += "<input type='submit' value='Set Time'>";
  html += "</form>";
  html += "</body></html>";
  server.send(200, "text/html", html);
}

void handleUpdate() {
  if (server.hasArg("time")) {
    String timeStr = server.arg("time");
    int params[6];
    int index = 0;
    int prevIndex = 0;

    // Парсинг строки времени
    for(int i = 0; i < timeStr.length(); i++){
      if(timeStr.charAt(i) == ':'){
        params[index++] = timeStr.substring(prevIndex, i).toInt();
        prevIndex = i+1;
        if(index >= 6) break;
      }
    }
    params[5] = timeStr.substring(prevIndex).toInt();

    if(index == 5){
      year = params[0];
      month = params[1];
      day = params[2];
      hour = params[3];
      minute = params[4];
      second = params[5];
      
      // Установка системного времени
      struct tm tm;
      tm.tm_year = year - 1900;
      tm.tm_mon = month - 1;
      tm.tm_mday = day;
      tm.tm_hour = hour;
      tm.tm_min = minute;
      tm.tm_sec = second;
      time_t t = mktime(&tm);
      struct timeval now = { .tv_sec = t };
      settimeofday(&now, NULL);

      timeSet = true;
      lastUpdate = millis();
      server.send(200, "text/plain", "Time updated");
      updateDisplay();
    } else {
      server.send(400, "text/plain", "Invalid format");
    }
  } else {
    server.send(400, "text/plain", "Missing time parameter");
  }
}

void updateTime() {
  if(!timeSet) return;
  
  unsigned long currentMillis = millis();
  if(currentMillis - lastUpdate >= 1000){
    second++;
    lastUpdate = currentMillis;
    
    if(second >= 60){
      second = 0;
      minute++;
      if(minute >= 60){
        minute = 0;
        hour++;
        if(hour >= 24){
          hour = 0;
          day++;
          // Упрощенная логика для дней в месяце
          int daysInMonth[] = {31,28,31,30,31,30,31,31,30,31,30,31};
          if(day > daysInMonth[month-1]){
            day = 1;
            month++;
            if(month > 12){
              month = 1;
              year++;
            }
          }
        }
      }
    }
    updateDisplay();
  }
}

void updateDisplay() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  
  // Получение текущего времени из системы
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    display.println("Time not set");
    display.display();
    return;
  }

  char timeStr[20];
  strftime(timeStr, sizeof(timeStr), "%Y-%m-%d\n%H:%M:%S", &timeinfo);
  display.println("Time:");
  display.println(timeStr);
  
  display.display();
}

void setup() {
  Serial.begin(115200);

  // Инициализация дисплея
  Wire.begin(OLED_SDA, OLED_SCL);
  if(!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("OLED error");
    while(1);
  }
  display.clearDisplay();
  display.display();

  // Запуск точки доступа
  WiFi.softAP(ssid, password);
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());

  // Роутинг сервера
  server.on("/", HTTP_GET, handleRoot);
  server.on("/update", HTTP_POST, handleUpdate);
  server.begin();

  // Начальное отображение
  updateDisplay();
}

void loop() {
  server.handleClient();
  updateTime();
}