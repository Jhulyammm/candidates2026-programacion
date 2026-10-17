// pruebas.cpp
// Modo de pruebas: medir el robot de verdad antes de correr una pista. Usa las
// mismas funciones que la pista (hw::avanzar, hw::girar...), así que lo que se
// ajuste aquí es lo que va a usar el robot en la ronda.
//
// Cómo se entra: con el robot conectado a la computadora y el monitor serial a
// 115200, al prender escribe "p" y Enter en los primeros 3 segundos. Para que
// la letra llegue, el cable que va al D0 de este Arduino tiene que estar
// desconectado, el mismo que se quita para subir el programa: si no, el Arduino
// de motores no deja pasar lo que manda la computadora. Por eso al arrancar sale
// "No contesta el Arduino de motores": en el modo de pruebas es normal.
//
// Cada letra hace una prueba y dice qué número cambiar según lo que pase.

#include <Arduino.h>
#include <Wire.h>
#include "pruebas.h"
#include "base.h"
#include "colores.h"
#include "hardware.h"

bool quierenModoPruebas() {
  Serial.println(F("Escribe p y Enter en 3 s para el modo de pruebas"));
  unsigned long inicio = millis();
  while (millis() - inicio < 3000) {
    if (Serial.available() && Serial.read() == 'p') return true;
  }
  return false;
}

static void ayuda() {
  Serial.println();
  Serial.println(F("Modo de pruebas. Escribe una letra y Enter:"));
  Serial.println(F("  s  sensores: distancias, color y switches (10 lecturas)"));
  Serial.println(F("  i  que hay conectado al I2C (el sensor de color es 0x29)"));
  Serial.println(F("  u  ultrasonicos: en que pin contesta el eco de cada TRIG"));
  Serial.println(F("  w  switches: 20 s mostrando que pin cambia (mueve los switches)"));
  Serial.println(F("  a  avanzar 30 cm (sin pared enfrente a menos de 70 cm)"));
  Serial.println(F("  g  girar 90 grados a la derecha"));
  Serial.println(F("  v  girar 4 vueltas completas"));
  Serial.println(F("  c  abrir o cerrar la compuerta"));
  Serial.println(F("  l  LEDs: cada color, 1 segundo"));
}

static void sensores() {
  for (int i = 0; i < 10; i++) {
    Rgb c = hw::leerRgb();
    Serial.print(F("enfrente "));
    Serial.print(hw::distFrente());
    Serial.print(F("  izq "));
    Serial.print(hw::distIzquierda());
    Serial.print(F("  der "));
    Serial.print(hw::distDerecha());
    Serial.print(F(" cm | color r "));
    Serial.print(c.r);
    Serial.print(F(" g "));
    Serial.print(c.g);
    Serial.print(F(" b "));
    Serial.print(c.b);
    Serial.print(F(" | pelota "));
    Serial.print(hw::pelotaEnGarra() ? F("si") : F("no"));
    Serial.print(F(" | boton "));
    Serial.println(hw::botonPresionado() ? F("si") : F("no"));
    delay(300);
  }
  Serial.println(F("Tapa cada ultrasonico con la mano: el que baja es el de ese lado."));
  Serial.println(F("Si no coinciden, cambia TRIG_ y ECO_ en hardware_robot.cpp."));
}

// Le pregunta a cada dirección del I2C si hay alguien. El TCS34725 contesta en
// la 0x29. Si no sale nada, el problema es de cables (SDA en A4, SCL en A5,
// VIN y GND); si sale otra dirección, es otro sensor.
static void buscarI2C() {
  int encontrados = 0;
  for (uint8_t dir = 1; dir < 127; dir++) {
    Wire.beginTransmission(dir);
    if (Wire.endTransmission() == 0) {
      Serial.print(F("Hay algo en 0x"));
      Serial.println(dir, HEX);
      encontrados++;
    }
  }
  if (encontrados == 0) Serial.println(F("Nada en el I2C: revisa SDA (A4), SCL (A5), VIN y GND del sensor."));
}

// Para buscar cables: los pines que pueden tener un eco o un switch van de D2 a
// A3 (en el Uno, A0 a A3 son los pines 14 a 17). Fuera quedan D0 y D1 (el
// serial), A4 y A5 (el I2C) y D7 (el servo). Se recorren con un ciclo y no con
// un arreglo: cada arreglo fijo se copia a la RAM, y en el Uno no sobra.
static bool esLibre(uint8_t p) { return p >= 2 && p <= A3 && p != 7; }
static bool esTrig(uint8_t p) { return p == 2 || p == 9 || p == 5; }   // los TRIG del esquemático
static bool esUltrasonico(uint8_t p) { return esTrig(p) || p == 3 || p == 10 || p == 4; }

static void imprimirPin(uint8_t p) {
  if (p >= A0) {
    Serial.print('A');
    Serial.print(p - A0);
  } else {
    Serial.print('D');
    Serial.print(p);
  }
}

