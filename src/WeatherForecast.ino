   //////////////////////////////////////////////
  //    Art Deco Weather Forecast Display     //
 //                                          //
//           http://www.educ8s.tv           //
/////////////////////////////////////////////

#include <Wire.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
// #include <Adafruit_ST7735.h>
#include <Adafruit_ST7789.h> // Hardware-specific library for ST7789
#include <Adafruit_GFX.h>
#include <Fonts/FreeMono11pt8b.h>
#include <ezTime.h>

// Color definitions
#define BLACK    0x0000
#define BLUE     0x001F
#define RED      0xF800
#define GREEN    0x07E0
#define CYAN     0x07FF
#define MAGENTA  0xF81F
#define YELLOW   0xFFE0 
#define WHITE    0xFFFF
#define GREY     0xC618

#define TFT_CS   0  
#define TFT_DC   4   
#define TFT_RST  2 

// the two next lines must be changed together
// WEATHERDATA_SIZE defines how many forecast elements are fetched from openweathermaps
// weatherJSONsize was created using https://arduinojson.org/v6/assistant/ for this particular number
#define WEATHERDATA_SIZE 8
const size_t weatherJSONsize = 8*JSON_ARRAY_SIZE(1) + JSON_ARRAY_SIZE(8) + 16*JSON_OBJECT_SIZE(1) + 9*JSON_OBJECT_SIZE(2) + 8*JSON_OBJECT_SIZE(4) + JSON_OBJECT_SIZE(5) + 9*JSON_OBJECT_SIZE(7) + 8*JSON_OBJECT_SIZE(9) + 2000;
DynamicJsonDocument root(weatherJSONsize);

long nextpoll;
long nextswitch;


const char* ssid     = "Buschfunk";      // SSID of local network
const char* password = "FritzBoxIstTotalSuper";   // Password on network
String APIKEY = "fb1d7728528b56504cb6af0aba6c6fbc";
String CityID = "2885397"; //Sparta, Greece
Timezone myTZ;

//bitmap width and height
const int bmpw = 128;
const int bmph = 90;

// In order to make the code more flexible reading different display size
// here you need to define areas for the different display items time, icon, 
// optionally description (on a larger display). and temperature
// The code will center the respective items inside these areas
// the original code assumes a display size of 128x160 with an icon area of
// (x,y,w,h,): (0,35,128,90)

// values for 80x160 display
// const int timeareax=24;
// const int timeareay=0;
// const int timeareaw=80;
// const int timeareah=35;

// const int iconareax=24;
// const int iconareay=35;
// const int iconareaw=80;
// const int iconareah=90;
//top left corner of the 128x90 icon area centered on the iconareaw
// const int iconareacx = iconareax+(iconareaw/2)-(bmpw/2);

// const bool descrarea=false;

// const int tempareax=24;
// const int tempareay=125;
// const int tempareaw=80;
// const int tempareah=35;

// values for 240x240 display with additional data (description, wind)
const int timeareax=0;
const int timeareay=0;
const int timeareaw=240;
const int timeareah=35;

const int iconareax=0;
const int iconareay=35;
const int iconareaw=240;
const int iconareah=90;
//top left corner of the 128x90 icon area centered on the iconareaw
const int iconareacx = iconareax+(iconareaw/2)-(bmpw/2);

const bool descrarea=true;
const int descrareax=0;
const int descrareay=125;
const int descrareaw=240;
const int descrareah=75;

const int tempareax=0;
const int tempareay=205;
const int tempareaw=240;
const int tempareah=35;


  int TESTCOUNTER=-1;
  const int TESTARR[]= {800, 801, 802, 803, 804, 200,300,500,511,520,521,522,531,600,601,602,611,612,615,616,620,621,622,701};
  const int TESTARRSIZE = sizeof(TESTARR)/sizeof(TESTARR[0]);


WiFiClient client;
char servername[]="api.openweathermap.org";  // remote server we will connect to

boolean night = false;
int  counter = 360;
String weatherDescription ="";
String weatherLocation = "";
float Temperature;

struct weatherdata
{
  char time[14];
  float temp;
  int weatherID;
  char description[20];
  char wind[14]; //270 @ 15 km/h
};

// typedef struct weatherdata Weatherdata;
// Weatherdata theWeatherdata;

