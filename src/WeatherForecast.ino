   //////////////////////////////////////////////
  //    Art Deco Weather Forecast Display     //
 //                                          //
//           http://www.educ8s.tv           //
/////////////////////////////////////////////

#include <Wire.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <Adafruit_ST7735.h>
#include <Adafruit_GFX.h>
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

//  #define TFT_CS         0   // was 4
//  #define TFT_RST        2   // was 16                                          
//  #define TFT_DC         4   // was 5

#define cs   0  
#define dc   4   
#define rst  2 

const char* ssid     = "Buschfunk";      // SSID of local network
const char* password = "FritzBoxIstTotalSuper";   // Password on network
String APIKEY = "fb1d7728528b56504cb6af0aba6c6fbc";
String CityID = "2885397"; //Sparta, Greece
//int TimeZone = 1; //GMT +2
  Timezone myTZ;


WiFiClient client;
char servername[]="api.openweathermap.org";  // remote server we will connect to

boolean night = false;
int  counter = 360;
String weatherDescription ="";
String weatherLocation = "";
float Temperature;

extern  unsigned char  cloud[];
extern  unsigned char  thunder[];
extern  unsigned char  wind[];

Adafruit_ST7735 tft = Adafruit_ST7735(cs, dc, rst);

void setup() {
  Serial.begin(115200);
  tft.initR(INITR_MINI160x80);   // initialize a ST7735S chip, black tab
  tft.fillScreen(BLACK);

  Serial.println("Connecting");
  WiFi.begin(ssid, password);

  tft.setCursor(10,80);
  tft.setTextColor(WHITE);
  tft.setTextSize(1);
  tft.print("Connecting...");

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
}

void loop() {

    if(counter == 360) //Get new data every 30 minutes
    {
      counter = 0;
      getWeatherData();
      
    }else
    {
      counter++;
      delay(5000);
      Serial.println(counter); 
    }
}

void getWeatherData() //client function to send/receive GET request data.
{

  HTTPClient http;
  http.useHTTP10(true);

  http.begin(client, "http://api.openweathermap.org/data/2.5/forecast?id="+CityID+"&units=metric&cnt=1&APPID="+APIKEY);
  
  http.GET();

  //char result[]="{\"cod\":\"200\",\"message\":0,\"cnt\":1,\"list\":[{\"dt\":1586034000,\"main\":{\"temp\":37.1,\"feels_like\":2.87,\"temp_min\":7.1,\"temp_max\":7.71,\"pressure\":1025,\"sea_level\":1025,\"grnd_level\":1019,\"humidity\":61,\"temp_kf\":-0.61},\"weather\":[{\"id\":800,\"main\":\"Clear\",\"description\":\"clear sky\",\"icon\":\"01n\"}],\"clouds\":{\"all\":0},\"wind\":{\"speed\":3.23,\"deg\":126},\"sys\":{\"pod\":\"n\"},\"dt_txt\":\"2020-04-04 21:00:00\"}],\"city\":{\"id\":2885397,\"name\":\"Korschenbroich\",\"coord\":{\"lat\":51.1914,\"lon\":6.5135},\"country\":\"DE\",\"timezone\":7200,\"sunrise\":1585976515,\"sunset\":1586023881}}";

// http://api.openweathermap.org/data/2.5/forecast?id=2885397&units=metric&cnt=1&APPID=fb1d7728528b56504cb6af0aba6c6fbc

  // char result[700]=" ";
  // char *ch;

  // Serial.println("Getting Weather Data");
  // if (client.connect(servername, 80)) {  //starts client connection, checks for connection
  //   client.println("GET /data/2.5/forecast?id="+CityID+"&units=metric&cnt=1&APPID="+APIKEY);
  //   client.println("Host: api.openweathermap.org");
  //   client.println("User-Agent: ArduinoWiFi/1.1");
  //   client.println("Connection: close");
  //   client.println();
  // } 
  // else {
  //   Serial.println("connection failed"); //error message if no client connect
  //   Serial.println();
  // }

  // while(client.connected() && !client.available()) delay(1); //waits for data
 
  //   Serial.println("Waiting for data");

  // while (client.connected() || client.available()) { //connected or data available
  //   char c = client.read(); //gets byte from ethernet buffer
  //   ch = c;
  //   //strcpy(ch, c);
  //   strcat (result, ch);
  //     //result = result+c;
  //   }

  // client.stop(); //stop client

  StaticJsonDocument<1024> root;

// Deserialize the JSON document
//  DeserializationError error = deserializeJson(root, result);
  DeserializationError error = deserializeJson(root, http.getStream());
  // Test if parsing succeeds.
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.c_str());
    return;
  }
  http.end();

  JsonObject list_0 = root["list"][0];
  JsonObject main =  list_0["main"];
  JsonArray weather_arr = list_0["weather"];
  JsonObject weather_0 = weather_arr[0];


  const char* location = root["city"]["name"];
  float temperature = main["temp"];
  const char* weather = weather_0["main"];
  const char* description = weather_0["description"];
  int weatherID = weather_0["id"];
  long dt_utc = list_0["dt"];

  // convert UTC timestamp to local time
  long dt_local = myTZ.tzTime(dt_utc, UTC_TIME);
  String timeS = dateTime(dt_local, "H:i");

  // limiting Temperature to 1 decimal place
  char tempstr[3];
  dtostrf(temperature,2,1, tempstr);

  Serial.println(location);
  Serial.println(tempstr);

  Serial.println(weather);
  Serial.println(description);
  Serial.println(weatherID);

  Serial.println(timeS);

  clearScreen();

  //int weatherID = idString.toInt();
  printData(timeS,tempstr, weatherID);

}

