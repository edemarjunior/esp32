//This example code is in the Public Domain (or CC0 licensed, at your option.)
//By Evandro Copercini - 2018
//
//This example creates a bridge between Serial and Classical Bluetooth (SPP)
//and also demonstrate that SerialBT have the same functionalities of a normal Serial

#include "BluetoothSerial.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
uint8_t ledPin = 16; // 
#include "esp_wpa2.h"
#include "SSD1306.h"
SSD1306 display(0x3c, 5, 4); 

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

BluetoothSerial SerialBT;

void setup() {
  Serial.begin(115200);
  SerialBT.begin("ESP32test"); //Bluetooth device name
  Serial.println("The device started, now you can pair it with bluetooth!");
  display.init(); // initialise the OLED
    display.flipScreenVertically(); // does what is says
    display.setFont(ArialMT_Plain_10); // does what is says
    // Set the origin of text to top left
    display.setTextAlignment(TEXT_ALIGN_CENTER_BOTH);
    Serial.begin(115200);
}

void printDeviceAddress() {
  Serial.println("Teste2");
  const uint8_t* point = esp_bt_dev_get_address();
  Serial.println("Teste3");
  for (int i = 0; i < 6; i++) {
 
    char str[3];
 
    sprintf(str, "%02X", (int)point[i]);
    Serial.print(str);
 
    if (i < 5){
      Serial.print(":");
    }
 
  }
}

void loop() {
  display.drawString(50, 10, "teste");
  
    display.display();
    display.clear();
  while(true){
    String frase = SerialBT.readString();
    Serial.println("Teste1");
    bool teste ;
   teste = SerialBT.find("38:9A:F6:6B:FD:8C");
   pinMode(ledPin, teste);
   if(SerialBT.find("38:9A:F6:6B:FD:8C")){
    Serial.println("entrou");
     pinMode(ledPin, OUTPUT);
   }
   Serial.println(teste);
   
    display.drawString(50, 30, frase);
    display.display();
    display.clear();
  }
}