struct weatherdata theWeatherdata[WEATHERDATA_SIZE];
int slot = 0;

extern  unsigned char  cloud[];
extern  unsigned char  thunder[];
extern  unsigned char  wind[];



// Init ST7735 80x160
// Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

  // Init ST7789 240x240
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);


/* UTF-8 to ISO-8859-1/ISO-8859-15 mapper.
 * Return 0..255 for valid ISO-8859-15 code points, 256 otherwise.
*/
static inline unsigned int to_latin9(const unsigned int code)
{
    /* Code points 0 to U+00FF are the same in both. */
    if (code < 256U)
        return code;
    switch (code) {
    case 0x0152U: return 188U; /* U+0152 = 0xBC: OE ligature */
    case 0x0153U: return 189U; /* U+0153 = 0xBD: oe ligature */
    case 0x0160U: return 166U; /* U+0160 = 0xA6: S with caron */
    case 0x0161U: return 168U; /* U+0161 = 0xA8: s with caron */
    case 0x0178U: return 190U; /* U+0178 = 0xBE: Y with diaresis */
    case 0x017DU: return 180U; /* U+017D = 0xB4: Z with caron */
    case 0x017EU: return 184U; /* U+017E = 0xB8: z with caron */
    case 0x20ACU: return 164U; /* U+20AC = 0xA4: Euro */
    default:      return 256U;
    }
}

/* Convert an UTF-8 string to ISO-8859-15.
 * All invalid sequences are ignored.
 * Note: output == input is allowed,
 * but   input < output < input + length
 * is not.
 * Output has to have room for (length+1) chars, including the trailing NUL byte.
*/
size_t utf8_to_latin9(char *const output, const char *const input, const size_t length)
{
    unsigned char             *out = (unsigned char *)output;
    const unsigned char       *in  = (const unsigned char *)input;
    const unsigned char *const end = (const unsigned char *)input + length;
    unsigned int               c;

    while (in < end)
        if (*in < 128)
            *(out++) = *(in++); /* Valid codepoint */
        else
        if (*in < 192)
            in++;               /* 10000000 .. 10111111 are invalid */
        else
        if (*in < 224) {        /* 110xxxxx 10xxxxxx */
            if (in + 1 >= end)
                break;
            if ((in[1] & 192U) == 128U) {
                c = to_latin9( (((unsigned int)(in[0] & 0x1FU)) << 6U)
                             |  ((unsigned int)(in[1] & 0x3FU)) );
                if (c < 256)
                    *(out++) = c;
            }
            in += 2;

        } else
        if (*in < 240) {        /* 1110xxxx 10xxxxxx 10xxxxxx */
            if (in + 2 >= end)
                break;
            if ((in[1] & 192U) == 128U &&
                (in[2] & 192U) == 128U) {
                c = to_latin9( (((unsigned int)(in[0] & 0x0FU)) << 12U)
                             | (((unsigned int)(in[1] & 0x3FU)) << 6U)
                             |  ((unsigned int)(in[2] & 0x3FU)) );
                if (c < 256)
                    *(out++) = c;
            }
            in += 3;

        } else
        if (*in < 248) {        /* 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx */
            if (in + 3 >= end)
                break;
            if ((in[1] & 192U) == 128U &&
                (in[2] & 192U) == 128U &&
                (in[3] & 192U) == 128U) {
                c = to_latin9( (((unsigned int)(in[0] & 0x07U)) << 18U)
                             | (((unsigned int)(in[1] & 0x3FU)) << 12U)
                             | (((unsigned int)(in[2] & 0x3FU)) << 6U)
                             |  ((unsigned int)(in[3] & 0x3FU)) );
                if (c < 256)
                    *(out++) = c;
            }
            in += 4;

        } else
        if (*in < 252) {        /* 111110xx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx */
            if (in + 4 >= end)
                break;
            if ((in[1] & 192U) == 128U &&
                (in[2] & 192U) == 128U &&
                (in[3] & 192U) == 128U &&
                (in[4] & 192U) == 128U) {
                c = to_latin9( (((unsigned int)(in[0] & 0x03U)) << 24U)
                             | (((unsigned int)(in[1] & 0x3FU)) << 18U)
                             | (((unsigned int)(in[2] & 0x3FU)) << 12U)
                             | (((unsigned int)(in[3] & 0x3FU)) << 6U)
                             |  ((unsigned int)(in[4] & 0x3FU)) );
                if (c < 256)
                    *(out++) = c;
            }
            in += 5;

        } else
        if (*in < 254) {        /* 1111110x 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx */
            if (in + 5 >= end)
                break;
            if ((in[1] & 192U) == 128U &&
                (in[2] & 192U) == 128U &&
                (in[3] & 192U) == 128U &&
                (in[4] & 192U) == 128U &&
                (in[5] & 192U) == 128U) {
                c = to_latin9( (((unsigned int)(in[0] & 0x01U)) << 30U)
                             | (((unsigned int)(in[1] & 0x3FU)) << 24U)
                             | (((unsigned int)(in[2] & 0x3FU)) << 18U)
                             | (((unsigned int)(in[3] & 0x3FU)) << 12U)
                             | (((unsigned int)(in[4] & 0x3FU)) << 6U)
                             |  ((unsigned int)(in[5] & 0x3FU)) );
                if (c < 256)
                    *(out++) = c;
            }
            in += 6;

        } else
            in++;               /* 11111110 and 11111111 are invalid */

    /* Terminate the output string. */
    *out = '\0';

    return (size_t)(out - (unsigned char *)output);
}