void printData(String timeString, char* tempstr, int weatherID)
{
  tft.setCursor(10,20);
  tft.setTextColor(WHITE);
  tft.setTextSize(2);
  tft.print(timeString);

  printWeatherIcon(weatherID);

  tft.setCursor(10,132);
  tft.setTextColor(WHITE);
  tft.setTextSize(2);
  tft.print(tempstr);

  tft.setCursor(55,130);
  tft.setTextColor(WHITE);
  tft.setTextSize(1);
  tft.print("o");
  tft.setCursor(60,132);
  tft.setTextColor(WHITE);
  tft.setTextSize(2);
  tft.print("C");
}

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
  case 771: drawFog(); break;
  case 781: drawFog(); break;

  default:break; 
 }
}

// String convertGMTTimeToLocal(String timeS)
// {
//  int length = timeS.length();
//  timeS = timeS.substring(length-8,length-6);
//  int time = timeS.toInt();
//  time = time+TimeZone;

//  if(time > 21 ||  time<7)
//  {
//   night=true;
//  }else
//  {
//   night = false;
//  }
//  timeS = String(time)+":00";
//  return timeS;
// }


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
    tft.fillCircle(tft.width()/2,tft.height()/2,tft.width()/3,YELLOW);
}

void drawTheFullMoon()
{
    tft.fillCircle(64,80,26,GREY);
}

void drawTheMoon()
{
    tft.fillCircle(64,80,26,GREY);
    tft.fillCircle(75,73,26,BLACK);
}

void drawCloud()
{
     tft.drawBitmap(0,35,cloud,128,90,GREY);
}

void drawCloudWithSun()
{
     tft.fillCircle(73,70,20,YELLOW);
     tft.drawBitmap(0,36,cloud,128,90,BLACK);
     tft.drawBitmap(0,40,cloud,128,90,GREY);
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
     tft.drawBitmap(0,35,cloud,128,90,GREY);
     tft.fillRoundRect(50, 105, 3, 13, 1, BLUE);
     tft.fillRoundRect(65, 105, 3, 13, 1, BLUE);
     tft.fillRoundRect(80, 105, 3, 13, 1, BLUE);
}

void drawModerateRain()
{
     tft.drawBitmap(0,35,cloud,128,90,GREY);
     tft.fillRoundRect(50, 105, 3, 15, 1, BLUE);
     tft.fillRoundRect(57, 102, 3, 15, 1, BLUE);
     tft.fillRoundRect(65, 105, 3, 15, 1, BLUE);
     tft.fillRoundRect(72, 102, 3, 15, 1, BLUE);
     tft.fillRoundRect(80, 105, 3, 15, 1, BLUE);
}

