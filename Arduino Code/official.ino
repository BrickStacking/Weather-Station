#include <SPI.h>            // f.k. for Arduino-1.5.2
#include <SD.h>             // Use the official SD library on hardware pins
#include <Adafruit_GFX.h>   // Hardware-specific library
#include <MCUFRIEND_kbv.h>
MCUFRIEND_kbv tft;
#if defined(ESP8266)
#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
#else
#include <WiFi.h>          //https://github.com/esp8266/Arduino
#endif

//needed for library
#include <DNSServer.h>
#if defined(ESP8266)
#include <ESP8266WebServer.h>
#else
#include <WebServer.h>
#endif
#include <WiFiManager.h>
#include <ArduinoJson.h>
#include <SPI.h>
#include <time.h>
#include <Ticker.h>
#include "Fonts/FreeSans9pt7b.h"    // when you want other fonts
#include "Fonts/FreeSans12pt7b.h" // when you want other fonts
#include "Fonts/FreeSerif12pt7b.h" // when you want other fonts
#include "Fonts/FreeSerifBold18pt7b.h" // when you want other fonts
#include "FreeDefaultFonts.h" // when you want other fonts FreeMonoBoldOblique9pt7b  font_TimesNewRomanBold
#include "Fonts/FreeSansBold9pt7b.h"
#define BLACK   0x0000
#define BLUE    0x001F
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF
#define GREY    0x8410
#define ORANGE  0xE880

#if defined(ESP32)
#define SD_CS     21
#else
#define SD_CS     53
#endif
#define NAMEMATCH ""        // "" Nếu không để gì thì mọi trường hợp đều thoả mãn
//#define NAMEMATCH "tiger"   // *tiger*.bmp
#define PALETTEDEPTH   0     // do not support Palette modes
//#define PALETTEDEPTH   8     // support 256-colour Palette

////////////////Declaration//////////////
uint8_t ret;
char namebuf[100] = "/";   //BMP files in root directory
//char namebuf[32] = "/bitmaps/";  //BMP directory e.g. files in /bitmaps/*.bmp
int timezone = 7;
String  line, thu, ngay, thang, nam, gio, phut, giay, thang_num;
int gio_int, phut_int, giay_int, thu_int, ngay_int, thang_int, nam_int;
int i_phut, i_giay;
void get_time();
void ngat1();
String key  = "ca66835ddbaa496c9d11aee5f48fd28e";  //7903422710e84566a1da194aff33611a  //ca66835ddbaa496c9d11aee5f48fd28e
String http = "GET http://api.weatherbit.io/v2.0/current?city=hanoi,VN&key=";
String http2 = "GET http://api.weatherbit.io/v2.0/forecast/daily?city=hanoi,VN&key=";
String suffix = " HTTP/1.1";
String all = "GET http://api.weatherbit.io/v2.0/current?city=hanoi,VN&key=ca66835ddbaa496c9d11aee5f48fd28e HTTP/1.1";
uint16_t code_daily, code_daily1, temp_hour3, code_daily3;
uint8_t r_humi1, r_humi2, r_humi3, temp_daily1,  temp_daily2 , temp_daily3, temp_hour1, temp_hour2, temp_hour4;
int cloud_daily, temp_daily, icon_transfer, r_humi;
float wind_speed_daily, code_daily2;
char icon_daily, state_icon_daily, description_daily;  // Với icon_daily là chuỗi nhận được, state_icon_daily là trạng thái ngày đê, icon_tranfer là chuyển về dạng decimal để gửi UART
//Char altribute
char temp_111[10], cloud_111[10], wind_111[10];
char temp_daily111[10], temp_daily222[10], temp_daily333[10], r_humi111[10], r_humi222[10], r_humi333[10]; //To display
char all_day_data1[100];
String all_day_data11, all_time11; //Sẽ chuyển tất cả về String, sau đó đẩy về char
char code_display[30], code_display_day1[30], code_display_day2[30], code_display_day3[30];
char state_weather_daily[40], state_weather_day1[40], state_weather_day2[40], state_weather_day3[40], time_now[10];
char  gio_int11[5], phut_int11[5], giay_int11[5], all_time1[15];
char *heat = "/heat.bmp";
char *humi = "/humi.bmp";
char *wind = "/wind.bmp";
unsigned long int time111, time222;
int sence = 0, last = 0; //1 cái là cảnh , 1 cái là lần chiếu
Ticker blinker, daily_get, future_get;
WiFiClient client;
File root;
int pathlen;

