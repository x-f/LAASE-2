/*
  LAASE-2
  x-f (x-f@people.lv), 2012-2013
    
  TODO
    GPS eco mode?
*/

// GPS pieslēgts vai nu D2 un D3 (SoftwareSerial), vai D0 un D1 (UART)
#define GPS_HW_SERIAL true
#define DEBUG false
 
#include <string.h>
#include <util/crc16.h>
#include <avr/wdt.h>
#include <Wire.h>
#include <TMP102.h> 

// Stalker temperatūras sensors
int tmp102Address = 0x48;

#include <TinyGPS_UBX.h>
TinyGPS gps;
// melns (2, GND), oranžs (3, VCC), dzeltens (5, RX), pelēks (6, TX)
// oranžbalts (GND), oranžs (3V3), zaļibalts (TX), zaļš (RX)
#if !GPS_HW_SERIAL
  #include <SoftwareSerial.h>
  SoftwareSerial GPS_Serial(2, 3);
#endif

#define PIN_radio 6

// LED for GPS status
//#define PIN_statusled 15

// buzzer EN pin
#define PIN_buzzer 15


#include <OneWire.h>
#include <DallasTemperature.h>
#define ONE_WIRE_BUS 2
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature DS18B20(&oneWire);
DeviceAddress ThermometerAddr_bat = { 0x28, 0x14, 0x24, 0xBB, 0x03, 0x00, 0x00, 0x9F };
DeviceAddress ThermometerAddr_ext = { 0x28, 0x5A, 0x13, 0xBB, 0x03, 0x00, 0x00, 0x2C };


#include <AmbientLightSensor.h>
AmbientLightSensor UV_sensor1(A2); // garais uz GND - UV
AmbientLightSensor UV_sensor2(A3); // garais uz GND - violets
#define UV_sensor_max1 24 // ja 0, tad mēra digitāli
#define UV_sensor_max2 2 // ja 0, tad mēra digitāli
// UV (violets) 16, UV (uv) 512


#include <PString.h>
static char datastring[110];
PString str(datastring, sizeof(datastring));

int count = 0;
byte gps_hour, gps_minute, gps_second;
long gps_lat, gps_lon, gps_alt;
unsigned long gps_fix_age;
boolean gps_has_fix = false;
byte gps_navmode = 99;

int tmp102_temp = 0;
int ds18b20_temp_ext = 0;
int ds18b20_temp_bat = 0;
int UV_sensor1_value, UV_sensor2_value = 0;


// milisekundes, kad programma sāka darboties
unsigned long program_started;
unsigned long timestamp_now;


static char msg_buffer[41]; // garākais ziņojums
prog_char msg0[] PROGMEM = "1, 2, 3, esam gaisa. Sveika, Latvija!"; // 1.5
prog_char msg1[] PROGMEM = "I'm freeeeezing up here!"; // 13
prog_char msg2[] PROGMEM = "Youngsters On The Air, do you copy?"; //23
prog_char msg3[] PROGMEM = "Hello, world! You look amazing today. :)"; //30
prog_char msg4[] PROGMEM = "Woo, re-entry!"; //25
prog_char msg5[] PROGMEM = "Thank you for flying with LAASE, 73 all!"; //7
PROGMEM const char *msgs[] = {msg0, msg1, msg2, msg3, msg4, msg5};
byte msg_id = 0;


void setup() {
  // watchdog
  wdt_disable();
  wdt_enable(WDTO_8S);
  wdt_reset();

  Serial.begin(9600);
  delay(150);
  resetGPS();
  delay(500);
  
  
  //delay(1000);
  GPS_setup();
  
  Wire.begin();

  // Dallas temperatūras sensori
  // Start up the library
  DS18B20.begin();
  // set the resolution to 10 bit (good enough?)
  DS18B20.setResolution(ThermometerAddr_bat, 10);
  DS18B20.setResolution(ThermometerAddr_ext, 10);

  if (UV_sensor_max1 && UV_sensor_max1 != 2000)
    UV_sensor1.setAnalogMeasurement(UV_sensor_max1);
  if (UV_sensor_max2 && UV_sensor_max2 != 2000)
    UV_sensor2.setAnalogMeasurement(UV_sensor_max2);

  pinMode(PIN_radio, OUTPUT);
  digitalWrite(PIN_radio, HIGH);

  //pinMode(PIN_statusled, OUTPUT);
  //digitalWrite(PIN_statusled, LOW);

  // 1 - off, 0 - on
  pinMode(PIN_buzzer, OUTPUT);
  digitalWrite(PIN_buzzer, HIGH);


  program_started = millis();

  #if DEBUG
    Serial.println(F("go"));
  #endif
}