void setup() {
  Serial.begin(115200);

  // Init ST7735 80x160
  // tft.initR(INITR_GREENTAB);
  // tft.setRotation(ST7735_MADCTL_BGR);
  // tft.invertDisplay(true);

  // Init ST7789 240x240
  tft.init(240, 240, SPI_MODE2);
  tft.invertDisplay(true);


  tft.fillScreen(BLACK);

  Serial.println("Connecting");
  WiFi.begin(ssid, password);

  tft.setFont(&FreeMono11pt8b);
  tft.setTextColor(WHITE);
  tft.setTextSize(1);
  drawCentreChar("Connecting...", tft.width()/2, tft.height()/2);

  Serial.print("display width: ");
  Serial.println(tft.width());
  Serial.print("display height: ");
  Serial.println(tft.height());


  while (WiFi.status() != WL_CONNECTED) {
  delay(500);
  }
  Serial.println("Connected, waiting for timesync...");
  waitForSync();
  Serial.println("UTC:             " + UTC.dateTime());
  myTZ.setLocation(F("de"));
  myTZ.setDefault();
  Serial.print(F("Germany:         "));
  Serial.println(myTZ.dateTime());
  nextpoll = now();
  nextswitch = now()+10;
}

void loop() {

  // if (TESTCOUNTER<(TESTARRSIZE-1))
  //   TESTCOUNTER++;
  // else
  //   TESTCOUNTER = 0;
  

  //Get new data every 30 minutes
  if(now() > nextpoll ){
    nextpoll = now() + 1800;
    Serial.print("POLL ");
    Serial.println(now());
    bool success = getWeatherData();
    if (success){
      printData(0);
    }
  }

  if (now() >= nextswitch){
    nextswitch = now()+5;
    if (slot<WEATHERDATA_SIZE-1) slot++;
    else slot=0;
    Serial.print("MARK ");
    Serial.print(slot);
    Serial.print(": ");
    Serial.println(now());
    printData(slot);
  }
  

}