void setup()
{
  uint16_t ID;
  Serial.begin(115200);
  Serial.print("Show BMP files on TFT with ID:0x");
  pinMode(2, OUTPUT);
  digitalWrite(2, HIGH);
  ID = tft.readID();
  Serial.println(ID, HEX);
  if (ID == 0x0D3D3) ID = 0x9481;
  tft.begin(ID);
  tft.setRotation(3);
  tft.setTextColor(0xFFFF, 0x0000);
  bool good = SD.begin(SD_CS);
  if (!good) {
    Serial.print(F("cannot start SD"));
    while (1);
  }
  root = SD.open(namebuf);
  pathlen = strlen(namebuf);
  strcpy(namebuf, "/load4.bmp");
  ret = showBMP(namebuf, 0, 0);
  WiFiManager wifiManager;
  wifiManager.autoConnect("Config Wifi");
  if (!wifiManager.autoConnect()) {
    Serial.println("failed to connect and hit timeout");
    //reset and try again, or maybe put it to deep sleep
    ESP.restart();
    delay(1000);
  }
  client.setTimeout(10000);
  configTime(timezone * 3600, 0, "pool.ntp.org", "time.nist.gov");
  get_time();
  get_current();
  get_3days();
  get_hours();
  blinker.attach(1 , count_time); //Ticker for counting time
  daily_get.attach(3600 * 4 , get_current);
  future_get.attach(3600 * 4 , get_3days);
  time111 = millis();
}

//////////////////////////////////////////////////////////////////////
void loop()
{
  if (nam.toInt() < 2000) {
    get_time();
    delay(200);
    Serial.println("Re_get");
  }
  else {
    if (millis() - time111 > 25000 && sence == 0) {
      sence = 1;
      last = 0;
    }
    if (millis() - time111 > 50000 && sence == 1) {
      sence = 0;
      last = 0;
      time111 = millis();
    }

    if (last == 0 && sence == 0)
      screen1();

    if (last == 0 && sence == 1)
      screen2();
  }

}

//////////////////////////////////////////////////////////////////////

void detech_state_weather(uint16_t code, char * state_weather_daily, char * code_display ) {
  Serial.println("Step 1");
  if (code == 200 || code == 201 || code == 202) {
    strcpy(state_weather_daily, "Hard Rain");
    if (icon_transfer == 100)
      strcpy(code_display, "/Traind.bmp");
    else
      strcpy(code_display, "/Trainn.bmp");
  }
  if (code == 230 || code == 231 || code == 232 || code == 233) {
    strcpy(state_weather_daily, "Thunderstorm");
    if (icon_transfer == 100)
      strcpy(code_display, "/Td.bmp");
    else
      strcpy(code_display, "/Tn.bmp");
  }

  if (code == 300 || code == 301 || code == 302) {
    strcpy(state_weather_daily, "Drizzle");
    if (icon_transfer == 100)
      strcpy(code_display, "/Cloudyd.bmp");
    else
      strcpy(code_display, "/Cloudyn.bmp");
  }

  if (code == 500 || code == 501 || code == 502 || code == 511 || code == 520 || code == 521 || code == 522) {
    strcpy(state_weather_daily, "Drizzle");
    if (icon_transfer == 100)
      strcpy(code_display, "/Haze.bmp");
    else
      strcpy(code_display, "/Haze.bmp");
  }

  if (code == 611 || code == 612 || code == 700 || code == 711 || code == 721 || code == 731 || code == 741 || code == 751) {
    strcpy(state_weather_daily, "Haze");
    if (icon_transfer == 100)
      strcpy(code_display, "/Hazed.bmp");
    else
      strcpy(code_display, "/Hazen.bmp");
  }

  if (code == 800) {
    strcpy(state_weather_daily, "Clear sky");
    if (icon_transfer == 100)
      strcpy(code_display, "/Cleard.bmp");
    else
      strcpy(code_display, "/Clearn.bmp");
  }
  if (code == 801 || code == 802 || code == 803 || code == 804) {
    strcpy(state_weather_daily, "Cloudy");
    if (icon_transfer == 100)
      strcpy(code_display, "/Cloudyd.bmp");
    else
      strcpy(code_display, "/Cloudyn.bmp");
  }
}

