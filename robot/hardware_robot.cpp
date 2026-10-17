// hardware_robot.cpp
// La parte física: cómo el robot de verdad cumple lo que promete hardware.h.
//
// Va en el Arduino de los SENSORES: U2 en el esquemático del 15 oct. El robot
// lleva dos Arduino Uno unidos por serial (el D1 de cada uno al D0 del otro):
//
//   U2, este: los 3 ultrasónicos, su sensor de color y TODA la lógica.
//   U4, el de motores (carpeta motores/): recibe "llanta izquierda a tanto,
//       derecha a tanto" y se lo pasa al L298N, y prende los LEDs del color.
//       No decide nada.
//
// La lógica va aquí y no en U4 porque lee sensores todo el tiempo, y un
// ultrasónico se mide con un cronómetro (pulseIn) en el mismo Arduino donde
// está conectado. A los motores, en cambio, basta con mandarles una orden de
// vez en cuando: 4 bytes por serial.
//
// El serial de U4 es el mismo del USB: lo que este Arduino imprime con
// hw::mensaje también le llega a U4, que lo ignora (ver motores.ino). Para
// subir este programa hay que desconectar el cable que llega a D0.
//
// Lo que NO está en el esquemático del 15 oct (el detalle en el README):
//   - Encoders e IMU. Sin ellos el robot avanza y gira por tiempo, y usa las
//     paredes para corregirse: la de enfrente para saber cuánto avanzó y la de
//     un lado para ir derecho.
//   - Botón, selector, sensores de piso, garra y sensor bajito. Tienen pin
//     PROPUESTO en los pines libres de U2. Mientras electrónica no los ponga no
//     hacen nada, y el botón se puede probar con un cable de D8 a GND.

#include <Arduino.h>
#include <stdarg.h>
#include <stdio.h>
#include <Wire.h>
#include <EEPROM.h>
#include <Servo.h>
#include <Adafruit_TCS34725.h>
#include "hardware.h"
#include "config.h"

// ------------------------------------------------------------------ Pines del esquemático del 15 oct
// El esquemático dice a qué pines va cada ultrasónico, pero no cuál quedó
// adelante. CONFIRMAR con mecánica; si es otro, solo se cambian estos números.
const int TRIG_FRENTE = 2, ECO_FRENTE = 3;    // U1
const int TRIG_IZQ = 9, ECO_IZQ = 10;         // U5
const int TRIG_DER = 5, ECO_DER = 4;          // U6
// El sensor de color (TCS34725) va por I2C, en A4 y A5. U4 tiene otro sensor de
// color en su propio I2C: los dos tienen la misma dirección fija (0x29), así
// que no pueden compartir cables.
// El Arduino de motores va por serial, en D0 y D1.
const char INICIO = '~';                      // con esta letra empieza cada orden: la misma que en motores.ino

// ------------------------------------------------------------------ Pines PROPUESTOS (faltan en el esquemático)
// Son los pines que le quedan libres a U2. Hay que acordarlos con electrónica.
const int SERVO_GARRA = 7;         // el servo SG90 de la compuerta. Va en ESTE Arduino: la librería
                                   // Servo le quita el PWM a los pines 9 y 10, y en U4 esos dos son
                                   // la velocidad de los motores
// No hay botón (16 oct): el de arranque es el mismo micro switch de la pelota,
// apretado con el dedo. Por eso BOTON y SENSOR_GARRA son el mismo pin.
const int BOTON = A3, SELECTOR = 12;
const int LINEA_IZQ = A0, LINEA_DER = A1;
const int SHARP_BAJO = A2;         // solo si HAY_SENSOR_BAJO (config.h); el diseño de mecánica no lo trae
const int SENSOR_GARRA = A3;       // el micro switch del fondo del pocket: se aprieta cuando entra la pelota

