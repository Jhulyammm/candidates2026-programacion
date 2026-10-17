// diagnostico_motores.ino
// Programa de diagnóstico para el Arduino de MOTORES (U4). NO es el de la ronda:
// después de usarlo se vuelve a subir motores/motores.ino.
//
// Cada 2 segundos dice por su USB qué hay conectado a su I2C (A4 y A5). El
// segundo sensor de color (TCS34725) debe salir en 0x29. Los motores se quedan
// apagados todo el tiempo.
//
// Está aparte, y no dentro de motores.ino, porque lo que imprime este Arduino
// también le llega al de sensores si el cable de regreso está conectado, y ahí
// unas letras sueltas podrían tomarse como comandos del modo de pruebas.

#include <Wire.h>

void setup() {
  for (int p = 5; p <= 10; p++) {      // IN4, IN3, IN2, IN1, ENA y ENB del L298N: apagados
    pinMode(p, OUTPUT);
    digitalWrite(p, LOW);
  }
  Serial.begin(115200);
  Wire.begin();
  Wire.setWireTimeout(3000, true);
}

void loop() {
  Serial.print(F("Arduino de motores, I2C:"));
  int encontrados = 0;
  for (uint8_t dir = 1; dir < 127; dir++) {
    Wire.beginTransmission(dir);
    if (Wire.endTransmission() == 0) {
      Serial.print(F(" 0x"));
      Serial.print(dir, HEX);
      encontrados++;
    }
  }
  if (encontrados == 0) Serial.print(F(" nada (revisa SDA en A4, SCL en A5, VIN y GND del sensor 2)"));
  Serial.println();
  delay(2000);
}
