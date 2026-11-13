#include <SPI.h>
#include <MFRC522.h>
#include <Keypad.h>
#include <EEPROM.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>

// Pantalla LCD I2C 16x2 (SDA = A4, SCL = A5)
LiquidCrystal_I2C* lcd;

uint8_t detectarLCD() {
  for (uint8_t dir = 0x20; dir < 0x40; dir++) {
    Wire.beginTransmission(dir);
    if (Wire.endTransmission() == 0) return dir;
  }
  return 0x27;
}

// Buzzer pasivo y servomotor
const int buzzerPin = 1;
const int servoPin  = 0;
Servo puerta;
const int ANGULO_CERRADO = 0;
const int ANGULO_ABIERTO = 90;

// Configuración RFID-RC522
#define RST_PIN 9
#define SS_PIN  10
MFRC522 mfrc522(SS_PIN, RST_PIN);

// Configuración 74HC595
const int latchPin = 8;
const int clockPin = 7;
const int dataPin  = 6;

// Configuración Teclado Matricial
const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte rowPins[ROWS] = {5, 4, 3, 2};
byte colPins[COLS] = {A0, A1, A2, A3};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// Estructura de Usuarios
struct Usuario {
  byte uid[4];
  char pin[5];
  char nombre[11];
};

Usuario usuarios[2];
int intentosFallidos = 0;

// Sonido del Impostor (frecuencias extraídas del audio). freq 0 = silencio
struct Nota {
  unsigned int freq;
  unsigned int dur;
};

const Nota sonidoImpostor[] = {
  {523, 200}, {659, 70},  {523, 90},  {392, 110},
  {659, 100}, {523, 110}, {910, 160}, {1000, 100},
  {90, 350},  {0, 80},
  {1960, 180}, {0, 40}, {1960, 160}, {0, 50},
  {1960, 140}, {0, 60}, {1960, 120}, {0, 70},
  {1960, 100}, {0, 90}, {1960, 80},  {0, 110},
  {1960, 60},  {0, 130}
};
const int totalNotas = sizeof(sonidoImpostor) / sizeof(sonidoImpostor[0]);

const unsigned int FRECUENCIA_ALARMA = 2000;

// Declaración anticipada de funciones
void actualizar74HC595(byte valor);
void cargarUsuariosEEPROM();
void iniciarSistema();
void iniciarRFID();
void mostrarEsperandoRFID();
int verificarTarjeta();
bool gestionarPin(int indiceUsuario);
void activarAlarma();
void mostrarLCD(const char* linea1, const char* linea2 = "");
void mostrarPin(int cantidad);
bool esperarOTecla(unsigned long ms, char teclaBuscada);
void abrirPuerta();
void cerrarPuerta();