// ------------------------------------------------------------------ Ajustes (hay que medirlos)
// Sin encoders, el robot sabe cuánto avanzó por el tiempo que lleva andando. Ese
// tiempo cambia con la carga de la batería: medir con la batería como va a
// estar en la ronda.
const float CM_POR_S = 15.0;        // andar 2 s con PWM_AVANCE, medir los cm y dividir entre 2
const float MS_POR_GRADO = 10.0;    // cronometrar 4 vueltas completas y dividir entre 1440
const int PWM_AVANCE = 160, PWM_LENTO = 110, PWM_GIRO = 150;
const int AJUSTE_DER = 0;           // si sin paredes se va chueco: + acelera la llanta derecha
const int FRENTE_CONFIABLE = 70;    // más lejos, el eco puede rebotar en las paredes de los lados
const int PARED_GUIA = 12;          // una pared de lado a menos de esto sirve para ir derecho
const int KP_PARED = 8;             // corrección por cada cm que se acerca o se aleja de ella
const int GARRA_ABIERTA = 90, GARRA_CERRADA = 20;   // grados del servo de la compuerta: medirlos al
                                                    // armarla (paso 10 del instructivo). En el
                                                    // modelo 3D abre 45 grados
const int LINEA_ES_BLANCO = LOW;    // el TCRT5000 manda LOW cuando ve blanco
const int PELOTA_DETECTADA = LOW;

static Servo servoGarra;
static Adafruit_TCS34725 sensorColor(TCS34725_INTEGRATIONTIME_24MS, TCS34725_GAIN_4X);

// ------------------------------------------------------------------ Hablar con el Arduino de motores
// Cada orden empieza con "~". Ningún texto de hw::mensaje la usa (y también van
// por este serial), así que U4 sabe qué es orden y qué es texto. Los números van
// en hexadecimal: puro texto, para que el monitor serial no se coma letras.
//
// Cada velocidad (de -254 a 254) va en un solo byte: la mitad, más 127 para que
// no haya negativos. Queda de 0 a 254 (7F = parado). Se pierde la mitad de la
// precisión (va de 2 en 2), y el motor no lo nota.
static uint8_t velocidadEnByte(int v) {
  v = constrain(v, -254, 254);
  return (uint8_t)(v / 2 + 127);
}

// Un byte como dos letras hexadecimales: 127 -> "7F"
static void enviarHex(uint8_t b) {
  const char DIGITOS[] = "0123456789ABCDEF";
  Serial.write(DIGITOS[b >> 4]);
  Serial.write(DIGITOS[b & 15]);
}

static void ordenMotores(int izq, int der) {
  Serial.write(INICIO);
  Serial.write('M');
  enviarHex(velocidadEnByte(izq));
  enviarHex(velocidadEnByte(der));
}

static void frenar() {
  Serial.write(INICIO);
  Serial.write('F');
  delay(150);                // U4 frena 80 ms; lo demás es para que el robot quede quieto
}                            // antes de leer los sensores

// ------------------------------------------------------------------ Distancias
static int ultrasonico(int trig, int eco) {
  digitalWrite(trig, LOW);
  delayMicroseconds(2);
  digitalWrite(trig, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig, LOW);
  unsigned long us = pulseIn(eco, HIGH, 12000UL);   // 12 ms: unos 2 m
  if (us == 0) return 200;                           // no rebotó: está lejos
  return (int)(us / 58);
}

int hw::distFrente() { return ultrasonico(TRIG_FRENTE, ECO_FRENTE); }
int hw::distIzquierda() { return ultrasonico(TRIG_IZQ, ECO_IZQ); }
int hw::distDerecha() { return ultrasonico(TRIG_DER, ECO_DER); }

int hw::distBaja() {
  if (!HAY_SENSOR_BAJO) return 200;           // sin sensor, nunca ve la pelota
  // Sharp GP2Y0A41SK0F (de 4 a 30 cm). La curva de voltaje a cm se ajusta midiendo.
  float volts = analogRead(SHARP_BAJO) * 5.0 / 1023.0;
  if (volts < 0.3) return 200;
  return (int)(12.08 * pow(volts, -1.058));
}