void screen1() {
  last = 1;
  tft.fillScreen(0x1928);
  for (uint16_t a = 0; a < 3; a++) {
    tft.drawFastVLine(240 + a, 0, 30, 0x07E0);
  }
  tft.drawRoundRect(0, 30, 480 , 290, 5, 0x07E0);
  tft.drawRoundRect(0, 0, 480 , 30, 4, 0x07E0);
  showmsgXY(60,  20, 1, &FreeSans9pt7b , all_day_data1);
  //  showmsgXY(320, 20, 1, &FreeSans9pt7b , "7:00 PM");
  //state_weather_daily code_display_dayly
  ret = showBMP(code_display, 30, 70);  //X truớc Y sau.  //Biến trạng thái ret respond kết quả hiển thị
  ret = showBMP(heat, 300, 60 - 15);
  ret = showBMP(humi, 293, 100 - 15);
  ret = showBMP(wind, 300, 140 - 15);
  for (uint16_t a = 0; a < 380; a++) {  // Vẽ đường ngang. 90 mỗi khoảng nhiệt
    tft.drawFastVLine(40 + a, 310, 2, 0x07FF);
  }
  for (uint16_t a = 3; a < 90; a++) { //Vẽ đường dọc từ 230-300. Biểu thị từ 0-50. (1.0 độ mỗi *C)
    tft.drawFastHLine(40, 220 + a, 2, 0x07FF);
  }
  for (int a = 0; a < 4; a++) {  //Vẽ chấm vàng
    tft.fillCircle(40 + (a + 1) * 90, 310, 2, 0xFFE0);
  }
  //Chia độ dọc
  showmsgXY(15, 210, 0, &FreeSans9pt7b , "*C");
  tft.fillCircle(40 , 280, 2, 0xFFE0);
  showmsgXY(15, 285, 0, &FreeSans9pt7b , "20");
  tft.fillCircle(40 , 240, 2, 0xFFE0); //Toạ độ cao 0:320, 20*C:280, 30*C:260 40*C:240
  showmsgXY(15, 245, 0, &FreeSans9pt7b , "40");
  //Hiển thị đường nhiệt

  tft.drawLine(40, 280, 130, 268, 0xF800); //Chạy điểm 1
  showmsgXY_r(130 - 10, 268 - 15, 0, &FreeSans9pt7b , "26*C");

  tft.drawLine(130, 268, 220, 264, 0xF800);//Chạy điểm 2
  showmsgXY_r(220 - 10, 264 - 15, 0, &FreeSans9pt7b , "28*C");

  tft.drawLine(220, 264, 310, 260, 0xF800);//Chạy điểm 3
  showmsgXY_r(310 - 10, 260 - 15, 0, &FreeSans9pt7b , "29*C");

  tft.drawLine(310, 260, 400, 264, 0xF800);//Chạy điểm 4
  showmsgXY_r(400 - 10, 264 - 15, 0, &FreeSans9pt7b , "28*C");

  showmsgXY(350, 67, 1, &FreeSans9pt7b , temp_111);
  showmsgXY(350, 110, 1, &FreeSans9pt7b , cloud_111);
  showmsgXY(350, 145, 1, &FreeSans9pt7b , wind_111);
  showmsgXY(50,  60, 2, &FreeSans9pt7b , "Ha Noi");
  showmsgXY(60,  20, 1, &FreeSans9pt7b , all_day_data1);
  showmsgXY(340, 20, 1, &FreeSans9pt7b , all_time1);
  showmsgXY(300, 200, 2, &FreeSans9pt7b , state_weather_daily);

}

void screen2() {
  last = 1;
  tft.fillScreen(0x1928);
  for (uint16_t a = 0; a < 3; a++) {
    tft.drawFastVLine(240 + a, 0, 30, 0x07E0);
  }
  tft.drawRoundRect(0, 30, 480 , 290, 5, 0x07E0);
  tft.drawRoundRect(0, 0, 480 , 30, 4, 0x07E0);
  showmsgXY(60,  20, 1, &FreeSans9pt7b , all_day_data1);
  showmsgXY(340, 20, 1, &FreeSans9pt7b , all_time1);// all_time1 time_now

  //Chia ô
  for (uint16_t a = 3; a < 250; a++) { //Vẽ đường dọc từ 230-300. Biểu thị từ 0-50. (1.0 độ mỗi *C)
    tft.drawFastHLine(160, 60 + a, 2, 0x07FF);
  }
  for (uint16_t a = 3; a < 250; a++) { //Vẽ đường dọc từ 230-300. Biểu thị từ 0-50. (1.0 độ mỗi *C)
    tft.drawFastHLine(320, 60 + a, 2, 0x07FF);
  }
  //Hiển thị icon các ô
  //Ô 1
  showmsgXY(55, 50, 1, &FreeSans9pt7b , "Today");
  ret = showBMP(code_display_day1, 5, 60);  //X truớc Y sau.  //Biến trạng thái ret respond kết quả hiển thị
  ret = showBMP(heat, 25, 220);
  showmsgXY(65, 240, 1, &FreeSans9pt7b , temp_daily111);
  ret = showBMP(humi, 22, 260);
  showmsgXY(65, 285, 1, &FreeSans9pt7b , r_humi111);

  //Ô 2
  showmsgXY(210, 50, 1, &FreeSans9pt7b , "Next day");
  ret = showBMP(code_display_day2, 5 + 160, 60); //X truớc Y sau.  //Biến trạng thái ret respond kết quả hiển thị
  ret = showBMP(heat, 185, 220);
  showmsgXY(65 + 160, 240, 1, &FreeSans9pt7b , temp_daily222);

  ret = showBMP(humi, 22 + 160, 260);
  showmsgXY(65 + 160, 285, 1, &FreeSans9pt7b , r_humi222);

  //Ô 3
  showmsgXY(210 + 160 - 20, 50, 1, &FreeSans9pt7b , "Next next day");
  ret = showBMP(code_display_day3, 5 + 160 * 2, 60); //X truớc Y sau.  //Biến trạng thái ret respond kết quả hiển thị
  ret = showBMP(heat, 185 + 160, 220);
  showmsgXY(65 + 160 * 2, 240, 1, &FreeSans9pt7b , temp_daily333);

  ret = showBMP(humi, 22 + 160 * 2, 260);
  showmsgXY(65 + 160 * 2, 285, 1, &FreeSans9pt7b , r_humi333);
}


