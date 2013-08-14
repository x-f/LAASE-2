
void GPS_setup() {
  #if !GPS_HW_SERIAL
    GPS_Serial.begin(9600);
  #else
    Serial.begin(9600);
  #endif
  delay(200);

  #if !GPS_HW_SERIAL
    GPS_Serial.flush();
  #else
    Serial.flush();
  #endif
  delay(100);
  
  // izslēdz visus GPS NMEA teikumus uBlox GPS modulim
  // ZDA, GLL, VTG, GSV, GSA, GGA, RMC
  // https://github.com/thecraag/craag-hab/blob/master/CRAAG1/code/CRAAG1c/CRAAG1c.ino
  // Turning off all GPS NMEA strings apart on the uBlox module
  // Taken from Project Swift (rather than the old way of sending ascii text)
  uint8_t setNMEAoff[] = {0xB5, 0x62, 0x06, 0x00, 0x14, 0x00, 0x01, 0x00, 0x00, 0x00, 0xD0, 0x08, 0x00, 0x00, 0x80, 0x25, 0x00, 0x00, 0x07, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0xA0, 0xA9};
  sendUBX(setNMEAoff, sizeof(setNMEAoff)/sizeof(uint8_t));
  
  delay(200);
  #if !GPS_HW_SERIAL
    GPS_Serial.flush();
  #else
    Serial.flush();
  #endif
  
  // airborne
  // ..
  // Set the navigation mode (Airborne, 1G)
  //Serial.print("Setting uBlox nav mode: ");
  uint8_t setNav[] = {0xB5, 0x62, 0x06, 0x24, 0x24, 0x00, 0xFF, 0xFF, 0x06, 0x03, 0x00, 0x00, 0x00, 0x00, 0x10, 0x27, 0x00, 0x00, 0x05, 0x00, 0xFA, 0x00, 0xFA, 0x00, 0x64, 0x00, 0x2C, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x16, 0xDC};
  sendUBX(setNav, sizeof(setNav)/sizeof(uint8_t));
  //getUBX_ACK(setNav);
  delay(1000);

  //uint8_t setEco[] = {0xB5, 0x62, 0x06, 0x11, 0x02, 0x00, 0x08, 0x04, 0x25, 0x95};
  //uint8_t setEco[] = {0xB5, 0x62, 0x06, 0x11, 0x02, 0x00, 0x00, 0x04, 0x1D, 0x85};
  //uint8_t setEco[] = {0xB5, 0x62, 0x06, 0x11, 0x02, 0x00, 0x08, 0x01, 0x22, 0x92};
  //sendUBX(setEco, sizeof(setEco)/sizeof(uint8_t));
  //getUBX_ACK(setEco);

  #if DEBUG
    Serial.println(F("GPS setup done"));
  #endif
}


boolean GPS_poll() {
  //Poll GPS
  #if !GPS_HW_SERIAL
    GPS_Serial.flush();
    GPS_Serial.println(F("$PUBX,00*33"));
  #else
    //Serial.flush();
    while(Serial.available()) Serial.read();
    Serial.println(F("$PUBX,00*33"));
  #endif
    delay(100);

    unsigned long starttime = millis();
    while (true) {
      #if !GPS_HW_SERIAL
      if (GPS_Serial.available()) {
        char c = GPS_Serial.read();
      #else
      if (Serial.available()) {
        char c = Serial.read();
      #endif
        if (gps.encode(c))
          return true;
      }
      // 
      if (millis() - starttime > 1000) {
        #if DEBUG
          Serial.println(F("timeout"));
        #endif
        break;
      }
    }
  return false;
}

void resetGPS() {
  uint8_t set_reset[] = {
    0xB5, 0x62, 0x06, 0x04, 0x04, 0x00, 0xFF, 0x87, 0x00, 0x00, 0x94, 0xF5
  };
  sendUBX(set_reset, sizeof(set_reset)/sizeof(uint8_t));
}

// http://ukhas.org.uk/guides:falcom_fsa03#sample_code
// Send a byte array of UBX protocol to the GPS
void sendUBX(uint8_t *MSG, uint8_t len) {
  for(int i=0; i<len; i++) {
    #if !GPS_HW_SERIAL
      GPS_Serial.write(MSG[i]);
    #else
      Serial.write(MSG[i]);
    #endif
  }
}

