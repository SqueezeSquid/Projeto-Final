#include <IRremote.h>

const int RECV_PIN = 2;
const int pin_Rele = 7;
float resultado = 0;
IRrecv irrecv(RECV_PIN);
decode_results results;

bool rele_ligado = false;

void setup(){
  Serial.begin(9600);
  irrecv.enableIRIn();
  irrecv.blink13(true);

  pinMode(pin_Rele, OUTPUT);
  digitalWrite(pin_Rele, LOW);
  
}

void loop(){
  if (irrecv.decode(&results)){

    resultado = results.value;

    if (resultado == 16753245){
      if (!digitalRead(pin_Rele)){
        digitalWrite(pin_Rele, HIGH);
        Serial.println("liga");
      }else{
        digitalWrite(pin_Rele, LOW);
        Serial.println("desliga");
      }
    }

    Serial.println(results.value);
    
    irrecv.resume();
  }
}
