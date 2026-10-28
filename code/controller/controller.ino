#include <SPI.h>
#include <RF24.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define CE_PIN  7
#define CSN_PIN 8
RF24 radio(CE_PIN, CSN_PIN);
const byte address[6] = "00001";

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

#define JOY1_Y_PIN A0
#define JOY1_X_PIN A1
#define JOY2_Y_PIN A2
#define JOY2_X_PIN A3

struct DataPacket {
  int throttle;
  int yaw;
};

DataPacket data;

bool radioOk = false;

void setup() {
  Serial.begin(9600);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED nu a pornit!");
  }
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);

  // Verifica ca modulul NRF chiar raspunde. Fara modul, radio.write()
  // poate intoarce "true" fals, deci "Link: OK" ar fi o minciuna.
  radioOk = radio.begin() && radio.isChipConnected();
  if (!radioOk) {
    Serial.println("NRF24L01 nu raspunde!");
  }

  radio.setChannel(100);
  radio.setDataRate(RF24_250KBPS);
  radio.setPALevel(RF24_PA_LOW);
  radio.openWritingPipe(address);
  radio.stopListening();

  data.throttle = 0;
  data.yaw = 0;
}

void loop() {
  int rawThrottle = analogRead(JOY1_Y_PIN);
  int rawYaw       = analogRead(JOY1_X_PIN);
  int rawPitch     = analogRead(JOY2_Y_PIN);
  int rawRoll      = analogRead(JOY2_X_PIN);

  data.throttle = map(rawThrottle, 0, 1023, 0, 100);
  data.yaw       = map(rawYaw, 0, 1023, -50, 50);

  bool chipPresent = radioOk && radio.isChipConnected();
  bool sendOk = chipPresent && radio.write(&data, sizeof(DataPacket));

  display.clearDisplay();
  display.setCursor(0, 0);

  display.print("NRF: ");
  display.println(chipPresent ? "detectat" : "LIPSA!");

  display.print("Link: ");
  display.println(sendOk ? "OK (ACK)" : "ESUATA");

  display.print("Throttle: ");
  display.println(data.throttle);

  display.print("Yaw: ");
  display.println(data.yaw);

  display.print("Pitch(raw): ");
  display.println(rawPitch);

  display.print("Roll(raw): ");
  display.println(rawRoll);

  display.display();

  delay(50);
}