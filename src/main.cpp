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
#include <IRremote.h>
#include <esp_wpa2.h>
#include <NTPClient.h>

BLEScan *pBLEScan;
WiFiManager wifi;
JsonObject doc;
IRrecv irrecv(35);
IRsend irsend;
decode_results results;
unsigned int rawCodes[RAWBUF];
String rawCodesEnv;
String response;
WiFiUDP udp;
int presenca;
NTPClient ntp(udp, "a.st1.ntp.br", -3 * 3600, 60000);

//Método para leitura de arquivo do file system do esp32
void lerArquivo(fs::FS &fs, const char *path)
{
  Serial.printf("Lendo arquivo: %s\n", path);

  File file = fs.open(path);
  if (!file || file.isDirectory())
  {
    Serial.println("Erro ao abrir o arquivo");
    return;
  }

  while (file.available())
  {
    //Joga as informações do arquivo para Json
    size_t size = file.size();
    char buf[600];
    file.readBytes(buf, size);
    DynamicJsonDocument js(2048);
    deserializeJson(js, buf, strlen(buf));
    doc = js.as<JsonObject>();
  }
}

void init()
{
  if (!SPIFFS.begin(true))
  {
    Serial.println("\nErro ao abrir o sistema de arquivos");
  }
  else
  {
    delay(100);
    lerArquivo(SPIFFS, "/config.txt");
    delay(100);
  }
}

//Método para conexão com WIFI WPA2
void connectWifi()
{
  init();
  char identidade[32];
  char ssid[32];
  char senha[32];

  strcpy(ssid, doc["ssid"]);
  strcpy(identidade, doc["identidade"]);
  strcpy(senha, doc["senha"]);
  WiFi.mode(WIFI_STA);
  esp_wifi_sta_wpa2_ent_set_identity((uint8_t *)identidade, strlen(identidade));
  esp_wifi_sta_wpa2_ent_set_username((uint8_t *)identidade, strlen(identidade));
  esp_wifi_sta_wpa2_ent_set_password((uint8_t *)senha, strlen(senha));
  WiFi.enableSTA(true);
  esp_wpa2_config_t config = WPA2_CONFIG_INIT_DEFAULT();

  if (esp_wifi_sta_wpa2_ent_enable(&config) != ESP_OK)
  {
    Serial.println("WPA2 Settings Not OK");
  }

  WiFi.begin(ssid);
  WiFi.setHostname(identidade);

  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print("conectando wifi...");
    delay(1000);
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address set: ");
  Serial.print(WiFi.localIP().toString().c_str());
}

void conectWIFIHOME()
{
  const char *id = "Edemarjr";
  const char *password = "Xtz250x2";
  WiFi.begin(id, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.println("Conectando WIFI");
  }
}

//Método para montar códigos IR
void storeCode(decode_results *results)
{
  int count = results->rawlen;

  for (int i = 1; i < count; i++)
  {
    if (i & 1)
    {
      Serial.print(results->rawbuf[i] * USECPERTICK, DEC);
      rawCodesEnv = rawCodesEnv + results->rawbuf[i] * USECPERTICK, DEC;
      rawCodes[i - 1] = results->rawbuf[i] * USECPERTICK, DEC;
    }
    else
    {
      Serial.write('-');
      Serial.print((unsigned long)results->rawbuf[i] * USECPERTICK, DEC);
      rawCodesEnv = rawCodesEnv + (unsigned long)results->rawbuf[i] * USECPERTICK, DEC;
      rawCodes[i - 1] = (unsigned long)results->rawbuf[i] * USECPERTICK, DEC;
    }
    Serial.print(" ");
    rawCodesEnv = rawCodesEnv + " ";
  }
}

//Seta Configurações iniciais BLE
void connectBle()
{
  BLEDevice::init("");
  BLEDevice::setPower(ESP_PWR_LVL_P7);
  pBLEScan = BLEDevice::getScan();
  pBLEScan->setActiveScan(true);
}

//Método para conexões com servidor
void conexao(String servico, String dados, boolean retorno)
{

  if (WiFi.status() == WL_CONNECTED)
  {

    HTTPClient http;
    http.begin("http://192.168.1.107:8070/WebServiceRest/rest/service/" + servico);
    http.addHeader("Content-Type", "application/json");

    int httpResponseCode = http.POST(dados);

    if (httpResponseCode < 1)
    {
      Serial.print("Erro, código: ");
      Serial.println(httpResponseCode);
    }
    if (retorno)
    {
      response = http.getString();
    }
    http.end();
  }
  else
  {
    Serial.println("Erro, sem conexão");
    connectWifi();
  }
}

//Método para salvar informações no file system
void salvarArquivo(fs::FS &fs, const char *path, const char *message)
{
  Serial.printf("salvando arquivo: %s\n", path);

  File file = fs.open(path, FILE_WRITE);
  if (!file)
  {
    Serial.println("erro ao salvar informação");
    return;
  }
  if (file.print(message))
  {
    Serial.println("Arquivo salvo");
  }
  else
  {
    Serial.println("Informação não salva");
  }
}

