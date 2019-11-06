#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <string>
#include "WiFi.h"
#include <HTTPClient.h>
#include <BLEAdvertisedDevice.h> 

uint8_t ledPin = 16;
const char* ssid = "AndroidAP";
const char* password =  "29110709";
BLEScan* pBLEScan;
String response;
String teste;
uint8_t count = 0;   
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {
          Serial.printf("Advertised Device: %s \n", advertisedDevice.toString().c_str());
          
          
    }
};

void connectWifi(){

  WiFi.begin(ssid, password); 
 int c = 0;
  while (WiFi.status() != WL_CONNECTED) { //Check for the connection
    delay(1000);
    c++;
    Serial.println("Connecting to WiFi..");
    if(c > 10){
      WiFi.begin(ssid, password); 
    }
  }
  
}

void connectBle(){
  BLEDevice::init("");
  BLEDevice::setPower(ESP_PWR_LVL_P7);
  pBLEScan = BLEDevice::getScan(); //create new scan
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true); //active scan uses more power, but get results faster
}

String getToken(){
  HTTPClient http;
Serial.println(11);
  if(WiFi.status()== WL_CONNECTED){   //Check WiFi connection status
  Serial.println(22);
   http.begin("http://192.168.43.125:8080/WebServiceRest/rest/service/getAut");  //Specify destination for HTTP request
   http.addHeader("Content-Type", "application/json");             //Specify content-type header
 Serial.println(33);
   http.POST(""); 
   Serial.println(44);//Send the actual POST request
   Serial.println(ESP.getFreeHeap());
   delay(5000);
  http.end();  //Free resources
   delay(5000);
   
 }else{
    Serial.println("Error in WiFi connection"); 
 }
  
}

String gerarPresenca(){
  HTTPClient http;
  if(WiFi.status()== WL_CONNECTED){   //Check WiFi connection status

   http.begin("http://192.168.1.107:8080/WebServiceRest/rest/service/registraPresenca");  //Specify destination for HTTP request
   http.addHeader("Content-Type", "application/json");             //Specify content-type header
    Serial.print("GERAR PRESENÇA");
   int httpResponseCode = http.POST(teste);   //Send the actual POST request
   Serial.println(httpResponseCode);
   if(httpResponseCode>0){
   }else{
    Serial.print("Error on sending POST: ");
    Serial.println(httpResponseCode);
   }
   Serial.println("teste 1");
   http.end();  //Free resources
   Serial.println("teste 5");
 }else{
    Serial.println("Error in WiFi connection"); 
 }
  
}

void setup() {
  delay(10);
  Serial.begin(115200);
}

void loop() {
  while(true){
     connectWifi();
    Serial.println("teste inicio");
    getToken();
    WiFi.mode(WIFI_OFF);
    //count = 0; 
    //teste = "";
    //pBLEScan->start(15);
  
    teste = teste + "}";
    Serial.println(teste); 
    //pBLEScan->clearResults();
    //pBLEScan->stop();
    //gerarPresenca();
    Serial.println("teste 2");
    Serial.println("teste 3");
    delay(4000);
  }
   //Send a request every 10 seconds
}
