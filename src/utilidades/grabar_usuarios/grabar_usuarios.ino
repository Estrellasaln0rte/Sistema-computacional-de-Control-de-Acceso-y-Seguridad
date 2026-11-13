// Utilidad: graba los 2 usuarios autorizados en la EEPROM (se ejecuta UNA vez).
// Formato por usuario (20 bytes): UID[4] + PIN[5] (4 dígitos + '\0') + NOMBRE[11] (10 car. + '\0').
// Usuario 0 -> direcciones 0..19, Usuario 1 -> direcciones 20..39.
#include <EEPROM.h>

struct Usuario {
  byte uid[4];
  char pin[5];
  char nombre[11];
};

// EDITE estos valores con los UID obtenidos con leer_uid.ino
const Usuario USUARIOS[2] = {
  { {0xDE, 0xAD, 0xBE, 0xEF}, "1234", "Usuario 1" },
  { {0x01, 0x02, 0x03, 0x04}, "4321", "Usuario 2" }
};

void setup() {
  Serial.begin(9600);
  int dir = 0;
  for (int u = 0; u < 2; u++) {
    for (int i = 0; i < 4; i++)  EEPROM.update(dir++, USUARIOS[u].uid[i]);
    for (int i = 0; i < 5; i++)  EEPROM.update(dir++, USUARIOS[u].pin[i]);
    for (int i = 0; i < 11; i++) EEPROM.update(dir++, USUARIOS[u].nombre[i]);
  }
  Serial.print(F("Usuarios grabados. Bytes escritos: "));
  Serial.println(dir);   // debe imprimir 40
}

void loop() {}