//////////////////////////////////////////////////////////////////////
#define BMPIMAGEOFFSET 54

#define BUFFPIXEL      20

uint16_t read16(File & f) {
  uint16_t result;         // read little-endian
  f.read((uint8_t*)&result, sizeof(result));
  return result;
}

uint32_t read32(File & f) {
  uint32_t result;
  f.read((uint8_t*)&result, sizeof(result));
  return result;
}

uint8_t showBMP(char *nm, int x, int y)
{
  File bmpFile;
  int bmpWidth, bmpHeight;    // W+H in pixels
  uint8_t bmpDepth;           // Bit depth (currently must be 24, 16, 8, 4, 1)
  uint32_t bmpImageoffset;    // Start of image data in file
  uint32_t rowSize;           // Not always = bmpWidth; may have padding
  uint8_t sdbuffer[3 * BUFFPIXEL];    // pixel in buffer (R+G+B per pixel)
  uint16_t lcdbuffer[(1 << PALETTEDEPTH) + BUFFPIXEL], *palette = NULL;
  uint8_t bitmask, bitshift;
  boolean flip = true;        // BMP is stored bottom-to-top
  int w, h, row, col, lcdbufsiz = (1 << PALETTEDEPTH) + BUFFPIXEL, buffidx;
  uint32_t pos;               // seek position
  boolean is565 = false;      //

  uint16_t bmpID;
  uint16_t n;                 // blocks read
  uint8_t ret;

  if ((x >= tft.width()) || (y >= tft.height()))
    return 1;               // off screen

  bmpFile = SD.open(nm);      // Parse BMP header
  bmpID = read16(bmpFile);    // BMP signature
  (void) read32(bmpFile);     // Read & ignore file size
  (void) read32(bmpFile);     // Read & ignore creator bytes
  bmpImageoffset = read32(bmpFile);       // Start of image data
  (void) read32(bmpFile);     // Read & ignore DIB header size
  bmpWidth = read32(bmpFile);
  bmpHeight = read32(bmpFile);
  n = read16(bmpFile);        // # planes -- must be '1'
  bmpDepth = read16(bmpFile); // bits per pixel
  pos = read32(bmpFile);      // format
  if (bmpID != 0x4D42) ret = 2; // bad ID
  else if (n != 1) ret = 3;   // too many planes
  else if (pos != 0 && pos != 3) ret = 4; // format: 0 = uncompressed, 3 = 565
  else if (bmpDepth < 16 && bmpDepth > PALETTEDEPTH) ret = 5; // palette
  else {
    bool first = true;
    is565 = (pos == 3);               // ?already in 16-bit format
    // BMP rows are padded (if needed) to 4-byte boundary
    rowSize = (bmpWidth * bmpDepth / 8 + 3) & ~3;
    if (bmpHeight < 0) {              // If negative, image is in top-down order.
      bmpHeight = -bmpHeight;
      flip = false;
    }

    w = bmpWidth;
    h = bmpHeight;
    if ((x + w) >= tft.width())       // Crop area to be loaded
      w = tft.width() - x;
    if ((y + h) >= tft.height())      //
      h = tft.height() - y;

    if (bmpDepth <= PALETTEDEPTH) {   // these modes have separate palette
      bmpFile.seek(BMPIMAGEOFFSET); //palette is always @ 54
      bitmask = 0xFF;
      if (bmpDepth < 8)
        bitmask >>= bmpDepth;
      bitshift = 8 - bmpDepth;
      n = 1 << bmpDepth;
      lcdbufsiz -= n;
      palette = lcdbuffer + lcdbufsiz;
      for (col = 0; col < n; col++) {
        pos = read32(bmpFile);    //map palette to 5-6-5
        palette[col] = ((pos & 0x0000F8) >> 3) | ((pos & 0x00FC00) >> 5) | ((pos & 0xF80000) >> 8);
      }
    }

    // Set TFT address window to clipped image bounds
    tft.setAddrWindow(x, y, x + w - 1, y + h - 1);
    for (row = 0; row < h; row++) { // For each scanline...
      // Seek to start of scan line.  It might seem labor-
      // intensive to be doing this on every line, but this
      // method covers a lot of gritty details like cropping
      // and scanline padding.  Also, the seek only takes
      // place if the file position actually needs to change
      // (avoids a lot of cluster math in SD library).
      uint8_t r, g, b, *sdptr;
      int lcdidx, lcdleft;
      if (flip)   // Bitmap is stored bottom-to-top order (normal BMP)
        pos = bmpImageoffset + (bmpHeight - 1 - row) * rowSize;
      else        // Bitmap is stored top-to-bottom
        pos = bmpImageoffset + row * rowSize;
      if (bmpFile.position() != pos) { // Need seek?
        bmpFile.seek(pos);
        buffidx = sizeof(sdbuffer); // Force buffer reload
      }

      for (col = 0; col < w; ) {  //pixels in row
        lcdleft = w - col;
        if (lcdleft > lcdbufsiz) lcdleft = lcdbufsiz;
        for (lcdidx = 0; lcdidx < lcdleft; lcdidx++) { // buffer at a time
          uint16_t color;
          // Time to read more pixel data?
          if (buffidx >= sizeof(sdbuffer)) { // Indeed
            bmpFile.read(sdbuffer, sizeof(sdbuffer));
            buffidx = 0; // Set index to beginning
            r = 0;
          }
          switch (bmpDepth) {          // Convert pixel from BMP to TFT format
            case 24:
              b = sdbuffer[buffidx++];
              g = sdbuffer[buffidx++];
              r = sdbuffer[buffidx++];
              color = tft.color565(r, g, b);
              break;
            case 16:
              b = sdbuffer[buffidx++];
              r = sdbuffer[buffidx++];
              if (is565)
                color = (r << 8) | (b);
              else
                color = (r << 9) | ((b & 0xE0) << 1) | (b & 0x1F);
              break;
            case 1:
            case 4:
            case 8:
              if (r == 0)
                b = sdbuffer[buffidx++], r = 8;
              color = palette[(b >> bitshift) & bitmask];
              r -= bmpDepth;
              b <<= bmpDepth;
              break;
          }
          lcdbuffer[lcdidx] = color;

        }
        tft.pushColors(lcdbuffer, lcdidx, first);
        first = false;
        col += lcdidx;
      }           // end cols
    }               // end rows
    tft.setAddrWindow(0, 0, tft.width() - 1, tft.height() - 1); //restore full screen
    ret = 0;        // good render
  }
  bmpFile.close();
  return (ret);
}