// Dispara cada TRIG del esquemático y escucha en todos los demás pines libres.
// Solo se manejan como salida los TRIG, que ya lo son en el robot: escuchar no
// arriesga nada. Los demás llevan pull-up, para que un pin al aire no parezca eco.
static void ultrasonicos() {
  for (uint8_t p = 0; p <= A3; p++)
    if (esLibre(p) && !esTrig(p)) pinMode(p, INPUT_PULLUP);
  for (uint8_t t = 0; t <= A3; t++) {
    if (!esTrig(t)) continue;
    Serial.print(F("TRIG "));
    imprimirPin(t);
    Serial.print(F(":"));
    bool alguno = false;
    for (uint8_t e = 0; e <= A3; e++) {
      if (!esLibre(e) || esTrig(e)) continue;
      delay(60);                          // que se apague el eco anterior
      digitalWrite(t, LOW);
      delayMicroseconds(2);
      digitalWrite(t, HIGH);
      delayMicroseconds(10);
      digitalWrite(t, LOW);
      unsigned long us = pulseIn(e, HIGH, 120000UL);
      if (us > 0) {
        Serial.print(F(" eco en "));
        imprimirPin(e);
        Serial.print(F(" ("));
        Serial.print(us / 58);
        Serial.print(F(" cm)"));
        alguno = true;
      }
    }
    if (!alguno) Serial.print(F(" nadie contesta: revisa VCC, GND, TRIG y ECHO de ese sensor"));
    Serial.println();
  }
  pinMode(3, INPUT);                       // los ECO del robot, como los deja hw::iniciar
  pinMode(10, INPUT);
  pinMode(4, INPUT);
  Serial.println(F("El codigo espera: TRIG D2 con eco D3, TRIG D9 con eco D10, TRIG D5 con eco D4."));
}

// 20 segundos diciendo qué pin cambia. Un switch entre un pin y GND se lee
// "GND" cerrado y "suelto" abierto (el pin tiene pull-up).
static bool esSwitch(uint8_t p) { return esLibre(p) && !esUltrasonico(p); }   // D6, D8, D11-D13, A0-A3

static void switches() {
  bool antes[A3 + 1];
  Serial.print(F("Ahorita en GND:"));
  for (uint8_t p = 0; p <= A3; p++) {
    if (!esSwitch(p)) continue;
    pinMode(p, INPUT_PULLUP);
    antes[p] = digitalRead(p);
    if (antes[p] == LOW) {
      Serial.print(' ');
      imprimirPin(p);
    }
  }
  Serial.println();
  Serial.println(F("Mueve los switches y aprieta el de la pelota (20 s)..."));
  unsigned long inicio = millis();
  while (millis() - inicio < 20000) {
    for (uint8_t p = 0; p <= A3; p++) {
      if (!esSwitch(p)) continue;
      bool ahora = digitalRead(p);
      if (ahora != antes[p]) {
        imprimirPin(p);
        Serial.println(ahora == LOW ? F(": suelto -> GND") : F(": GND -> suelto"));
        antes[p] = ahora;
      }
    }
    delay(20);
  }
  Serial.println(F("Listo. El codigo espera: switch de pista en D12 y el de la pelota (que tambien es el boton) en A3."));
}

static void avanzar() {
  hw::avanzar(30);
  Serial.println(F("Mide cuantos cm avanzo de verdad (d):"));
  Serial.println(F("- De reversa: INVERTIR_IZQUIERDO e INVERTIR_DERECHO en true (motores.ino)."));
  Serial.println(F("- Giro en su lugar: cambia uno de los dos INVERTIR y vuelve a probar."));
  Serial.println(F("- Derecho: CM_POR_S nuevo = CM_POR_S x d / 30 (hardware_robot.cpp)."));
  Serial.println(F("- Se fue chueco: AJUSTE_DER (+ acelera la llanta derecha)."));
}

static void girar() {
  hw::girar(90);
  Serial.println(F("Debe girar a la DERECHA."));
  Serial.println(F("- Giro a la izquierda: A_ES_IZQUIERDO = false (motores.ino) y prueba a otra vez."));
  Serial.println(F("- Giro de mas o de menos: mide con v."));
}

static void vueltas() {
  hw::girar(4 * 360);
  Serial.println(F("Cuenta cuantas vueltas dio de verdad (n):"));
  Serial.println(F("MS_POR_GRADO nuevo = MS_POR_GRADO x 4 / n (hardware_robot.cpp)."));
}

static void compuerta() {
  static bool abierta = false;
  abierta = !abierta;
  hw::garra(abierta);
  Serial.println(abierta ? F("Abierta (GARRA_ABIERTA)") : F("Cerrada (GARRA_CERRADA)"));
  Serial.println(F("Si no queda bien, cambia esos grados en hardware_robot.cpp."));
}

static void leds() {
  for (int c = CIAN; c < NUM_COLORES; c++) {
    hw::mostrarColor((Color)c);
    delay(1000);
  }
}

void modoPruebas() {
  ayuda();
  while (true) {
    if (!Serial.available()) continue;
    char letra = Serial.read();
    switch (letra) {
      case 's': sensores(); break;
      case 'i': buscarI2C(); break;
      case 'u': ultrasonicos(); break;
      case 'w': switches(); break;
      case 'a': avanzar(); break;
      case 'g': girar(); break;
      case 'v': vueltas(); break;
      case 'c': compuerta(); break;
      case 'l': leds(); break;
      case '\r':
      case '\n': break;
      default: ayuda();
    }
  }
}