boolean getWeatherData() //client function to send/receive GET request data.
{

  HTTPClient http;
  http.useHTTP10(true);

  http.begin(client, "http://api.openweathermap.org/data/2.5/forecast?id="+CityID+"&units=metric&cnt=8&APPID="+APIKEY+"&lang=de");
  
  http.GET();

  // StaticJsonDocument<1024> root;

  // for testing purposes
  // char result[]="{\"cod\":\"200\",\"message\":0,\"cnt\":1,\"list\":[{\"dt\":1586034000,\"main\":{\"temp\":37.1,\"feels_like\":2.87,\"temp_min\":7.1,\"temp_max\":7.71,\"pressure\":1025,\"sea_level\":1025,\"grnd_level\":1019,\"humidity\":61,\"temp_kf\":-0.61},\"weather\":[{\"id\":800,\"main\":\"Clear\",\"description\":\"clear sky\",\"icon\":\"01n\"}],\"clouds\":{\"all\":0},\"wind\":{\"speed\":3.23,\"deg\":126},\"sys\":{\"pod\":\"n\"},\"dt_txt\":\"2020-04-04 21:00:00\"}],\"city\":{\"id\":2885397,\"name\":\"Korschenbroich\",\"coord\":{\"lat\":51.1914,\"lon\":6.5135},\"country\":\"DE\",\"timezone\":7200,\"sunrise\":1585976515,\"sunset\":1586023881}}";
  // DeserializationError error = deserializeJson(root, result);


  // Deserialize the JSON document
  DeserializationError error = deserializeJson(root, http.getStream());
  // Test if parsing succeeds.
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.c_str());
    return false;
  }
  http.end();


  for (int cnt=0;cnt<WEATHERDATA_SIZE; cnt++){
    theWeatherdata[cnt].temp = root["list"][cnt]["main"]["temp"];
    theWeatherdata[cnt].weatherID = root["list"][cnt]["weather"][0]["id"];
    // dateTime(myTZ.tzTime(root["list"][cnt]["dt"], UTC_TIME), "H:i").toCharArray(theWeatherdata[cnt].time,6);

    //theWeatherdata[cnt].time = root["list"][cnt]["dt"];
    generateTimeString(root["list"][cnt]["dt"], theWeatherdata[cnt].time);

    // const char* description;
    // description = root["list"][cnt]["weather"][0]["description"];
    // const char* description = "Überwiegend bewölkt";
    char description [30] = "Überwiegend bewölkt";
    //utf8_to_latin9 (description, theWeatherdata[cnt].description, 20);
    utf8_to_latin9 (description, description, 20);
    strcpy(theWeatherdata[cnt].description, description);

    int winddir = root["list"][cnt]["wind"]["deg"];
    char winddirchar[4];
    itoa(winddir,winddirchar,10);
    int windspeed = root["list"][cnt]["wind"]["speed"]; // m/s in metric
    char windspeedchar[4];
    sprintf(windspeedchar,"%3.0f",windspeed*3.6);
    strcpy(theWeatherdata[cnt].wind, winddirchar);
    strcat(theWeatherdata[cnt].wind, "@");
    strcat(theWeatherdata[cnt].wind, windspeedchar);
    strcat(theWeatherdata[cnt].wind, "km/h");

    Serial.print("element: ");
    Serial.println(cnt);
    Serial.print("Time: ");
    Serial.println(theWeatherdata[cnt].time);
    Serial.print("Temp: ");
    Serial.println(theWeatherdata[cnt].temp);
    Serial.print("weatherid: ");
    Serial.println(theWeatherdata[cnt].weatherID);
    Serial.print("description: ");
    Serial.println(theWeatherdata[cnt].description);
    Serial.print("wind: ");
    Serial.println(theWeatherdata[cnt].wind);

    Serial.println("-------------------------------");
  }
  return true;
}



// we can't just use myTZ.hour(dt) because ezTime is broken:
// https://github.com/ropg/ezTime/issues/10
// https://github.com/ropg/ezTime/issues/32
void generateTimeString(long dt, char *str){
    char t[6];
    myTZ.dateTime(myTZ.tzTime(dt,UTC_TIME),"H:i").toCharArray(t,6);
    Serial.println(dt);

// this comparison is also broken. It will switch days only on midnight, UTC
    if (UTC.day() == UTC.day(dt)){
      strcpy(str, "Heute, ");
    }
    else if (UTC.day()+1 == UTC.day(dt)){
      strcpy(str, "Morgen, ");
    }
    else {
      Serial.println (" WTF!?");
      strcpy(str, "WTF?!");
    }
    strcat(str, t);
    Serial.println(str);
}

