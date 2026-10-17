// hardware.h
// La frontera entre la lógica y el robot.
//
// La lógica (pistas, explorador, colores) solo usa estas funciones y no sabe
// si del otro lado hay un robot o una simulación:
//   - en el robot las implementa hardware_robot.cpp (motores, sensores, LED)
//   - en la computadora las implementa simulador/mundo.cpp (una pista falsa)
// Así se puede probar la lógica sin robot, y cuando electrónica cambie una
// pieza solo se toca hardware_robot.cpp.

#pragma once
#include "base.h"

// ---- Textos de los mensajes. En el Arduino Uno la RAM es de 2 KB, y cada texto
// entre comillas se copia a la RAM aunque nunca cambie: los mensajes solos se
// comían más de 1.5 KB. TXT("...") deja el texto en la memoria de programa
// (32 KB, sobra). En la computadora TXT no hace nada.
#ifdef ARDUINO
#include <avr/pgmspace.h>
#define TXT(texto) PSTR(texto)
#else
#define TXT(texto) (texto)
#endif

namespace hw {

void iniciar();
void reiniciarReloj();              // la ronda empieza aquí
unsigned long ms();                 // milisegundos desde que empezó la ronda

// ---- Movimiento: siempre recto o en giros de 90 grados
void avanzar(int cm);               // negativo = de reversa. Va recto con las paredes
void girar(int grados);             // +90 derecha, -90 izquierda, 180 media vuelta

// ---- Distancias, en cm
int distFrente();
int distIzquierda();
int distDerecha();
int distBaja();                     // sensor bajito (a 2 cm del piso): ve la pelota

// ---- Piso
Rgb leerRgb();                      // sensor de color, sin clasificar
bool lineaIzquierda();              // sensores de piso delante de cada llanta: ¿ven blanco?
bool lineaDerecha();
int inclinacion();                  // grados de cabeceo según el IMU (0 = plano)

// ---- Garra
void garra(bool abierta);
bool pelotaEnGarra();

// ---- Lo que ve el juez y lo que vemos nosotros
void mostrarColor(Color c);         // los LEDs, que están en el Arduino de motores
void mostrarNumero(int n);          // para el ArUco: tiene que verse por lo menos 3 s
// Serial en el robot, consola en la simulación. Funciona como printf, y el
// formato SIEMPRE va con TXT: hw::mensaje(TXT("Casilla (%d, %d)"), x, y).
// Sin TXT, en el Arduino sale basura (busca el texto en la memoria equivocada).
void mensaje(const char* formato, ...);

// ---- Botón y selector
bool botonPresionado();
void esperarBoton();
bool selectorPistaB();

// ---- Lo único que se guarda de un encendido a otro: la calibración de color.
// Nada del mapa: el reglamento prohíbe el pre-mapeo.
void guardarCalibracion(const Calibracion& c);
bool cargarCalibracion(Calibracion& c);

}  // namespace hw