//Método para inicializar wifimanager e configurar o esp32
void configuraDispositivo()
{
  WiFiManager wifiManager;
  wifiManager.setBreakAfterConfig(true);
  String n = "Configurar dispositivo FURB - ";
  String nome=  n + WiFi.macAddress().c_str();

  if (!wifiManager.startConfigPortal(nome.c_str()))
  {
    if (!SPIFFS.begin(true))
    {
      Serial.println("\nErro ao abrir o sistema de arquivos");
    }
    else
    {
      delay(3000);
      StaticJsonDocument<1056> docJson;
      char inf[700];
      docJson["ssid"] = wifiManager.getSSID();
      docJson["identidade"] = wifiManager.getIdentidade();
      docJson["senha"] = wifiManager.getPassword();
      docJson["sala"] = wifiManager.getSala();
      docJson["bloco"] = wifiManager.getBloco();
      docJson["horaInicialM"] = wifiManager.getHoraInicialM();
      docJson["horaFinalM"] = wifiManager.getHoraFinalM();
      docJson["horaInicialV"] = wifiManager.getHoraInicialV();
      docJson["horaFinalV"] = wifiManager.getHoraFinalV();
      docJson["horaInicialN"] = wifiManager.getHoraInicialN();
      docJson["horaFinalN"] = wifiManager.getHoraFinalN();
      docJson["endereco"] = wifiManager.getEndereco();
      docJson["esp"] = wifiManager.getNome();

      serializeJson(docJson, inf);
      Serial.println(inf);
      salvarArquivo(SPIFFS, "/config.txt", inf);
      delay(100);
      ESP.restart();
    }
  }
}

void configuraInfraRedReciver(int tipo)
{
  if (doc.size() < 1)
  {
    Serial.println("Deve ser configurado o dispositivo.");
  }
  else
  {
    irrecv.enableIRIn();

    while (true)
    {

      Serial.println("Aguardando infravermelho....");
      if (irrecv.decode(&results))
      {
        storeCode(&results);
        Serial.println(rawCodesEnv);
        irrecv.resume();
        delay(30);

        init();
        Serial.println("teste1");
        char sala[32];
        char esp[32];
        char buf[2500];

        StaticJsonDocument<2056> docJson;
        strcpy(sala, doc["sala"]);
        strcpy(esp, doc["esp"]);
        Serial.println(rawCodesEnv);
        docJson["raw"] = rawCodesEnv;
        docJson["sala"] = sala;
        docJson["esp"] = esp;
        docJson["tipo"] = tipo;
        Serial.println("teste3");
        serializeJson(docJson, buf);
        docJson.clear();
        Serial.println("teste1");
        rawCodesEnv = "";
        //connectWifi();
        conectWIFIHOME();
        conexao("insertRaw", buf, false);
        ESP.restart();
      }
    }
  }
}

//Método para enviar sinal IR
void enviarSinalIR(char inf[300])
{
  conexao("getRaw", inf, true);
  delay(100);

  if (response.length() > 0)
  {

    char *cstr = new char[response.length() + 1];
    strcpy(cstr, response.c_str());
    char *p = strtok(cstr, " ");
    int cont = 0;
    uint8_t count = 0;
    rawCodes[count] = atoi(p);

    while (p != 0)
    {
      count++;
      p = strtok(NULL, " ");
      if (p != " " && p != NULL)
        rawCodes[count] = atoi(p);
    }

    irsend.sendRaw(rawCodes, cont + 100, 38);
    response.clear();
  }
}

//Método para validar horário de aula
boolean verificaHoraAula()
{
  init();
  char horaInicialM[12];
  char horaFinalM[12];
  char horaInicialV[12];
  char horaFinalV[12];
  char horaInicialN[12];
  char horaFinalN[12];

  strcpy(horaInicialM, doc["horaInicialM"]);
  strcpy(horaFinalM, doc["horaFinalM"]);
  strcpy(horaInicialV, doc["horaInicialV"]);
  strcpy(horaFinalV, doc["horaFinalV"]);
  strcpy(horaInicialN, doc["horaInicialN"]);
  strcpy(horaFinalN, doc["horaFinalN"]);

  char *horaI = strtok(horaInicialM, ":");
  char *minutoI = strtok(NULL, ":");
  char *horaF = strtok(horaFinalM, ":");
  char *minutoF = strtok(NULL, ":");

  if ((ntp.getHours() > atoi(horaI) || (ntp.getHours() == atoi(horaI) && ntp.getMinutes() >= atoi(minutoI))) &&
      (ntp.getHours() < atoi(horaF) || (ntp.getHours() == atoi(horaF) && ntp.getMinutes() <= atoi(minutoF))))
    return true;

  horaI = strtok(horaInicialV, ":");
  minutoI = strtok(NULL, ":");
  horaF = strtok(horaFinalV, ":");
  minutoF = strtok(NULL, ":");

  if ((ntp.getHours() > atoi(horaI) || (ntp.getHours() == atoi(horaI) && ntp.getMinutes() >= atoi(minutoI))) &&
      (ntp.getHours() < atoi(horaF) || (ntp.getHours() == atoi(horaF) && ntp.getMinutes() <= atoi(minutoF))))
    return true;

  horaI = strtok(horaInicialN, ":");
  minutoI = strtok(NULL, ":");
  horaF = strtok(horaFinalN, ":");
  minutoF = strtok(NULL, ":");

  if ((ntp.getHours() > atoi(horaI) || (ntp.getHours() == atoi(horaI) && ntp.getMinutes() >= atoi(minutoI))) &&
      (ntp.getHours() < atoi(horaF) || (ntp.getHours() == atoi(horaF) && ntp.getMinutes() <= atoi(minutoF))))
    return true;

  return false;
}

