void connect_wifi() {
  if (WiFi.status() == WL_CONNECTED) {
    return;
  }
  WiFi.mode(WIFI_STA);  
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect_mqtt(void) {
  while (!MQTT.connected()) {
    Serial.print("Tentando conexão MQTT...");
    // Create a random client ID
    String clientId = "ESP32";
    clientId += String(random(0xffff), HEX);
    // Se conectado
    if (MQTT.connect(clientId.c_str(), mqttUser, mqttPassword)) {
      Serial.println("conectado");
      // Depois de conectado, publique um anúncio ...
      MQTT.publish("Squeeze_Squid/feeds/sala-inteligente-status", "Iniciando Comunicação");
      //... e subscribe.
      MQTT.subscribe("Squeeze_Squid/feeds/sala-inteligente-comando"); // <<<<----- mudar aqui
    } else {
      Serial.print("Falha, rc=");
      Serial.print(MQTT.state());
      Serial.println(" Tentando novamente em 5s");
      delay(5000);
    }
  }
}

void verifica_conexoes_wifi_mqtt(void) {
  connect_wifi();
  if (!MQTT.connected()) reconnect_mqtt();
}

bool colocar_cartao(){
  if (!mfrc522.PICC_IsNewCardPresent()) return false;
  if (!mfrc522.PICC_ReadCardSerial()) return false;
  return true;
}

void cadastrar_cartao(){
  unsigned long Tempo_Cadastro = millis();
  unsigned long Tempo_LED = millis();
  bool modo_low = false;
  Serial.println("Esperando conexão...");
  while(millis() - Tempo_Cadastro < 10000){
    if(millis() - Tempo_LED > 500 and modo_low == false){
      digitalWrite(led_amarelo, LOW);
      modo_low = true;
      Tempo_LED = millis();
    }
    if(millis() - Tempo_LED > 500 and modo_low == true){
      digitalWrite(led_amarelo, HIGH);
      modo_low = false;
      Tempo_LED = millis();
    }
    if(colocar_cartao()){
      Serial.println("Cartão conectado...");
      digitalWrite(led_amarelo, LOW);
      String id_cartao = "falha";
      unsigned long Tempo_Verificar = millis();
      while(id_cartao == "falha"){
        id_cartao = verificar_cartao();
        if(millis() - Tempo_Verificar > 5000){
            for(int i = 0; i < 3; i++){
              digitalWrite(led_vermelho, HIGH);
              delay(250);
              digitalWrite(led_vermelho, LOW);
              delay(250);
          }
          Serial.println("Falha ao conectar!");
          return;
        }
      }
      id_cartao.trim();
      if(verificar_memoria("/registro.txt", id_cartao)){
        for(int i = 0; i < 3; i++){
          digitalWrite(led_vermelho, HIGH);
          delay(250);
          digitalWrite(led_vermelho, LOW);
          delay(250);
        }
        Serial.println("Cartao ja cadastrado!");
        return;
      }else{
        writeFile(id_cartao + "/", "/registro.txt");
        for(int i = 0; i < 3; i++){
          digitalWrite(led_verde, HIGH);
          delay(250);
          digitalWrite(led_verde, LOW);
          delay(250);
        }
        Serial.println("Conectado com sucesso!");
        return;
      }
    }
  }
  for(int i = 0; i < 3; i++){
    digitalWrite(led_vermelho, HIGH);
    delay(250);
    digitalWrite(led_vermelho, LOW);
    delay(250);
  }
  Serial.println("Falha ao conectar cartão...");
  return;
}

bool verificar_memoria(String path, String valor){
  File rFile = SPIFFS.open(path, "r");//r+ leitura e escrita
  if (!rFile) {
    Serial.println("Erro ao abrir arquivo!");
  }
  else {
    while (rFile.position() < rFile.size()){
      String s = rFile.readStringUntil('/');
      s.trim();
      valor.trim();
      if (valor == s){
        rFile.close();
        return true;
      }
    }
    rFile.close();
  }
  return false;
}

String verificar_cartao(){
  if (colocar_cartao()){
    Serial.println("UID da tag:");                   // Mostra UID do token na serial
    String conteudo;                          //Cria uma variável vazia, do tipo string
    byte letra;
    for (byte i = 0; i < mfrc522.uid.size; i++) {
      if (mfrc522.uid.uidByte[i] < 0x10) {
        Serial.print(" 0");
      } else {
        Serial.print(" ");
      }
      Serial.print(mfrc522.uid.uidByte[i], HEX);  // Printa a mensagem convertida em hexadecimal
      if (mfrc522.uid.uidByte[i] < 0x10) {
        conteudo.concat(String(" 0"));
      } else {
        conteudo.concat(String(" "));
      }
      conteudo.concat(String(mfrc522.uid.uidByte[i], HEX));
    }
    Serial.println();
    conteudo.toUpperCase();  //Coloca todas as letras da string em maiúscula
    return conteudo;
  }
  return "falha";
}

void abrir_trava(){
  unsigned long tempo_trava = millis();
  while(millis() - tempo_trava < 10000){
    digitalWrite(trava, LOW);
    digitalWrite(led_verde, HIGH);
  }
  digitalWrite(trava, HIGH);
  digitalWrite(led_verde, LOW);
}

void formatFile() {
  Serial.println("Formantando SPIFFS");
  SPIFFS.format();
  Serial.println("Formatou SPIFFS");
}

void openFS(void) {
  if (!SPIFFS.begin()) {
    Serial.println("\nErro ao abrir o sistema de arquivos");
  }
  else {
    Serial.println("\nSistema de arquivos aberto com sucesso!");
  }
}

void writeFile(String state, String path) { //escreve conteúdo em um arquivo
  File rFile = SPIFFS.open(path, "a");//a para anexar
  if (!rFile) {
    Serial.println("Erro ao abrir arquivo!");
  }
  else {
    Serial.print("tamanho");
    Serial.println(rFile.size());
    rFile.println(state);
    Serial.print("Gravou: ");
    Serial.println(state);
  }
  rFile.close();
}

String readFile(String path) {
  Serial.println("Read file");
  File rFile = SPIFFS.open(path, "r");//r+ leitura e escrita
  if (!rFile) {
    Serial.println("Erro ao abrir arquivo!");
  }
  else {
    Serial.print("----------Lendo arquivo ");
    Serial.print(path);
    Serial.println("  ---------");
    while (rFile.position() < rFile.size())
    {
      s = rFile.readStringUntil('/');
      s.trim();
      Serial.println(s);
    }
    rFile.close();
    return s;
  }
}
