
void read_sensors() {

  // stalker temperatūras sensors
  tmp102_temp = Tmp102.getTemperatureInCelsius() * 10;

   // Dallas temperatūras sensori
  DS18B20.requestTemperatures();
  ds18b20_temp_ext = DS18B20.getTempC(ThermometerAddr_ext) * 10;
  ds18b20_temp_bat = DS18B20.getTempC(ThermometerAddr_bat) * 10;

  UV_sensor1_value = UV_sensor1.measure();
  UV_sensor2_value = UV_sensor2.measure();
}


// http://bildr.org/2011/01/tmp102-arduino/
float tmp102_getTemperature(){
  Wire.requestFrom(tmp102Address,2); 

  byte MSB = Wire.read();
  byte LSB = Wire.read();

  //it's a 12bit int, using two's compliment for negative
  int TemperatureSum = ((MSB << 8) | LSB) >> 4; 

  float celsius = TemperatureSum*0.0625;
  return celsius;
}