void printData(int slot)
{
  clearScreen();
  tft.setTextColor(WHITE);
  tft.setTextSize(1);
  // drawCentreChar(theWeatherdata[0].time, timeareax+timeareaw/2, timeareay+timeareah/2);
  drawCentreChar(theWeatherdata[slot].time, timeareax+timeareaw/2, timeareay+timeareah/2);

  // printWeatherIcon(theWeatherdata[0].weatherID);
  printWeatherIcon(theWeatherdata[slot].weatherID);

  char degC[3] = " C";
  char tempstr[6];
  char all[9] = "";
  dtostrf(theWeatherdata[slot].temp,2,1, tempstr);
  strcat(all, tempstr);
  strcat(all, degC);
  drawCentreChar(all, tempareax+tempareaw/2, tempareay+tempareah/2);

  if (descrarea){
      drawCentreChar(theWeatherdata[slot].description, descrareax+descrareaw/2, descrareay);
      drawCentreChar(theWeatherdata[slot].wind, descrareax+descrareaw/2, descrareay+15);

  }

  // tft.fillRect(descrareax,descrareay,descrareaw,descrareah,GREY);

  // tft.setCursor(55,130);
  // tft.setTextColor(WHITE);
  // tft.setTextSize(1);
  // tft.print("o");
  // tft.setCursor(60,132);
  // tft.setTextColor(WHITE);
  // tft.setTextSize(2);
  // tft.print("°C");
}

void drawCentreChar(const char *buf, int x, int y)
{
    int16_t x1, y1;
    uint16_t w, h;
    //put x=0 to avoid text wrapping
    tft.getTextBounds(buf, 0, y, &x1, &y1, &w, &h); //calc width of new string
    tft.setCursor(x - w / 2, y);
    tft.print(buf);
}

// see https://openweathermap.org/weather-conditions
void printWeatherIcon(int id)
{
 switch(id)
 {
  case 800: drawClearWeather(); break;
  case 801: drawFewClouds(); break;
  case 802: drawFewClouds(); break;
  case 803: drawCloud(); break;
  case 804: drawCloud(); break;
  
  case 200: drawThunderstorm(); break;
  case 201: drawThunderstorm(); break;
  case 202: drawThunderstorm(); break;
  case 210: drawThunderstorm(); break;
  case 211: drawThunderstorm(); break;
  case 212: drawThunderstorm(); break;
  case 221: drawThunderstorm(); break;
  case 230: drawThunderstorm(); break;
  case 231: drawThunderstorm(); break;
  case 232: drawThunderstorm(); break;

  case 300: drawLightRain(); break;
  case 301: drawLightRain(); break;
  case 302: drawLightRain(); break;
  case 310: drawLightRain(); break;
  case 311: drawLightRain(); break;
  case 312: drawLightRain(); break;
  case 313: drawLightRain(); break;
  case 314: drawLightRain(); break;
  case 321: drawLightRain(); break;

  case 500: drawLightRainWithSunOrMoon(); break;
  case 501: drawLightRainWithSunOrMoon(); break;
  case 502: drawLightRainWithSunOrMoon(); break;
  case 503: drawLightRainWithSunOrMoon(); break;
  case 504: drawLightRainWithSunOrMoon(); break;
  case 511: drawLightRain(); break;
  case 520: drawModerateRain(); break;
  case 521: drawModerateRain(); break;
  case 522: drawHeavyRain(); break;
  case 531: drawHeavyRain(); break;

  case 600: drawLightSnowfall(); break;
  case 601: drawModerateSnowfall(); break;
  case 602: drawHeavySnowfall(); break;
  case 611: drawLightSnowfall(); break;
  case 612: drawLightSnowfall(); break;
  case 615: drawLightSnowfall(); break;
  case 616: drawLightSnowfall(); break;
  case 620: drawLightSnowfall(); break;
  case 621: drawModerateSnowfall(); break;
  case 622: drawHeavySnowfall(); break;

  case 701: drawFog(); break;
  case 711: drawFog(); break;
  case 721: drawFog(); break;
  case 731: drawFog(); break;
  case 741: drawFog(); break;
  case 751: drawFog(); break;
  case 761: drawFog(); break;
  case 762: drawFog(); break;
  case 771: drawWind(); break;
  case 781: drawWind(); break;

  default:break; 
 }
}

void clearScreen()
{
    tft.fillScreen(BLACK);
}

void drawClearWeather()
{
  if(night)
  {
    drawTheMoon();
  }else
  {
    drawTheSun();
  }
}

void drawFewClouds()
{
  if(night)
  {
    drawCloudAndTheMoon();
  }else
  {
    drawCloudWithSun();
  }
}

void drawTheSun()
{
    tft.fillCircle(iconareacx+(bmpw/2),iconareay+iconareah/2,iconareah/3,YELLOW);
}

void drawTheFullMoon()
{
    tft.fillCircle(iconareacx+(bmpw/2),iconareay+iconareah/2,iconareah/3,GREY);
}