/***********Function get data********************/
//////////////////////////////////////////////////

void get_time()
{
  time_t now = time(nullptr);
  String t = ctime(&now);
  // Serial.println(ctime(&now));
  thu   = t.substring(0, 3);
  //thu.toUpperCase();
  t.remove(0, 4);
  thang = t.substring(0, 3);
  t.remove(0, 4);
  ngay  = t.substring(0, 2);
  ngay.trim();
  ngay_int = ngay.toInt();
  t.remove(0, 3);
  gio   = t.substring(0, 2);
  gio_int = gio.toInt();
  t.remove(0, 3);
  phut  = t.substring(0, 2);
  i_phut = phut.toInt();
  phut_int = phut.toInt();
  t.remove(0, 3);
  giay  = t.substring(0, 2);
  i_giay = giay.toInt();
  giay_int = giay.toInt();
  t.remove(0, 3);
  nam   = t.substring(0, 4);
  nam_int = nam.toInt();
  if (ngay.toInt() < 10)
    ngay = "0" + ngay;  // So sánh dạng String.
  all_day_data11 = thu + "," + ngay + " " + thang + " " + nam;
  all_day_data11.toCharArray(all_day_data1, 20); //Với allday_data11 là chuỗi String, convert sang char array
  Serial.println(all_day_data1);
}
////////////////////


//=========================================================================//
//=========================================================================//
//=========================================================================//

