// Utilidad: lee el UID de una tarjeta RFID y lo imprime en el Monitor Serie (9600 baudios).
// Úselo para obtener el UID de cada tarjeta antes de grabar los usuarios.
#include <SPI.h>
#include <MFRC522.h>

#define RST_PIN 9
#define SS_PIN  10
MFRC522 mfrc522(SS_PIN, RST_PIN);

void setup() {
  Serial.begin(9600);
  SPI.begin();
  mfrc522.PCD_Init();
  Serial.println(F("Acerque una tarjeta..."));
}

void loop() {
  if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) return;
  Serial.print(F("UID: "));
  for (byte i = 0; i < 4; i++) {
    Serial.print(F("0x"));
    if (mfrc522.uid.uidByte[i] < 0x10) Serial.print('0');
    Serial.print(mfrc522.uid.uidByte[i], HEX);
    Serial.print(i < 3 ? F(", ") : F("\n"));
  }
  mfrc522.PICC_HaltA();
}