void drawTheMoon()
{
    tft.fillCircle(iconareacx+(bmpw/2),iconareay+iconareah/2,iconareah/3,GREY);
    tft.fillCircle(iconareacx+75,iconareay+38,iconareah/3,BLACK);
}

void drawCloud()
{
     tft.drawBitmap(iconareacx,iconareay,cloud,bmpw,bmph,GREY);
}

void drawCloudWithSun()
{
     tft.fillCircle(iconareacx+73,iconareay+34,20,YELLOW);
     drawCloud();
     tft.drawBitmap(iconareacx,iconareay+4,cloud,bmpw,bmph,GREY);
}

void drawLightRainWithSunOrMoon()
{
  if(night)
  {
    drawCloudTheMoonAndRain();
  }else
  {
    drawCloudSunAndRain();
  }
}

void drawLightRain()
{
     drawCloud();
     tft.fillRoundRect(iconareacx+50, iconareay+70, 3, 13, 1, BLUE);
     tft.fillRoundRect(iconareacx+65, iconareay+70, 3, 13, 1, BLUE);
     tft.fillRoundRect(iconareacx+80, iconareay+70, 3, 13, 1, BLUE);
}

void drawModerateRain()
{
     drawCloud();
     tft.fillRoundRect(iconareacx+50, iconareay+70, 3, 15, 1, BLUE);
     tft.fillRoundRect(iconareacx+57, iconareay+67, 3, 15, 1, BLUE);
     tft.fillRoundRect(iconareacx+65, iconareay+70, 3, 15, 1, BLUE);
     tft.fillRoundRect(iconareacx+72, iconareay+67, 3, 15, 1, BLUE);
     tft.fillRoundRect(iconareacx+80, iconareay+70, 3, 15, 1, BLUE);
}

void drawHeavyRain()
{
     drawCloud();
     tft.fillRoundRect(iconareacx+43, iconareay+67, 3, 15, 1, BLUE);
     tft.fillRoundRect(iconareacx+50, iconareay+70, 3, 15, 1, BLUE);
     tft.fillRoundRect(iconareacx+57, iconareay+67, 3, 15, 1, BLUE);
     tft.fillRoundRect(iconareacx+65, iconareay+70, 3, 15, 1, BLUE);
     tft.fillRoundRect(iconareacx+72, iconareay+67, 3, 15, 1, BLUE);
     tft.fillRoundRect(iconareacx+80, iconareay+70, 3, 15, 1, BLUE);
     tft.fillRoundRect(iconareacx+87, iconareay+67, 3, 15, 1, BLUE);
}

void drawThunderstorm()
{
     tft.drawBitmap(iconareacx,iconareay+5,thunder,bmpw,bmph,YELLOW);
     drawCloud();
     tft.fillRoundRect(iconareacx+48, iconareay+67, 3, 15, 1, BLUE);
     tft.fillRoundRect(iconareacx+55, iconareay+67, 3, 15, 1, BLUE);
     tft.fillRoundRect(iconareacx+74, iconareay+67, 3, 15, 1, BLUE);
     tft.fillRoundRect(iconareacx+82, iconareay+67, 3, 15, 1, BLUE);
}

void drawLightSnowfall()
{
     drawCloud();
     tft.fillCircle(iconareacx+50, iconareay+67, 3, GREY);
     tft.fillCircle(iconareacx+65, iconareay+70, 3, GREY);
     tft.fillCircle(iconareacx+82, iconareay+67, 3, GREY);
}

void drawModerateSnowfall()
{
     drawCloud();
     tft.fillCircle(iconareacx+50, iconareay+70, 3, GREY);
     tft.fillCircle(iconareacx+50, iconareay+80, 3, GREY);
     tft.fillCircle(iconareacx+65, iconareay+73, 3, GREY);
     tft.fillCircle(iconareacx+65, iconareay+83, 3, GREY);
     tft.fillCircle(iconareacx+82, iconareay+70, 3, GREY);
     tft.fillCircle(iconareacx+82, iconareay+80, 3, GREY);
}