void get_current() {
  if (!client.connect("api.weatherbit.io", 80)) {
    Serial.println(F("Connection failed"));
    return;
  }
  Serial.println(F("Connected!"));
  // Send HTTP request
  //client.println(F("v2.0/current?city=hanoi,VN&key=" + String(key) ));  //http://api.weatherbit.io/v2.0/current?city=hanoi,VN&key=ca66835ddbaa496c9d11aee5f48fd28e
  client.println(http + key + suffix);
  //client.println(all);
  // client.println(F("GET http://api.weatherbit.io/v2.0/current?city=hanoi,VN&key=15b46c99b8df4dd5914d2dec3f143acc HTTP/1.0"));
  client.println(F("Host: api.weatherbit.io"));
  client.println(F("Connection: close"));
  if (client.println() == 0) {
    Serial.println(F("Failed to send request"));
    return;
  }

  // Check HTTP statusSS
  char status[32] = {0};
  client.readBytesUntil('\r', status, sizeof(status));
  if (strcmp(status, "HTTP/1.1 200 OK") != 0) {
    Serial.print(F("Unexpected response: "));
    Serial.println(status);
    return;
  }

  // Skip HTTP headers
  char endOfHeaders[] = "\r\n\r\n";
  if (!client.find(endOfHeaders)) {
    Serial.println(F("Invalid response"));
    return;
  }

  // Allocate the JSON document
  // Use arduinojson.org/v6/assistant to compute the capacity.
  const size_t capacity = JSON_ARRAY_SIZE(1) + JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(3) + JSON_OBJECT_SIZE(35) + 440;
  DynamicJsonDocument doc(capacity);
  //
  //  Serial.print("All data:");
  //  Serial.println(client);

  // Parse JSON object
  DeserializationError error = deserializeJson(doc, client);
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.c_str());
    return;
  }
  //=========================================================================//
  // Extract values

  JsonObject data_0 = doc["data"][0];
  cloud_daily = data_0["clouds"]; // 75
  wind_speed_daily = data_0["wind_spd"]; // 3.1
  r_humi = data_0["rh"]; // 66

  JsonObject data_0_weather = data_0["weather"];
  const char* icon_daily = data_0_weather["icon"]; // "c03n"
  code_daily = data_0_weather["code"]; // "500"
  const char* description_daily = data_0_weather["description"]; // "Light rain"
  temp_daily = data_0["temp"]; // 18
  //Chuyển code day or night
  state_icon_daily = icon_daily[3];
  int i(state_icon_daily);
  icon_transfer = i;
  // Disconnect

  client.stop();
  Serial.print("Cloud :");
  Serial.println(cloud_daily);

  Serial.print("Relative Humidity :");
  Serial.println(r_humi);

  Serial.print("Wind Speed :");
  Serial.println(wind_speed_daily);

  Serial.print("Icon :");
  Serial.println(icon_daily);

  Serial.print("Code :");
  Serial.println(code_daily);

  Serial.print("Cal Temp :");
  Serial.println(temp_daily);


  Serial.print("Weather Condition :");
  Serial.println(description_daily);

  Serial.print("Stable Code :");
  Serial.println(state_icon_daily);

  Serial.print("Transfer Code :");
  Serial.println(icon_transfer);


  String a, b, c;
  a = String(temp_daily) + " *C";
  b = String(wind_speed_daily) + " m/s";
  c = String(r_humi) + " %";
  a.toCharArray(temp_111, 10);
  b.toCharArray(wind_111, 10);  //cloud_111
  c.toCharArray(cloud_111, 10);   //wind_111
  detech_state_weather(code_daily, state_weather_daily, code_display);
  Serial.println("Converting :");
  Serial.println(code_daily);
  Serial.println(state_weather_daily);
  Serial.println(code_display);
  Serial.println(temp_111);
  Serial.println(cloud_111);
  Serial.println(wind_111);

}



