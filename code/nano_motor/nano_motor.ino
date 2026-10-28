#include <SPI.h>
#include <RF24.h>

// ---------- DRV8833 #1 ----------
#define AIN1 6   // PWM - Motor 1
#define AIN2 2
#define BIN1 3   // PWM - Motor 2
#define BIN2 4

// ---------- DRV8833 #2 ----------
#define AIN1_2 5   // PWM - Motor 3
#define AIN2_2 A0
#define BIN1_2 9   // PWM - Motor 4
#define BIN2_2 A1

#define CE_PIN 7
#define CSN_PIN 8

RF24 radio(CE_PIN, CSN_PIN);
const byte address[6] = "00001";

struct DataPacket {
  int throttle;
  int yaw;
};

DataPacket data;

// ---------- Setari ----------
const int IDLE_SPEED = 127;                 // jumatate din 255, cat timp nu e legatura radio
const unsigned long LINK_TIMEOUT_MS = 500;  // fara pachet atatea ms = conexiune pierduta

bool radioOk = false;       // modulul NRF raspunde?
bool linked = false;        // primim pachete de la controller?
unsigned long lastReceive = 0;
unsigned long lastStatusPrint = 0;

void setMotors(int speed) {
  analogWrite(AIN1, speed);    // Motor 1
  analogWrite(BIN1, speed);    // Motor 2
  analogWrite(AIN1_2, speed);  // Motor 3
  analogWrite(BIN1_2, speed);  // Motor 4
}

void setup() {
  Serial.begin(9600);

  pinMode(AIN1, OUTPUT);
  pinMode(AIN2, OUTPUT);
  pinMode(BIN1, OUTPUT);
  pinMode(BIN2, OUTPUT);
  pinMode(AIN1_2, OUTPUT);
  pinMode(AIN2_2, OUTPUT);
  pinMode(BIN1_2, OUTPUT);
  pinMode(BIN2_2, OUTPUT);

  digitalWrite(AIN2, LOW);
  digitalWrite(BIN2, LOW);
  digitalWrite(AIN2_2, LOW);
  digitalWrite(BIN2_2, LOW);

  radioOk = radio.begin() && radio.isChipConnected();

  if (radioOk) {
    radio.setChannel(100);
    radio.setDataRate(RF24_250KBPS);
    radio.setPALevel(RF24_PA_LOW);
    radio.openReadingPipe(1, address);
    radio.startListening();
    Serial.println("[RADIO] Modul NRF24L01 detectat. Astept controllerul...");
  } else {
    Serial.println("[RADIO] EROARE: modulul NRF24L01 nu raspunde (verifica cablaj si 3.3V).");
    Serial.println("[RADIO] Continui fara radio, motoarele raman pe idle.");
  }

  lastReceive = millis();
  Serial.println("start");
}

void loop() {
  // ---------- Citeste pachetele, daca radioul merge ----------
  if (radioOk && radio.available()) {
    while (radio.available()) {
      radio.read(&data, sizeof(data));
    }
    lastReceive = millis();

    if (!linked) {
      linked = true;
      Serial.println("[LINK] CONECTAT la controller.");
    }

    data.throttle = constrain(data.throttle, 0, 100);
    int speed = map(data.throttle, 0, 100, 0, 255);
    setMotors(speed);

    Serial.print("[ZBOR] Throttle: ");
    Serial.print(data.throttle);
    Serial.print("  -> viteza PWM: ");
    Serial.println(speed);
  }

  // ---------- Verifica daca legatura s-a pierdut / nu exista ----------
  if (millis() - lastReceive > LINK_TIMEOUT_MS) {
    if (linked) {
      linked = false;
      Serial.println("[LINK] Conexiune PIERDUTA. Motoarele revin pe idle.");
    }

    // Fara legatura: motoarele merg pe jumatate
    setMotors(IDLE_SPEED);

    // Mesaj de stare la fiecare secunda, ca sa nu umple Serial Monitor-ul
    if (millis() - lastStatusPrint > 1000) {
      lastStatusPrint = millis();
      Serial.print("[IDLE] Neconectat. Motoare la ");
      Serial.print(IDLE_SPEED);
      Serial.println("/255.");
    }
  }
}