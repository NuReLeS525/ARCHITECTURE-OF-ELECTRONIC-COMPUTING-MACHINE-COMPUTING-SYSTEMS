#include <SPI.h>
#include <MFRC522.h>
#include <Stepper.h>
#include <Servo.h>

// -------- RFID --------
#define SS_PIN 10
#define RST_PIN 7

MFRC522 rfid(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;

// -------- Step Motor --------
#define IN1 2
#define IN2 3
#define IN3 4
#define IN4 5

Stepper stepperMotor(2048, IN1, IN3, IN2, IN4);

// -------- Rain Sensor + Servo --------
const int rainSensorPin = 8;
const int servoPin = 9;

Servo myServo;
bool clothesCollected = false;


void setup() {
  Serial.begin(9600);

  // ---- RFID ----
  SPI.begin();
  rfid.PCD_Init();
  for (byte i = 0; i < 6; i++) key.keyByte[i] = 0xFF;

  // ---- Step Motor ----
  stepperMotor.setSpeed(30);

  // ---- Rain Sensor ----
  pinMode(rainSensorPin, INPUT_PULLUP);

  // ---- Servo ----
  myServo.attach(servoPin);
  myServo.write(140); // стартовое положение
}

void loop() {

  // ======================================================
  // ======== 1. БЛОК: ДОЖДЬ + СЕРВО ======================
  // ======================================================

  int rainState = digitalRead(rainSensorPin);

  if (rainState == LOW && !clothesCollected) {
    myServo.write(10);
    clothesCollected = true;
    Serial.println("Rain detected - Collecting clothes!");
  }
  else if (rainState == HIGH && clothesCollected) {
    myServo.write(140);
    clothesCollected = false;
    Serial.println("No rain - Returning clothes");
  }

  // Пауза для сенсора
  delay(100);

  // ======================================================
  // ======== 2. БЛОК: RFID + STEPPER =====================
  // ======================================================

  if (!rfid.PICC_IsNewCardPresent()) return;
  if (!rfid.PICC_ReadCardSerial()) return;

  Serial.println("Card detected");

  byte block = 4;
  byte readBuffer[18];
  byte size = sizeof(readBuffer);

  // Аутентификация
  MFRC522::StatusCode status;
  status = rfid.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, block, &key, &(rfid.uid));

  if (status != MFRC522::STATUS_OK) {
    Serial.println("Auth failed");
    return;
  }

  // Чтение данных
  status = rfid.MIFARE_Read(block, readBuffer, &size);
  if (status != MFRC522::STATUS_OK) {
    Serial.println("Read failed");
    return;
  }

  String data = "";
  for (int i = 0; i < 16; i++) {
    if (readBuffer[i] != 0 && readBuffer[i] != ' ')
      data += (char)readBuffer[i];
  }

  Serial.print("Parsed: ");
  Serial.println(data);

  if (data == "OK_ACCESS") {
    Serial.println("ACCESS GRANTED!");
    openDoor();
  } else {
    Serial.println("ACCESS DENIED!");
  }

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
}


// ======== ФУНКЦИЯ ОТКРЫТИЯ ДВЕРИ =========
void openDoor() {
  Serial.println("Opening door...");

  stepperMotor.step(1024);
  delay(1500);
  stepperMotor.step(-1024);

  Serial.println("Door cycle done");
}
