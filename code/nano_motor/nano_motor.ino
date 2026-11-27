#include <SPI.h>
#include <Wire.h>
#include <RF24.h>
#include <MPU6050_light.h>

#define M1_PIN 6
#define M2_PIN 3
#define M3_PIN 5
#define M4_PIN 9

#define CE_PIN 7
#define CSN_PIN 8

RF24 radio(CE_PIN, CSN_PIN);
MPU6050 mpu(Wire);

const byte address[6] = "00001";

struct DataPacket {
  int throttle;
  int yaw;
};

DataPacket data;

const int IDLE_SPEED = 127;
const unsigned long LINK_TIMEOUT_MS = 500;

bool radioOk = false;
bool linked = false;
unsigned long lastReceive = 0;
unsigned long lastStatusPrint = 0;

float Kp_pitch = 1.5;
float Kd_pitch = 0.4;
float Kp_roll = 1.5;
float Kd_roll = 0.4;
float Kp_yaw = 1.0;

void setIndividualMotors(int m1, int m2, int m3, int m4) {
  analogWrite(M1_PIN, constrain(m1, 0, 255));
  analogWrite(M2_PIN, constrain(m2, 0, 255));
  analogWrite(M3_PIN, constrain(m3, 0, 255));
  analogWrite(M4_PIN, constrain(m4, 0, 255));
}

void setAllMotors(int speed) {
  setIndividualMotors(speed, speed, speed, speed);
}

void setup() {
  Serial.begin(115200);
  Wire.begin();

  pinMode(M1_PIN, OUTPUT);
  pinMode(M2_PIN, OUTPUT);
  pinMode(M3_PIN, OUTPUT);
  pinMode(M4_PIN, OUTPUT);

  setAllMotors(0);

  byte status = mpu.begin();
  Serial.print("MPU6050 status: ");
  Serial.println(status);
  while (status != 0) {}
  
  Serial.println("Calibrare MPU6050, nu misca drona...");
  delay(1000);
  mpu.calcOffsets(true, true);
  Serial.println("Calibrare gata!");

  radioOk = radio.begin() && radio.isChipConnected();

  if (radioOk) {
    radio.setChannel(100);
    radio.setDataRate(RF24_250KBPS);
    radio.setPALevel(RF24_PA_LOW);
    radio.openReadingPipe(1, address);
    radio.startListening();
    Serial.println("[RADIO] NRF24L01 detectat.");
  } else {
    Serial.println("[RADIO] EROARE NRF24L01.");
  }

  lastReceive = millis();
}

void loop() {
  mpu.update();

  if (radioOk && radio.available()) {
    while (radio.available()) {
      radio.read(&data, sizeof(data));
    }
    lastReceive = millis();

    if (!linked) {
      linked = true;
      Serial.println("[LINK] CONECTAT.");
    }
  }

  if (millis() - lastReceive > LINK_TIMEOUT_MS) {
    if (linked) {
      linked = false;
      Serial.println("[LINK] Conexiune PIERDUTA.");
    }

    setAllMotors(IDLE_SPEED);

    if (millis() - lastStatusPrint > 1000) {
      lastStatusPrint = millis();
      Serial.print("[IDLE] Motoare la: ");
      Serial.println(IDLE_SPEED);
    }
  } else {
    int baseThrottle = map(constrain(data.throttle, 0, 100), 0, 100, 0, 255);

    if (baseThrottle < 15) {
      setAllMotors(0);
    } else {
      float pitch = mpu.getAngleX();
      float roll = mpu.getAngleY();
      float gyroX = mpu.getGyroX();
      float gyroY = mpu.getGyroY();
      float gyroZ = mpu.getGyroZ();

      float targetYawRate = map(constrain(data.yaw, -100, 100), -100, 100, -90, 90);

      float pitchPID = (pitch * Kp_pitch) + (gyroX * Kd_pitch);
      float rollPID = (roll * Kp_roll) + (gyroY * Kd_roll);
      float yawPID = (targetYawRate - gyroZ) * Kp_yaw;

      int m1 = baseThrottle - pitchPID + rollPID - yawPID;
      int m2 = baseThrottle - pitchPID - rollPID + yawPID;
      int m3 = baseThrottle + pitchPID - rollPID - yawPID;
      int m4 = baseThrottle + pitchPID + rollPID + yawPID;

      setIndividualMotors(m1, m2, m3, m4);
    }
  }
}