// ------------------------------------------------------------------ Movimiento
// Avanzar sin encoders. Dos ayudas para no depender solo del reloj:
//
// 1. Si hay pared enfrente a menos de 70 cm, se mide con ella: avanzar 10 cm es
//    que la pared quede 10 cm más cerca. El reloj queda de respaldo.
// 2. Si hay pared a un lado, el robot trata de mantenerse a la distancia a la
//    que empezó. Es lo que hacía el IMU del borrador del Mega: ir derecho.
void hw::avanzar(int cm) {
  if (cm == 0) return;
  int sentido = cm > 0 ? 1 : -1;
  unsigned long msPlaneado = (unsigned long)(abs(cm) * 1000.0 / CM_POR_S);

  int frente0 = distFrente();
  bool conFrente = frente0 < FRENTE_CONFIABLE;
  int objetivo = frente0 - cm;           // de reversa cm es negativo: la pared queda más lejos
  if (objetivo < 3) objetivo = 3;        // el HC-SR04 no mide bien a menos de 3 cm

  // La pared guía es la del lado más cercano, si está cerca
  int izq0 = distIzquierda();
  int der0 = distDerecha();
  bool guiaIzq = izq0 < PARED_GUIA && izq0 <= der0;
  bool guiaDer = der0 < PARED_GUIA && der0 < izq0;

  unsigned long inicio = millis();
  while (true) {
    unsigned long t = millis() - inicio;
    int base = PWM_AVANCE;
    if (conFrente) {
      int falta = sentido > 0 ? distFrente() - objetivo : objetivo - distFrente();
      if (falta <= 0) break;
      if (falta < 5) base = PWM_LENTO;     // despacio al final, para no pasarse por la inercia
      if (t > msPlaneado + 1500) break;    // el sensor se equivocó o algo lo atoró
    } else {
      if (t >= msPlaneado) break;
      delay(10);
    }

    // error = cuántos cm se desvió hacia la derecha. Si la pared de un lado deja
    // de verse (se abrió un pasillo), en esa vuelta no se corrige.
    int error = 0;
    if (guiaIzq) {
      int d = distIzquierda();
      if (d < PARED_GUIA + 5) error = d - izq0;
    } else if (guiaDer) {
      int d = distDerecha();
      if (d < PARED_GUIA + 5) error = der0 - d;
    }
    int c = constrain(KP_PARED * error, -60, 60);
    // Desviado a la derecha: la llanta derecha acelera y la izquierda frena. De
    // reversa es al revés, por eso todo se multiplica por el sentido.
    ordenMotores(sentido * (base - c), sentido * (base + c + AJUSTE_DER));
  }
  frenar();
}

// Girar sin IMU: por tiempo. El error se va sumando con cada giro; cuando haya
// IMU, esta función es la primera que cambia.
void hw::girar(int grados) {
  unsigned long ms = (unsigned long)(abs(grados) * MS_POR_GRADO);
  int s = grados > 0 ? 1 : -1;           // a la derecha: llanta izquierda adelante, derecha atrás
  unsigned long inicio = millis();
  while (millis() - inicio < ms) {
    ordenMotores(s * PWM_GIRO, -s * PWM_GIRO);
    delay(10);
  }
  frenar();
}

int hw::inclinacion() {
  return 0;   // sin IMU no hay forma de saber si está en la rampa: siempre "plano"
}

// ------------------------------------------------------------------ Piso
Rgb hw::leerRgb() {
  uint16_t r, g, b, c;
  sensorColor.getRawData(&r, &g, &b, &c);   // tarda lo que dura la integración (24 ms)
  // Se divide entre el brillo: así una sombra o la altura no cambian el color
  long suma = (long)r + g + b;
  if (suma == 0) suma = 1;
  Rgb m;
  m.r = (int)(r * 255L / suma);
  m.g = (int)(g * 255L / suma);
  m.b = (int)(b * 255L / suma);
  return m;
}

bool hw::lineaIzquierda() { return digitalRead(LINEA_IZQ) == LINEA_ES_BLANCO; }
bool hw::lineaDerecha() { return digitalRead(LINEA_DER) == LINEA_ES_BLANCO; }

// ------------------------------------------------------------------ Garra
void hw::garra(bool abierta) {
  servoGarra.write(abierta ? GARRA_ABIERTA : GARRA_CERRADA);
  delay(300);
}

bool hw::pelotaEnGarra() { return digitalRead(SENSOR_GARRA) == PELOTA_DETECTADA; }

// ------------------------------------------------------------------ LED y Serial
// Los LEDs están en U4: se le manda el número del color y él sabe cómo prenderlos.
void hw::mostrarColor(Color c) {
  Serial.write(INICIO);
  Serial.write('L');
  enviarHex((uint8_t)c);
  Serial.print(F("LED: "));
  Serial.println(nombreColor(c));
}

void hw::mostrarNumero(int n) {
  // Sin pantalla, por ahora solo al Serial. El ArUco pide que se vea 3 s.
  Serial.print(F("Número: "));
  Serial.println(n);
}