void setup() {
  pinMode(latchPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(dataPin, OUTPUT);
  actualizar74HC595(0b00000001);   // LED 1: el arranque llegó hasta aquí

  pinMode(buzzerPin, OUTPUT);
  noTone(buzzerPin);

  Wire.begin();
  lcd = new LiquidCrystal_I2C(detectarLCD(), 16, 2);
  lcd->init();
  lcd->backlight();

  // Posición inicial del servo (se conecta solo un momento)
  puerta.attach(servoPin);
  puerta.write(ANGULO_CERRADO);
  delay(600);
  puerta.detach();
  delay(200);                      // deja estabilizar la alimentación

  // El RC522 se inicia DESPUÉS del servo
  SPI.begin();
  iniciarRFID();

  // Diagnóstico del lector
  byte version = mfrc522.PCD_ReadRegister(mfrc522.VersionReg);
  if (version == 0x00 || version == 0xFF) {
    mostrarLCD("Error RC522", "Revise cableado");
  } else {
    mostrarLCD("RC522 OK");
    lcd->setCursor(0, 1);
    lcd->print("Version 0x");
    lcd->print(version, HEX);
  }
  delay(2000);

  cargarUsuariosEEPROM();
  iniciarSistema();
}

void loop() {
  if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
    return;
  }

  // Estado: Validando RFID (LED 3)
  actualizar74HC595(0b00000100);
  mostrarLCD("Tarjeta leida", "Validando RFID");

  int indiceUsuario = verificarTarjeta();

  if (indiceUsuario != -1) {
    mostrarLCD("Bienvenido(s)", usuarios[indiceUsuario].nombre);
    delay(1500);

    intentosFallidos = 0;
    bool accesoConcedido = gestionarPin(indiceUsuario);

    if (accesoConcedido) {
      // Estado: Autorizado (LED 6)
      actualizar74HC595(0b00100000);
      mostrarLCD("Acceso", "Autorizado");
      delay(1500);
      mostrarLCD("Abriendo puerta", "C = cerrar");
      abrirPuerta();

      while (true) {
        char teclaCierre = keypad.getKey();
        if (teclaCierre == 'C') {
          mostrarLCD("Cerrando", "puerta...");
          cerrarPuerta();
          break;
        }
      }
      iniciarSistema();
    } else {
      activarAlarma();
    }
  } else {
    // Estado: Denegado (LED 7)
    actualizar74HC595(0b01000000);
    mostrarLCD("Tarjeta Invalida", "Acceso Denegado");
    delay(2500);
    mostrarEsperandoRFID();
  }

  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
}

// ---------- RFID ----------

void iniciarRFID() {
  mfrc522.PCD_Init();
  mfrc522.PCD_SetAntennaGain(mfrc522.RxGain_max);
}

// ---------- Servo (solo se alimenta mientras se mueve) ----------

void abrirPuerta() {
  puerta.attach(servoPin);
  for (int pos = ANGULO_CERRADO; pos <= ANGULO_ABIERTO; pos++) {
    puerta.write(pos);
    delay(15);
  }
  delay(300);
  puerta.detach();
}

void cerrarPuerta() {
  puerta.attach(servoPin);
  for (int pos = ANGULO_ABIERTO; pos >= ANGULO_CERRADO; pos--) {
    puerta.write(pos);
    delay(15);
  }
  delay(300);
  puerta.detach();
}

// ---------- Utilidades de pantalla ----------

void mostrarLCD(const char* linea1, const char* linea2) {
  lcd->clear();
  lcd->setCursor(0, 0);
  lcd->print(linea1);
  lcd->setCursor(0, 1);
  lcd->print(linea2);
}

void mostrarPin(int cantidad) {
  lcd->setCursor(0, 1);
  lcd->print("PIN: ");
  for (int i = 0; i < 4; i++) {
    lcd->print(i < cantidad ? '*' : '_');
  }
  lcd->print("       ");
}

bool esperarOTecla(unsigned long ms, char teclaBuscada) {
  unsigned long inicio = millis();
  while (millis() - inicio < ms) {
    if (keypad.getKey() == teclaBuscada) return true;
  }
  return false;
}

// ---------- Hardware ----------

void actualizar74HC595(byte valor) {
  digitalWrite(latchPin, LOW);
  shiftOut(dataPin, clockPin, MSBFIRST, valor);
  digitalWrite(latchPin, HIGH);
}

void cargarUsuariosEEPROM() {
  int dir = 0;
  for (int u = 0; u < 2; u++) {
    for (int i = 0; i < 4; i++)  usuarios[u].uid[i]    = EEPROM.read(dir++);
    for (int i = 0; i < 5; i++)  usuarios[u].pin[i]    = (char)EEPROM.read(dir++);
    for (int i = 0; i < 11; i++) usuarios[u].nombre[i] = (char)EEPROM.read(dir++);
  }
}

// ---------- Estados ----------

void iniciarSistema() {
  // Estado: Inicio (LED 1)
  actualizar74HC595(0b00000001);
  mostrarLCD("Iniciando...");
  iniciarRFID();                   // reinicia el lector por si el servo lo afectó
  delay(2000);
  mostrarEsperandoRFID();
}

