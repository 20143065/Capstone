#include <SoftwareSerial.h>
#include <ESP8266.h>
#include <Adafruit_BME280.h>


#define SSID        "KMU_CS232"
#define PASSWORD    "cs9104790"
#define HOST_NAME   "d2zz7ru5nmmqs3.cloudfront.net"
#define HOST_PORT   (80)
#define API_KEY     "gyfzID5sLaFEQLg6EuRv802q0rGtuSs1kUOkRXL9"

#define D_NAME "iams7:232_1" /*AirSensor<학번>>*/
#define PM_SENSOR   "232:pm"
#define CO2_SENSOR  "232:co2"
#define TH_SENSOR   "232:th"
#define ROOM_NUM    232

SoftwareSerial Serial_wifi(2, 3); /* RX, TX */
ESP8266 Wifi(Serial_wifi);

unsigned int PM_CF10;
unsigned int PM_CF25;
unsigned int PM_CF100;
unsigned int PM_AT10;
unsigned int PM_AT25;
unsigned int PM_AT100;

Adafruit_BME280 bme; // I2C

void setup() {
  bool status;
  status = bme.begin();  
  
  Serial.begin(9600);
  //Serial.print("setup begin\r\n");

  //Serial.print("FW Version:");
  Serial.println(Wifi.getVersion().c_str());

  if (Wifi.setOprToStationSoftAP()) {
    //Serial.print("to station + softap ok\r\n");
  } else {
    //Serial.print("to station + softap err\r\n");
  }

  if (Wifi.joinAP(SSID, PASSWORD)) {
    //Serial.print("Join AP success\r\n");
    //Serial.print("IP:\r\n");
    Serial.println( Wifi.getLocalIP().c_str());
  } else {
    Serial.print("Join AP failure\r\n");
  }

  if (Wifi.disableMUX()) {
    //Serial.print("single ok\r\n");
  } else {
    //Serial.print("single err\r\n");
  }

  //Serial.print("setup end\r\n");
  //Serial.println("--------------------wait--------------------");
  delay(60000);
  //Serial.println(F("Sensing Start"));
}

void loop() {
  // 이산화탄소
  int co2_sig = A2;
  int co2_analog = 0;
  float co2_volt = 0;
  float co2_conc = 0;

  for(int i=0; i<32; i++){
    co2_analog += analogRead(co2_sig);
  }

  co2_volt = (co2_analog >> 5) * 5.0 * 1000 / 1023.0;
  co2_conc = co2_volt * 0.4;

  //Serial.print(" CO2: ");
  //Serial.print(co2_conc);
  //Serial.print(" ppm\n");

  
  // 미세먼지
  int IDX = 0;
  unsigned char readData;
  unsigned char RAW;

  while( Serial.available() ) {
      readData = Serial.read();
      if( (IDX==0 && readData!=0x42) || (IDX==1 && readData!=0x4d) ) {
        break;
      }
      if( IDX > 15 ) {
        break;
      }
      else if( IDX==4 || IDX==6 || IDX==8 || IDX==10 || IDX==12 || IDX==14 ) {
         RAW = readData;
      } else if( IDX==5 ) {
        PM_CF10 = 256*RAW+readData;
      } else if( IDX==7 ) {
        PM_CF25=256*RAW+readData;
      } else if( IDX==9 ) {
        PM_CF100=256*RAW+readData;
      } else if( IDX==11 ) {
        PM_AT10=256*RAW+readData;
      } else if( IDX==13 ) {
        PM_AT25=256*RAW+readData;
      } else if( IDX==15 ) {
        PM_AT100=256*RAW+readData;
      }
      IDX++;
    }
    
//    Serial.print("\n temp : ");
//    Serial.print(18);
//    Serial.print("\n humid : ");
//    Serial.print(26);
//    Serial.print("\n PM2.5 : ");
//    Serial.print(PM_AT25);
//    Serial.print("\n PM10.0 : ");
//   Serial.print(PM_AT100);
//    Serial.print("\n");

  //uint8_t buffer[128] = {0};

  Serial_wifi.listen();
  Wifi.createTCP(HOST_NAME, HOST_PORT);
  //Serial.print("create tcp ok\r\n");

  char *paramTpl = "?device=%s&th_s=%s&t=%d&h=%d&pm_s=%s&p25=%d&p10=%d&co2_s=%s&co2=%d&room=%d";
  char param[80];
  sprintf(param, paramTpl, D_NAME, TH_SENSOR,int(bme.readTemperature())-2,int(bme.readHumidity()),PM_SENSOR,PM_AT25,PM_AT100,CO2_SENSOR,int(co2_conc),ROOM_NUM);

  // This will send the request to the server
  char *headerTpl = "GET /iams/lecture7%s HTTP/1.1\r\n"
                    "Host: %s\r\n"
                    "x-api-key: %s\r\n"
                    "Cache-Control: no-cache\r\n"
                    "User-Agent: KMU-IoT/%s\r\n\r\n";
  char header[200];
  sprintf(header, headerTpl, param, HOST_NAME, API_KEY, D_NAME);
  //Serial.println(header);
  Wifi.send((const uint8_t*)header, strlen(header));

  //Serial.println("——————————————————————");

  delay(60000);
}
