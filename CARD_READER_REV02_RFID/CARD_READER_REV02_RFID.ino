#include <SPI.h>
#include <MFRC522.h>
#include <FS.h>
#include "SPIFFS.h"
#include <WiFi.h>
#include <NTPClient.h> /* https://github.com/arduino-libraries/NTPClient */
#include <PubSubClient.h>

//--------------------DHT Temperatura-----------------------------//
#include <DHT.h>
#define DHTPIN 5 
#define DHTTYPE DHT11   // DHT 11
DHT dht(DHTPIN, DHTTYPE);
//----------------------------------------------------------------//

#define PINO_SS 21
#define PINO_RST 2
#define trava 22
#define led_verde 4
#define led_amarelo 12
#define led_vermelho 14

#define TOPICO_SUBSCRIBE "Sala Inteligente - Comando"
#define TOPICO_PUBLISH "Sala Inteligente - Info"

#define IO_USERNAME  "rferreiramendori@gmail.com"
#define IO_KEY       "EUTEamo1978"
const char* ssid = "George_2G";
const char* password =  "100200300";
const char* mqttserver = "maqiatto.com";
const int mqttport = 1883;
const char* mqttUser = IO_USERNAME;
const char* mqttPassword = IO_KEY;

WiFiClient espClient;
PubSubClient MQTT(espClient);

int Tempo_MQTT = millis();

String str;
String s, s_old;
String Data;
String Hora;
String cartao;
String mensagem = "";

unsigned long tempo = millis();
unsigned long tempo_trava;

bool modo_low;

WiFiUDP udp;
NTPClient ntp(udp, "a.st1.ntp.br", -3 * 3600, 60000);/* Cria um objeto "NTP" com as configurações.utilizada no Brasil */

MFRC522 mfrc522(PINO_SS, PINO_RST);

void callback(char* topic, byte* payload, unsigned int length) {
  /*for (int i = 0; i < length; i++) {
    mensagem[i] = char(payload[i]);
  }*/
  Serial.println(char(payload[0]));
  if (char(payload[0]) == 'a') {
    cadastrar_cartao();
  }
  if (char(payload[0]) == 'b') {
    readFile("/registro.txt");
  }
  if (char(payload[0]) == 'c') {
    formatFile();    //Caso essa seja a primeira vez utilizando o ESP, descomente essa linha;
    readFile("/registro.txt");
  }
  if (char(payload[0]) == 'd') {
    verificar_cartao();
  }
}

void setup() {
  Serial.begin(115200);
  SPI.begin();
  dht.begin();
  mfrc522.PCD_Init();

  connect_wifi();

  delay(100);
  
  ntp.begin();           
  ntp.forceUpdate(); 
  Serial.println("abrir arquivo");
  openFS();

  pinMode(trava, OUTPUT);
  pinMode(led_verde, OUTPUT);
  pinMode(led_amarelo, OUTPUT);
  pinMode(led_vermelho, OUTPUT);

  digitalWrite(trava, HIGH);
  digitalWrite(led_verde, LOW);
  digitalWrite(led_amarelo, LOW);
  digitalWrite(led_vermelho, LOW);

  //writeFile("Diego Maia Marques/53 A2 3C 91/Ryan Noriyuki Ferreira Mendori/83 9C BD 0B/", "/registro.txt");
  String arqui = readFile("/registro.txt");

  MQTT.setServer(mqttserver, 1883);
  MQTT.setCallback(callback);
}

void loop() {

  verifica_conexoes_wifi_mqtt();

  if (!MQTT.connected()) {
    reconnect_mqtt();
  }

  float h = dht.readHumidity();
  float t = dht.readTemperature();

  float hic = dht.computeHeatIndex(t, h, false);
  
  MQTT.loop();
  if ((millis() - Tempo_MQTT) > 5000) {
    MQTT.publish("rferreiramendori@gmail.com/sala-inteligente-status", "SALA=OK");
    char s_t[8];
    dtostrf(t,1,2,s_t);
    MQTT.publish("rferreiramendori@gmail.com/temperatura", s_t);
    char s_h[8];
    dtostrf(h,1,2,s_h);
    MQTT.publish("rferreiramendori@gmail.com/umidade", s_h);
    char s_hic[8];
    dtostrf(hic,1,2,s_hic);
    MQTT.publish("rferreiramendori@gmail.com/sensacao_termica", s_hic);
    
    Tempo_MQTT = millis();
  }

  if(colocar_cartao()){
    String ID = verificar_cartao();
    if (!verificar_memoria("/registro.txt", ID.substring(1))){
      for(int i = 0; i < 3; i++){
        digitalWrite(led_vermelho, HIGH);
        delay(250);
        digitalWrite(led_vermelho, LOW);
        delay(250);
      }
      return;
    }else{
      abrir_trava();
    }
  }
}