void mostrarEsperandoRFID() {
  // Estado: Esperando RFID (LED 2)
  actualizar74HC595(0b00000010);
  mostrarLCD("Presente su", "tarjeta");
}

int verificarTarjeta() {
  delay(1000);
  for (int i = 0; i < 2; i++) {
    bool coincide = true;
    for (byte j = 0; j < 4; j++) {
      if (mfrc522.uid.uidByte[j] != usuarios[i].uid[j]) {
        coincide = false;
        break;
      }
    }
    if (coincide) return i;
  }
  return -1;
}

bool gestionarPin(int indiceUsuario) {
  // Estado: Esperando PIN (LED 4)
  actualizar74HC595(0b00001000);

  char pinIngresado[5];
  int indexPin = 0;
  pinIngresado[0] = '\0';

  mostrarLCD("*=Borrar  #=OK", "");
  delay(1500);
  mostrarLCD("Ingrese PIN:");
  mostrarPin(0);

  while (true) {
    char tecla = keypad.getKey();
    if (tecla) {
      if (tecla == '*') {
        if (indexPin > 0) {
          indexPin--;
          pinIngresado[indexPin] = '\0';
          mostrarPin(indexPin);
        }
      }
      else if (tecla == '#') {
        if (indexPin == 4) {
          pinIngresado[4] = '\0';

          // Estado: Validando PIN (LED 5)
          actualizar74HC595(0b00010000);
          mostrarLCD("Validando PIN...");
          delay(1000);

          if (strcmp(pinIngresado, usuarios[indiceUsuario].pin) == 0) {
            return true;
          } else {
            intentosFallidos++;
            if (intentosFallidos >= 3) {
              return false;
            } else {
              // Estado: Denegado por PIN incorrecto (LED 7)
              actualizar74HC595(0b01000000);
              char reintento[17];
              snprintf(reintento, sizeof(reintento), "Reintente (%d/3)", intentosFallidos);
              mostrarLCD("PIN Incorrecto", reintento);
              delay(2000);

              // Volver al estado de Esperando PIN (LED 4)
              actualizar74HC595(0b00001000);
              indexPin = 0;
              pinIngresado[0] = '\0';
              mostrarLCD("Ingrese PIN:");
              mostrarPin(0);
            }
          }
        } else {
          mostrarLCD("PIN de 4", "digitos exactos");
          delay(1500);
          mostrarLCD("Ingrese PIN:");
          mostrarPin(indexPin);
        }
      }
      else if (isDigit(tecla) && indexPin < 4) {
        pinIngresado[indexPin] = tecla;
        indexPin++;
        pinIngresado[indexPin] = '\0';
        mostrarPin(indexPin);
      }
    }
  }
}

void activarAlarma() {
  // Estado: Alarma (LED 8)
  actualizar74HC595(0b10000000);
  mostrarLCD("ACCESO BLOQUEADO", "ALARMA ACTIVADA");

  bool reiniciar = false;

  // 1) Sonido del Impostor (B lo puede interrumpir)
  for (int i = 0; i < totalNotas && !reiniciar; i++) {
    if (sonidoImpostor[i].freq > 0) tone(buzzerPin, sonidoImpostor[i].freq);
    else noTone(buzzerPin);

    if (esperarOTecla(sonidoImpostor[i].dur, 'B')) reiniciar = true;
  }
  noTone(buzzerPin);

  // 2) Alarma constante hasta presionar B
  if (!reiniciar) {
    mostrarLCD("ALARMA ACTIVADA", "B = reiniciar");
    tone(buzzerPin, FRECUENCIA_ALARMA);
    while (keypad.getKey() != 'B') { }
  }

  noTone(buzzerPin);
  mostrarLCD("Alarma", "desactivada");
  delay(1500);
  iniciarSistema();
}