void get_3days() {
  if (!client.connect("api.weatherbit.io", 80)) {
    Serial.println(F("Connection failed"));
    return;
  }
  Serial.println(F("Connected!"));
  // Send HTTP request
  //client.println(F("v2.0/forecast/daily?city=hanoi,VN&key=" + String(key)+"&days=3" ));  //http://api.weatherbit.io/v2.0/current?city=hanoi,VN&key=ca66835ddbaa496c9d11aee5f48fd28e
  //client.println(http + key + "&days=3" + suffix);
  //client.println(all);
  client.println(F("GET http://api.weatherbit.io/v2.0/forecast/daily?city=hanoi,VN&key=ca66835ddbaa496c9d11aee5f48fd28e&days=3 HTTP/1.1"));
  client.println(F("Host: api.weatherbit.io"));
  client.println(F("Connection: close"));
  if (client.println() == 0) {
    Serial.println(F("Failed to send request"));
    return;
  }

  // Check HTTP status
  char status[32] = {0};
  client.readBytesUntil('\r', status, sizeof(status));
  if (strcmp(status, "HTTP/1.1 200 OK") != 0) {
    Serial.print(F("Unexpected response: "));
    Serial.println(status);
    return;
  }

  // Skip HTTP headers
  char endOfHeaders[] = "\r\n\r\n";
  if (!client.find(endOfHeaders)) {
    Serial.println(F("Invalid response"));
    return;
  }

  // Allocate the JSON document
  // Use arduinojson.org/v6/assistant to compute the capacity.
  const size_t capacity = 3400;
  DynamicJsonDocument doc(capacity);
  DeserializationError error = deserializeJson(doc, client);
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.c_str());
    return;
  }
  //=========================================================================//
  //
  JsonArray data = doc["data"];
  JsonObject data_0 = data[0];
  int data_0_rh = data_0["rh"]; // 83
  r_humi1 = data_0_rh;
  const char* data_0_valid_date = data_0["valid_date"]; // "2019-04-12"
  JsonObject data_0_weather = data_0["weather"];
  unsigned int data_0_weather_code = data_0_weather["code"]; // 803
  code_daily1 = data_0_weather_code;
  const char* data_0_weather_description = data_0_weather["description"]; // "Broken clouds"
  float data_0_temp = data_0["temp"]; // 28.4
  temp_daily1 = data_0_temp;
  float data_0_max_temp = data_0["max_temp"]; // 31.6
  float data_0_min_temp = data_0["min_temp"]; // 25.4

  //Khối dữ liệu 2
  JsonObject data_1 = data[1];
  int data_1_rh = data_1["rh"]; // 81
  r_humi2 = data_1_rh;
  const char* data_1_valid_date = data_1["valid_date"]; // "2019-04-13"
  JsonObject data_1_weather = data_1["weather"];
  unsigned int data_1_weather_code = data_1_weather["code"]; // 803
  code_daily2 = data_1_weather_code;
  const char* data_1_weather_description = data_1_weather["description"]; // "Thunderstorm with rain"
  float data_1_temp = data_1["temp"]; // 28.4
  temp_daily2 = data_1_temp;
  float data_1_max_temp = data_1["max_temp"]; // 32.6
  float data_1_min_temp = data_1["min_temp"]; // 26.2

  //Khối dữ liệu 3
  JsonObject data_2 = data[2];
  int data_2_rh = data_2["rh"]; // 81
  r_humi3 = data_2_rh;
  const char* data_2_valid_date = data_2["valid_date"]; // "2019-04-14"
  JsonObject data_2_weather = data_2["weather"];
  unsigned int data_2_weather_code = data_2_weather["code"]; // 803
  code_daily3 = data_2_weather_code;
  const char* data_2_weather_description = data_2_weather["description"]; // "Overcast clouds"
  float data_2_temp = data_2["temp"]; // 28.4
  temp_daily3 = data_2_temp;
  float data_2_max_temp = data_2["max_temp"]; // 32.8
  float data_2_min_temp = data_2["min_temp"]; // 26.5

  //==============================================================================//
  // Disconnect

  client.stop();
  Serial.print("Code 1:");
  Serial.println(code_daily1);

  Serial.print("Code 2:");
  Serial.println(code_daily2);

  Serial.print("Code 3:");
  Serial.println(code_daily3);

  Serial.print("Condition 1:");
  Serial.println(data_0_weather_description);

  Serial.print("Condition 2:");
  Serial.println(data_1_weather_description);

  Serial.print("Condition 3:");
  Serial.println(data_2_weather_description);

  Serial.print("Humi 1:");
  Serial.println(r_humi1);

  Serial.print("Humi 2:");
  Serial.println(r_humi2);

  Serial.print("Humi 3:");
  Serial.println(r_humi3);

  Serial.print("Temp 1:");
  Serial.println(temp_daily1);

  Serial.print("Temp 2:");
  Serial.println(temp_daily2);

  Serial.print("Temp 3:");
  Serial.println(temp_daily3);

  Serial.print("Data Converting");
  String a, b, c, d, e, f;
  a = String(r_humi1) + " %";
  b = String(r_humi2) + " %";
  c = String(r_humi3) + " %";
  d = String(temp_daily1) + "*C";
  e = String(temp_daily2) + "*C";
  f = String(temp_daily3) + "*C";
  a.toCharArray(r_humi111, 10);
  b.toCharArray(r_humi222, 10);
  c.toCharArray(r_humi333, 10);
  d.toCharArray(temp_daily111, 10);
  e.toCharArray(temp_daily222, 10);
  f.toCharArray(temp_daily333, 10);
  Serial.println(r_humi111);
  Serial.println(r_humi222);
  Serial.println(r_humi333);
  Serial.println(temp_daily111);
  Serial.println(temp_daily222);
  Serial.println(temp_daily333);
  detech_state_weather(code_daily1, state_weather_day1, code_display_day1);
  detech_state_weather(code_daily2, state_weather_day2, code_display_day2);
  detech_state_weather(code_daily3, state_weather_day3, code_display_day3);

}