void drawHeavySnowfall()
{
     drawCloud();
     tft.fillCircle(iconareacx+40, iconareay+70, 3, GREY);
     tft.fillCircle(iconareacx+52, iconareay+70, 3, GREY);
     tft.fillCircle(iconareacx+52, iconareay+80, 3, GREY);
     tft.fillCircle(iconareacx+65, iconareay+73, 3, GREY);
     tft.fillCircle(iconareacx+65, iconareay+83, 3, GREY);
     tft.fillCircle(iconareacx+80, iconareay+70, 3, GREY);
     tft.fillCircle(iconareacx+80, iconareay+80, 3, GREY);
     tft.fillCircle(iconareacx+92, iconareay+70, 3, GREY);     
}

void drawCloudSunAndRain()
{
     tft.fillCircle(iconareacx+73,iconareay+35,20,YELLOW);
     tft.drawBitmap(iconareacx,iconareay,cloud,bmpw,bmph,BLACK);
     tft.drawBitmap(iconareacx,iconareay+3,cloud,bmpw,bmph,GREY);
     tft.fillRoundRect(iconareacx+50, iconareay+73, 3, 13, 1, BLUE);
     tft.fillRoundRect(iconareacx+65, iconareay+73, 3, 13, 1, BLUE);
     tft.fillRoundRect(iconareacx+80, iconareay+73, 3, 13, 1, BLUE);
}

void drawCloudAndTheMoon()
{
     tft.fillCircle(iconareacx+94,iconareay+25,18,GREY);
     tft.fillCircle(iconareacx+105,iconareay+18,18,BLACK);
     tft.drawBitmap(iconareacx,iconareay,cloud,bmpw,bmph,BLACK);
     tft.drawBitmap(iconareacx,iconareay+3,cloud,bmpw,bmph,GREY);
}

void drawCloudTheMoonAndRain()
{
     tft.fillCircle(iconareacx+94,iconareay+28,18,GREY);
     tft.fillCircle(iconareacx+105,iconareay+21,18,BLACK);
     tft.drawBitmap(iconareacx,iconareay,cloud,bmpw,bmph,BLACK);
     tft.drawBitmap(iconareacx,iconareay+3,cloud,bmpw,bmph,GREY);
     tft.fillRoundRect(iconareacx+50, iconareay+73, 3, 11, 1, BLUE);
     tft.fillRoundRect(iconareacx+65, iconareay+73, 3, 11, 1, BLUE);
     tft.fillRoundRect(iconareacx+80, iconareay+73, 3, 11, 1, BLUE);
}

void drawWind()
{  
     tft.drawBitmap(iconareacx,iconareay,wind,bmpw,bmph,GREY);   
}

void drawFog()
{
  tft.fillRoundRect(iconareacx+30, iconareay+25, 55, 4, 1, GREY);
  tft.fillRoundRect(iconareacx+10, iconareay+35, 90, 4, 1, GREY);
  tft.fillRoundRect(iconareacx+20, iconareay+45, 100, 4, 1, GREY);
  tft.fillRoundRect(iconareacx+5, iconareay+55, 90, 4, 1, GREY);
  tft.fillRoundRect(iconareacx+20, iconareay+65, 80, 4, 1, GREY);
}

void clearIcon()
{
     tft.fillRect(iconareacx,iconareay,bmpw,bmph,BLACK);
}

void drawAll()
{
    drawTheSun();
    delay(1000);
    clearIcon();

    drawTheFullMoon();
    delay(1000);
    clearIcon();
    
    drawTheMoon();
    delay(1000);
    clearIcon();
    
    drawCloud();
    delay(1000);
    clearIcon();

    drawCloudWithSun();
    delay(1000);
    clearIcon();
    
    drawLightRain();
    delay(1000);
    clearIcon();

    drawModerateRain();
    delay(1000);
    clearIcon();

    drawHeavyRain();
    delay(1000);
    clearIcon();

    drawThunderstorm();
    delay(1000);
    clearIcon();

    drawLightSnowfall();
    delay(1000);
    clearIcon();

    drawModerateSnowfall();
    delay(1000);
    clearIcon();

    drawHeavySnowfall();
    delay(1000);
    clearIcon();

    drawCloudSunAndRain();
    delay(1000);
    clearIcon();

    drawCloudAndTheMoon();
    delay(1000);
    clearIcon();

    drawCloudTheMoonAndRain();
    delay(1000);
    clearIcon();

    drawWind();
    delay(1000);
    clearIcon();

    drawFog();
    delay(1000);
    clearIcon();
}



