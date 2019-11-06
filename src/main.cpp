#include <BLEDevice.h>
#include <Preferences.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <string>
#include <HTTPClient.h>
#include <BLEAdvertisedDevice.h>         
#include <WiFi.h>      
#include <DNSServer.h>
#include <WebServer.h>
#include <WiFiManager.h>
#include <FS.h>
#include <sPIFFS.h>
#include <ArduinoJson.h>

BLEScan* pBLEScan;
String response;
String json;
uint8_t count = 0;
HTTPClient http; 
WiFiManager wifi;
StaticJsonDocument<600> doc;
 
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {
          Serial.printf("Dispositivos encontrados: %s \n", advertisedDevice.toString().c_str());
          if(advertisedDevice.getName().c_str() != ""){
          count++;
          if(json != ""){
            json = json + ",";
          }else{
            json = json + "{";
          }
          
          json = json + "\"" + count + "\":{";
          json = json + "\"token\":\"" + response + "\",";
          json = json + "\"mac\":\"" + advertisedDevice.getAddress().toString().c_str() + "\",";
          json = json + "\"aluno\":\"" +  advertisedDevice.getName().c_str() + "\",";
          json = json + "\"esp\":\"" +  "esp2" + "\"}";
          }          
    }
};

void connectWifi(){
  const char* senha = doc["ssid"];
  const char* id = doc["senha"];
  WiFi.begin(id, senha); 

  while (WiFi.status() != WL_CONNECTED) { 
    delay(10);
    Serial.println("Conectando WIFI");
  } 
}

void connectBle(){
  BLEDevice::init("");
  BLEDevice::setPower(ESP_PWR_LVL_P7);
  pBLEScan = BLEDevice::getScan(); 
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true); 
}

void getToken(){
  if(WiFi.status()== WL_CONNECTED){ 
   http.begin("http://192.168.1.107:8080/WebServiceRest/rest/service/getAut"); 
   http.addHeader("Content-Type", "application/json");  
   int httpResponseCode = http.POST("");
   if(httpResponseCode>0){
    response = http.getString(); 
   }else{
    Serial.print("Erro, código: ");
    Serial.println(httpResponseCode);
   }

   http.end();
 }else{
    Serial.println("Erro, sem conexão."); 
    connectWifi();
 }
}

void gerarPresenca(){

  if(WiFi.status()== WL_CONNECTED){ 

   http.begin("http://192.168.1.107:8080/WebServiceRest/rest/service/registraPresenca");
   http.addHeader("Content-Type", "application/json");
   int httpResponseCode = http.POST(json); 
   
   if(httpResponseCode>0){
   }else{
    Serial.print("Erro, código: ");
    Serial.println(httpResponseCode);
   }
   http.end();
 }else{
    Serial.println("Erro, sem conexão"); 
    connectWifi();
 }
  
}

void lerArquivo(fs::FS &fs, const char * path){
    Serial.printf("Lendo arquivo: %s\n", path);

    File file = fs.open(path);
    if(!file || file.isDirectory()){
        Serial.println("Erro ao abrir o arquivo");
        return;
    }

    Serial.print("informações do arquivo: ");

    while(file.available()){
      size_t size = file.size();
      char buf [600];
      file.readBytes(buf, size);
      Serial.print(buf);
      deserializeJson(doc, buf);
    }
}

void salvarArquivo(fs::FS &fs, const char * path, const char * message){
    Serial.printf("salvando arquivo: %s\n", path);

    File file = fs.open(path, FILE_WRITE);
    if(!file){
        Serial.println("erro ao salvar informação");
        return;
    }
    if(file.print(message)){
        Serial.println("Arquivo salvo");
    } else {
        Serial.println("Informação não salva");
    }
}

int listarArquivos(fs::FS &fs, const char * dirname, uint8_t levels){
    Serial.printf("Listando arquivos e diretorios: %s\n", dirname);

    File root = fs.open(dirname);
    if(!root){
        Serial.println("Erro ao abrir arquivo");
        return 0;
    }
    if(!root.isDirectory()){
        Serial.println("Não é um diretório valido");
        return 0;
    }

    File file = root.openNextFile();
    int c = 0;
    while(file){
      c ++; 
        if(file.isDirectory()){
            Serial.print("  DIR : ");
            Serial.println(file.name());
            if(levels){
                listarArquivos(fs, file.name(), levels -1);
            }
        } else {
            Serial.print("  Arquivo: ");
            Serial.print(file.name());
            Serial.print("  Tamanho: ");
            Serial.println(file.size());
        }
        file = root.openNextFile();
    }
    return c;
}

void setup() {
  delay(10);
  Serial.begin(9600);
  wifi.setBreakAfterConfig(true); 
  wifi.setConnectTimeout(100);
   if(!SPIFFS.begin(true)){
    Serial.println("\nErro ao abrir o sistema de arquivos");
  } else {
    listarArquivos(SPIFFS, "/", 0);
    lerArquivo(SPIFFS, "/config.txt");
    Serial.println("teste1");
    const char* world = doc["identidade"];
    Serial.println(world);
    delay(10000);
  }
  
  if (!wifi.startConfigPortal("Controlador FURB")) {
      doc.clear();
      doc["ssid"] = wifi.getSSID();
      doc["identidade"] = wifi.getIdentidade();
      doc["senha"] = wifi.getPassword();
      doc["esp"] = wifi.getNome();
      doc["sala"] = wifi.getSala();
      doc["bloco"] = wifi.getBloco();
      doc["horaInicialM"] = wifi.getHoraInicialM();
      doc["horaFinalM"] = wifi.getHoraFinalM();
      doc["horaInicialV"] = wifi.getHoraInicialV();
      doc["horaFinalV"] = wifi.getHoraFinalV();
      doc["horaInicialN"] = wifi.getHoraInicialN();
      doc["horaFinalN"] = wifi.getHoraFinalN();
      doc["url"] = wifi.getEndereco();
      char buf[600];
      serializeJson(doc, buf);

      salvarArquivo(SPIFFS, "/config.txt", buf);

      ESP.restart();
  }

  connectBle();
  connectWifi();
}

void loop() {
  while(true){
    response = "";
    getToken();
    count = 0; 
    json = "";
    pBLEScan->start(15);
  
    json = json + "}"; 
    pBLEScan->clearResults();
    gerarPresenca();
  }
}