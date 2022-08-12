

#ifdef _DISPLAY_
// SSD1306Wire display(OLED_I2C_ADDRESS, SDA, SCL);

#else
// extern SSD1306Wire display;// ADDRESS, SDA, SCL  -  SDA and SCL usually populate automatically based on your board's pins_arduino.h e.g. https://github.com/esp8266/Arduino/blob/master/variants/nodemcu/pins_arduino.h
void display_gps();
#endif