// El mensaje no se arma en un arreglo para luego imprimirlo (serían 110 bytes
// de RAM en el peor momento, a media ruta del BFS). Se imprime letra por letra
// directo al Serial: "salida" es un archivo de C cuya función de escribir manda
// cada letra por el Serial. El formato viene de la memoria de programa (TXT),
// por eso vfprintf_P y no vfprintf.
static int escribirLetra(char letra, FILE*) {
  Serial.write(letra);
  return 0;
}
static FILE salida;

void hw::mensaje(const char* formato, ...) {
  va_list datos;
  va_start(datos, formato);
  vfprintf_P(&salida, formato, datos);
  va_end(datos);
  Serial.println();
}

// ------------------------------------------------------------------ Botón, selector y reloj
bool hw::botonPresionado() { return digitalRead(BOTON) == LOW; }

void hw::esperarBoton() {
  while (!botonPresionado()) delay(5);
  delay(30);
  while (botonPresionado()) delay(5);
  delay(30);
}

bool hw::selectorPistaB() { return digitalRead(SELECTOR) == LOW; }

static unsigned long inicioRonda = 0;
void hw::reiniciarReloj() { inicioRonda = millis(); }
unsigned long hw::ms() { return millis() - inicioRonda; }

// ------------------------------------------------------------------ EEPROM: solo la calibración
// En la dirección 0 va una marca, para saber si lo que hay en la EEPROM es
// nuestro. Justo después, la calibración. Se lee directo a donde se va a usar:
// una copia de paso costaría 100 bytes de RAM, y en el Uno no sobran.
const uint16_t MARCA = 0xCA27;        // cambió del Mega al Uno: la calibración vieja no sirve
const int DIR_CALIBRACION = sizeof(MARCA);

void hw::guardarCalibracion(const Calibracion& c) {
  EEPROM.put(0, MARCA);
  EEPROM.put(DIR_CALIBRACION, c);
}

bool hw::cargarCalibracion(Calibracion& c) {
  uint16_t marca;
  EEPROM.get(0, marca);
  if (marca != MARCA) return false;
  EEPROM.get(DIR_CALIBRACION, c);
  return true;
}

// ------------------------------------------------------------------ Arranque
void hw::iniciar() {
  Serial.begin(115200);
  fdev_setup_stream(&salida, escribirLetra, NULL, _FDEV_SETUP_WRITE);   // para hw::mensaje
  pinMode(TRIG_FRENTE, OUTPUT);
  pinMode(ECO_FRENTE, INPUT);
  pinMode(TRIG_IZQ, OUTPUT);
  pinMode(ECO_IZQ, INPUT);
  pinMode(TRIG_DER, OUTPUT);
  pinMode(ECO_DER, INPUT);
  // Lo propuesto, con pull-up: sin nada conectado se lee "no hay línea", "no
  // hay pelota", "botón suelto" y "pista A". Así no hace nada raro mientras falte.
  pinMode(LINEA_IZQ, INPUT_PULLUP);
  pinMode(LINEA_DER, INPUT_PULLUP);
  pinMode(SENSOR_GARRA, INPUT_PULLUP);
  pinMode(BOTON, INPUT_PULLUP);
  pinMode(SELECTOR, INPUT_PULLUP);

  Wire.begin();                       // el I2C ahora solo es para el sensor de color
  Wire.setWireTimeout(3000, true);    // si el ruido de los motores traba el bus, no se cuelga

  // U4 puede tardar más en arrancar que este: se le pregunta hasta 2 s si está
  // ("~?") y se espera que conteste "~!".
  bool motoresListos = false;
  for (int i = 0; i < 20 && !motoresListos; i++) {
    Serial.write(INICIO);
    Serial.write('?');
    bool llegoInicio = false;
    unsigned long t = millis();
    while (millis() - t < 100 && !motoresListos) {
      if (!Serial.available()) continue;
      char c = (char)Serial.read();
      motoresListos = llegoInicio && c == '!';
      llegoInicio = (c == INICIO);
    }
  }
  Serial.println();
  Serial.println(motoresListos ? F("Arduino de motores: listo") : F("No contesta el Arduino de motores (U4)"));
  if (!sensorColor.begin()) Serial.println(F("No encuentro el sensor de color (en el modo de pruebas, i dice que hay en el I2C)"));
  servoGarra.attach(SERVO_GARRA);
  servoGarra.write(GARRA_CERRADA);
}