void setup()
{
  Serial.begin(9600);
  //Botões de configuração
  //Configuração inicial, nome, sala etc..
  pinMode(27, INPUT);
  //configuração IR ar condicionado
  pinMode(16, INPUT);
  //Configuração IR projetor
  pinMode(33, INPUT);
  //Configuração Sensor presença
  pinMode(14, INPUT);
  delay(5000);

  Serial.println("Iniciando");

  //Pega valores das IOs
  byte valorBotaoConfigIRR = digitalRead(16);
  byte valorBotaoConfigIRP = digitalRead(33);
  byte valorBTConfigInf = digitalRead(27);

  if (valorBTConfigInf == 1)
  {
    configuraDispositivo();
  }

  if (valorBotaoConfigIRR == 1)
  {
    init();
    configuraInfraRedReciver(1);
  }

  if (valorBotaoConfigIRP == 1)
  {
    init();
    configuraInfraRedReciver(2);
  }

  //Inicializa BLE
  connectBle();
  Serial.println("teste1");
  //Conecta WIFI
  conectWIFIHOME();
  //connectWifi();
  //Inicializa sistema de hora
  ntp.begin();
  ntp.forceUpdate();
}

void loop()
{
  init();
  if (doc.size() < 1)
  {
    Serial.println("Deve ser configurado o dispositivo.");
    int c = 0;
    while(c < 3){
      pinMode(2,OUTPUT);
      digitalWrite(2, HIGH); 
      delay(1000);                       
      digitalWrite(2, LOW);   
      delay(1000);
      c++;
    }
    ESP.restart();
  }
  else
  {

    if (verificaHoraAula())
    {
      Serial.println("Iniciando processo de registro de frequencia. ");
      presenca = 0;
      //Scaneia BLEs
      BLEScanResults foundDevices = pBLEScan->start(25);
      int devices_count = foundDevices.getCount();
      Serial.println("Quantidade de Alunos encontrados: ");
      Serial.println(devices_count);
      init();
      char sala[32];
      char esp[32];

      strcpy(sala, doc["sala"]);
      strcpy(esp, doc["esp"]);
      String js = "{";
      //Para cada device encontrado, coloca no Json as informações para envio
      for (uint32_t i = 0; i < devices_count; i++)
      {
        BLEAdvertisedDevice device = foundDevices.getDevice(i);
        if (i == devices_count || i == 0)
          js = js + "\"" + i + "\":{";
        else
          js = js + ",\"" + i + "\":{";

        js = js + "\"aluno\":\"" + device.getName().c_str() + "\",";
        js = js + "\"mac\":\"" + device.getAddress().toString().c_str() + "\",";
        js = js + "\"sala\":\"" + sala + "\",";
        js = js + "\"esp\":\"" + esp + "\"}";
        Serial.println("Nome: ");
        Serial.println(device.getName().c_str());
        Serial.println("Mac: ");
        Serial.println(device.getAddress().toString().c_str());
      }

      js = js + "}";

      pBLEScan->clearResults();

      //Envia alunos encontrados para registo no servidor
      conexao("registraPresenca", js, false);
      Serial.println("Registros enviados");
      js = "";
      response = "";
      delay(200000);
    }
    else
    {
      //Verifica se tem alguem em sala de aula
      if (presenca == 0)
      {
        presenca = 1;

        int temPresenca = 0;
        int cont = 0;

        while (cont < 5)
        {
          if (digitalRead(14) == 1)
            temPresenca++;
          cont++;
          delay(2000);
        }

        //Se não possui pessoas em sala de aula, envia sinal para desligar os aparelhos.
        if (temPresenca <= 2)
        {
          Serial.println("Desligando equipamentos: ");
          char buf[2048];
          char sala[32];
          char esp[32];
          StaticJsonDocument<2048> docJsons;
          init();
          strcpy(sala, doc["sala"]);
          strcpy(esp, doc["esp"]);
          docJsons["sala"] = sala;
          docJsons["esp"] = esp;
          docJsons["tipo"] = 1;
          serializeJson(docJsons, buf);
          enviarSinalIR(buf);
          delay(2000);
          docJsons["tipo"] = 2;
          serializeJson(docJsons, buf);
          enviarSinalIR(buf);
          delay(2000);
          enviarSinalIR(buf);
          delay(2000);
          docJsons.clear();
          Serial.println("Equipamentos desligados: ");
        }else{
          Serial.println("Verificado movimento: ");
          presenca = 0;
        }
      }
    }
  }
}
