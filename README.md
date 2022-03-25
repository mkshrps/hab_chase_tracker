# hab_chase_tracker
ESP8266/Esp32 HabHub wifi enabled chase car tracker 
Designed for ESP32 as an alternative to Arduino or Pi this HAB receiver uses Lora for reception (no RTTY). Decodes GPS position etc and retransmits HABHUUB so can be used as a habhub tracker for chase car or base station use. 
It should work with any version of ESP32 board and the pin settings are easily identified within the code (circa line 30) and just need to be adapted for your board variant.
PITS compatible just set the pits mode within code 
outputs information to a minature oled display
will auto tune frequency using lora frrequency error detection assuming the tx and rx are within the lora mode bandwidth. 