void get_hours() {
  if (!client.connect("api.weatherbit.io", 80)) {
    Serial.println(F("Connection failed"));
    return;
  }
  Serial.println(F("Connected!"));
  // Send HTTP request
  //client.println(F("v2.0/forecast/daily?city=hanoi,VN&key=" + String(key)+"&days=3" ));  //http://api.weatherbit.io/v2.0/current?city=hanoi,VN&key=ca66835ddbaa496c9d11aee5f48fd28e
  //client.println(http + key + "&days=3" + suffix);
  //client.println(all);
  client.println(F("GET http://api.weatherbit.io/v2.0/forecast/hourly?city=hanoi,VN&key=ca66835ddbaa496c9d11aee5f48fd28e&hours=4 HTTP/1.1"));
  client.println(F("Host: api.weatherbit.io"));
  client.println(F("Connection: close"));
  if (client.println() == 0) {
    Serial.println(F("Failed to send request"));
    return;
  }

  // Check HTTP status
  char status[32] = {0};
  client.readBytesUntil('\r', status, sizeof(status));
  if (strcmp(status, "HTTP/1.1 200 OK") != 0) {
    Serial.print(F("Unexpected response: "));
    Serial.println(status);
    return;
  }

  // Skip HTTP headers
  char endOfHeaders[] = "\r\n\r\n";
  if (!client.find(endOfHeaders)) {
    Serial.println(F("Invalid response"));
    return;
  }

  // Allocate the JSON document
  // Use arduinojson.org/v6/assistant to compute the capacity.
  const size_t capacity = 3970;
  DynamicJsonDocument doc(capacity);
  DeserializationError error = deserializeJson(doc, client);
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.c_str());
    return;
  }
  //=========================================================================//
  //
  JsonArray data = doc["data"];
  JsonObject data_0 = data[0];
  JsonObject data_0_weather = data_0["weather"];
  float data_0_temp = data_0["temp"]; // 28.9
  temp_hour1 = data_0_temp;
  JsonObject data_1 = data[1];
  JsonObject data_1_weather = data_1["weather"];
  float data_1_temp = data_1["temp"]; // 29.6
  temp_hour2 = data_1_temp;
  JsonObject data_2 = data[2];
  JsonObject data_2_weather = data_2["weather"];
  float data_2_temp = data_2["temp"]; // 30.1
  temp_hour3 = data_2_temp;
  JsonObject data_3 = data[3];
  JsonObject data_3_weather = data_3["weather"];
  float data_3_temp = data_3["temp"]; // 30.1
  temp_hour4 = data_3_temp;
  // Disconnect
  client.stop();

  Serial.print("Temp1 :");
  Serial.println(temp_hour1);

  Serial.print("Temp2 :");
  Serial.println(temp_hour2);

  Serial.print("Temp3 :");
  Serial.println(temp_hour3);

  Serial.print("Temp4 :");
  Serial.println(temp_hour4);

}
void showmsgXY(int x, int y, int sz, const GFXfont * f, const char *msg)
{
  int16_t x1, y1;
  uint16_t wid, ht;
  tft.setFont(f);
  tft.setCursor(x, y);
  tft.setTextColor(0xFFFF);   //Set white color
  tft.setTextSize(sz);
  tft.print(msg);
}

void showmsgXY_r(int x, int y, int sz, const GFXfont * f, const char *msg)
{
  int16_t x1, y1;
  uint16_t wid, ht;
  tft.setFont(f);
  tft.setCursor(x, y);
  tft.setTextColor(0xF800);   //Set white color
  tft.setTextSize(sz);
  tft.print(msg);
}

void count_time ()
{
  giay_int++;
  if (giay_int == 60) {
    giay_int = 0;
    phut_int++;
  }
  if (phut_int == 60) {
    phut_int = 0;
    gio_int++;
    get_time();
  }
  if (gio_int >= 1 && phut_int >= 10)
    all_time11 = String(gio_int) + ":" + String(phut_int);
  else if (gio_int == 0 && phut_int >= 10)
    all_time11 = "0" + String(gio_int) + ":" + String(phut_int);
  else if (gio_int == 0 && phut_int < 10)
    all_time11 = "0" + String(gio_int) + ":" + "0" + String(phut_int);
  else
    all_time11 = String(gio_int) + ":" + String(phut_int);

  all_time11.toCharArray(all_time1, 20);
  Serial.println(all_time1);
}
