# Sistema de Control de Acceso y Seguridad (Variante A)

Proyecto de aplicación 2S2026 – Arquitectura del Computador, Universidad Rafael Landívar.

Terminal de control de acceso basada en **Arduino UNO**: identifica al usuario con una tarjeta **RFID (RC522)**, valida un **PIN de 4 dígitos** con un teclado 4×4, abre una puerta con un **servomotor SG90** y dispara una **alarma** tras 3 PIN incorrectos. Los usuarios se guardan en la **EEPROM** y el estado del programa se muestra en **8 LEDs** mediante un **74HC595**.

**Integrantes:** 
Lizbeth Andrea Herrera Ortega – 1246024
Marcela Nicole Letran Lee – 1102124
Bruno Antonio Urías Castillo – 1184524

· **Sección:** 02

## Contenido
| Carpeta | Descripción |
|---|---|
| `Documentación/` | Documentación completa (diagrama de bloques, tabla de pines, arquitectura, manual de usuario, pruebas) |
| `src/control_acceso/` | Programa principal (`control_acceso.ino`) |
| `src/utilidades/` | `leer_uid` y `grabar_usuarios` para cargar usuarios en la EEPROM |
| `Diseño/` | Modelos STL del prototipo _(agregar)_ |

## Librerías
MFRC522, Keypad, LiquidCrystal I2C (instalar desde el Gestor de bibliotecas). SPI, Wire, EEPROM y Servo vienen con el IDE.

## Puesta en marcha
1. Cargue `leer_uid` y anote el UID de cada tarjeta (Monitor Serie, 9600 baudios).
2. Edite los UID/PIN/nombres en `grabar_usuarios`, cárguelo y ejecútelo una vez (imprime `40`).
3. Desconecte servo (D0) y buzzer (D1), cargue `control_acceso.ino` y vuelva a conectarlos.
4. Alimente el servo con su fuente (4.8–6 V) y una los GND.

## Uso rápido
- Presente la tarjeta → ingrese el PIN (`*` borra, `#` confirma).
- Acceso autorizado: la puerta se abre; **C** la cierra.
- Tres PIN incorrectos: alarma; **B** la desactiva.

## Pines
| Pin | Función | Pin | Función |
|---|---|---|---|
| D0 | Servo | D9 | RC522 RST |
| D1 | Buzzer | D10–D13 | RC522 SS, MOSI, MISO, SCK |
| D2–D5 | Filas del teclado | A0–A3 | Columnas del teclado |
| D6, D7, D8 | 74HC595 DS, SH_CP, ST_CP | A4, A5 | LCD I2C SDA, SCL |

Más detalle en `docs/Documentacion_Control_Acceso.docx`.