void drawHeavyRain()
{
     tft.drawBitmap(0,35,cloud,128,90,GREY);
     tft.fillRoundRect(43, 102, 3, 15, 1, BLUE);
     tft.fillRoundRect(50, 105, 3, 15, 1, BLUE);
     tft.fillRoundRect(57, 102, 3, 15, 1, BLUE);
     tft.fillRoundRect(65, 105, 3, 15, 1, BLUE);
     tft.fillRoundRect(72, 102, 3, 15, 1, BLUE);
     tft.fillRoundRect(80, 105, 3, 15, 1, BLUE);
     tft.fillRoundRect(87, 102, 3, 15, 1, BLUE);
}

void drawThunderstorm()
{
     tft.drawBitmap(0,40,thunder,128,90,YELLOW);
     tft.drawBitmap(0,35,cloud,128,90,GREY);
     tft.fillRoundRect(48, 102, 3, 15, 1, BLUE);
     tft.fillRoundRect(55, 102, 3, 15, 1, BLUE);
     tft.fillRoundRect(74, 102, 3, 15, 1, BLUE);
     tft.fillRoundRect(82, 102, 3, 15, 1, BLUE);
}

void drawLightSnowfall()
{
     tft.drawBitmap(0,30,cloud,128,90,GREY);
     tft.fillCircle(50, 100, 3, GREY);
     tft.fillCircle(65, 103, 3, GREY);
     tft.fillCircle(82, 100, 3, GREY);
}

void drawModerateSnowfall()
{
     tft.drawBitmap(0,35,cloud,128,90,GREY);
     tft.fillCircle(50, 105, 3, GREY);
     tft.fillCircle(50, 115, 3, GREY);
     tft.fillCircle(65, 108, 3, GREY);
     tft.fillCircle(65, 118, 3, GREY);
     tft.fillCircle(82, 105, 3, GREY);
     tft.fillCircle(82, 115, 3, GREY);
}

void drawHeavySnowfall()
{
     tft.drawBitmap(0,35,cloud,128,90,GREY);
     tft.fillCircle(40, 105, 3, GREY);
     tft.fillCircle(52, 105, 3, GREY);
     tft.fillCircle(52, 115, 3, GREY);
     tft.fillCircle(65, 108, 3, GREY);
     tft.fillCircle(65, 118, 3, GREY);
     tft.fillCircle(80, 105, 3, GREY);
     tft.fillCircle(80, 115, 3, GREY);
     tft.fillCircle(92, 105, 3, GREY);     
}

void drawCloudSunAndRain()
{
     tft.fillCircle(73,70,20,YELLOW);
     tft.drawBitmap(0,32,cloud,128,90,BLACK);
     tft.drawBitmap(0,35,cloud,128,90,GREY);
     tft.fillRoundRect(50, 105, 3, 13, 1, BLUE);
     tft.fillRoundRect(65, 105, 3, 13, 1, BLUE);
     tft.fillRoundRect(80, 105, 3, 13, 1, BLUE);
}

void drawCloudAndTheMoon()
{
     tft.fillCircle(94,60,18,GREY);
     tft.fillCircle(105,53,18,BLACK);
     tft.drawBitmap(0,32,cloud,128,90,BLACK);
     tft.drawBitmap(0,35,cloud,128,90,GREY);
}

void drawCloudTheMoonAndRain()
{
     tft.fillCircle(94,60,18,GREY);
     tft.fillCircle(105,53,18,BLACK);
     tft.drawBitmap(0,32,cloud,128,90,BLACK);
     tft.drawBitmap(0,35,cloud,128,90,GREY);
     tft.fillRoundRect(50, 105, 3, 11, 1, BLUE);
     tft.fillRoundRect(65, 105, 3, 11, 1, BLUE);
     tft.fillRoundRect(80, 105, 3, 11, 1, BLUE);
}

void drawWind()
{  
     tft.drawBitmap(0,35,wind,128,90,GREY);   
}

void drawFog()
{
  tft.fillRoundRect(45, 60, 40, 4, 1, GREY);
  tft.fillRoundRect(40, 70, 50, 4, 1, GREY);
  tft.fillRoundRect(35, 80, 60, 4, 1, GREY);
  tft.fillRoundRect(40, 90, 50, 4, 1, GREY);
  tft.fillRoundRect(45, 100, 40, 4, 1, GREY);
}

void clearIcon()
{
     tft.fillRect(0,40,128,100,BLACK);
}