void loop() {
  // watchdog
  wdt_reset();
  
  timestamp_now = millis();
  

  if (count % 15 == 0) {
    //Serial.flush();
    //resetGPS();
    while(Serial.available()) Serial.read();
    delay(500);
    GPS_setup();
    //Serial.flush();
    while(Serial.available()) Serial.read();
  }
    
  GPS_poll();
  read_sensors();

  gps.crack_time(&gps_hour, &gps_minute, &gps_second, &gps_fix_age);
  gps.get_position(&gps_lat, &gps_lon, &gps_fix_age);
  gps_alt = gps.altitude();
  gps_has_fix = gps.has_fix();

  /*
  // statusa LEDs
  //  - ja nav GPS fix, deg
  //  - ja ir GPS fix, iemirgojas
  if (gps_has_fix) {
    for (int i = 0; i < 3; i++) {
      digitalWrite(PIN_statusled, HIGH);
      delay(100);
      digitalWrite(PIN_statusled, LOW);
      delay(50);
    }
  } else {
    digitalWrite(PIN_statusled, HIGH);
  }
  */
  
  // buzzer
  boolean buzzer_on = false;
  if (gps.fix_quality() > 1) {
    if (gps_alt/100.0 < 1000) {
      if (gps_hour >= 19 || gps_hour <= 4)
        // silence (time)
        buzzer_on = false;
      else 
        // beep
        buzzer_on = true;
    } else
      // silence (above alt)
      buzzer_on = false;
  } else
    // beep (no sats)
    buzzer_on = true;
  digitalWrite(PIN_buzzer, !buzzer_on);
  

  char time[8];
  sprintf(time, "%02d:%02d:%02d", gps_hour, gps_minute, gps_second);

  str.begin();
  str.print(F("$$LAASE,"));
  str.print(count);
  str.print(",");
  str.print(time);
  str.print(",");

  str.print(gps_lat/100000.0, 5);
  str.print(",");
  str.print(gps_lon/100000.0, 5);
  str.print(",");
  str.print(gps_alt/100.0, 0);
  str.print(",");

  str.print(gps.speed()/100, DEC); // km/h
  str.print(",");
  str.print(gps.sats(), DEC);
  str.print(",");

  str.print(tmp102_temp, DEC); // minor
  str.print(","); // minor
  str.print(ds18b20_temp_ext, DEC);
  str.print(",");
  str.print(ds18b20_temp_bat, DEC);
  str.print(",");
  str.print(UV_sensor1_value, DEC);
  str.print(",");
  str.print(UV_sensor2_value, DEC);

  //str.print(","); // minor
  //str.print(freeRam(), DEC); // minor


  unsigned int CHECKSUM = gps_CRC16_checksum(datastring);  // Calculates the checksum for this datastring
  char checksum_str[6];
  sprintf(checksum_str, "*%04X\n", CHECKSUM);
  //strcat(datastring, checksum_str);

  // preamble for dl-fldigi to better lock
  rtty_txbyte(0x80);
  rtty_txbyte(0x80);
  rtty_txbyte(0x80);
  rtty_txstring("$");
  rtty_txstring(datastring);
  rtty_txstring(checksum_str);


  // pārējā info, ko nav nepieciešams sūtīt uz zemi
  
  str.print(gps.course()/100, DEC); // minor
  str.print(","); // minor

  str.print(gps.fix_quality(), DEC);
  str.print(",");
  str.print(gps_navmode, DEC);
  str.print(",");
  //str.print(gps_fix_age, DEC); // minor
  //str.print(","); // minor
  //str.print(gps_has_fix, DEC);
  //str.print(",");

  //str.print(tmp102_temp, DEC); // minor

  //str.print(","); // minor
  //str.print(freeRam(), DEC); // minor
  
  
  char msg_txt_id = -1;
  if (gps.fix_quality() > 1) {
    if (
         (msg_id == 0 && gps_alt/100.0 > 1500)
      || (msg_id == 1 && gps_alt/100.0 > 13000)
      || (msg_id == 2 && gps_alt/100.0 > 23000)
      || (msg_id == 3 && gps_alt/100.0 > 30000)
      || (msg_id == 4 && gps_alt/100.0 < 25000)
      || (msg_id == 5 && gps_alt/100.0 < 7000)
    ) {
      msg_txt_id = msg_id++;
    }
    if (msg_txt_id > -1) {
      rtty_txbyte(0x80);
      rtty_txstring(strcpy_P(msg_buffer, (char*)pgm_read_word(&msgs[msg_txt_id])));
      rtty_txstring("\n");
    }
  }
  
  
  #if DEBUG
    //Serial.print(datastring);
  #endif
  
  count++;

}

//-------------------------------------------


/*int freeRam() {
  extern int __heap_start, *__brkval; 
  int v; 
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
}*/

