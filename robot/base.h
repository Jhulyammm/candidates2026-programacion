// base.h
// Lo que usan las dos pistas: rumbos, colores y moverse por casillas.
// En la propuesta del 19 sep es la parte "La base: casillas y rumbos".

#pragma once

// ------------------------------------------------------------------ Rumbos
// Son de la PISTA, no del robot: "arriba" es siempre la misma dirección aunque
// el robot esté volteado. Girar a la derecha es sumar 1 y a la izquierda restar 1,
// siempre módulo 4.
const int ARRIBA = 0;
const int DERECHA = 1;
const int ABAJO = 2;
const int IZQUIERDA = 3;
const int NINGUNA = -1;

// Cuánto cambia (x, y) al avanzar una casilla en cada rumbo
const int DX[4] = { 0, 1, 0, -1 };
const int DY[4] = { 1, 0, -1, 0 };

inline int opuesta(int d) { return (d + 2) % 4; }
inline int aLaDerecha(int d) { return (d + 1) % 4; }
inline int aLaIzquierda(int d) { return (d + 3) % 4; }

// ------------------------------------------------------------------ Colores
enum Color { BLANCO, CIAN, AMARILLO, NARANJA, ROSA, ROJO, VERDE, NUM_COLORES, DESCONOCIDO };

struct Rgb {
  int r, g, b;
};

// La calibración: una lista de referencias "este RGB es tal color". Un mismo
// color puede tener varias (el verde del inicio y el de la sección 2 no son el
// mismo tono). Se toma en los 2 minutos de calibración de cada ronda.
const int MAX_REFERENCIAS = 12;

struct Referencia {
  Color color;
  Rgb rgb;
};

struct Calibracion {
  Referencia ref[MAX_REFERENCIAS];
  int cuantas;
};

const char* nombreColor(Color c);
bool esColorDePunto(Color c);          // cian, amarillo, naranja o rosa: 5 puntos en la pista A

// ------------------------------------------------------------------ Moverse por casillas
void girarHacia(int& rumbo, int nuevo);   // gira lo necesario y actualiza el rumbo
void avanzarCasilla();                    // 30 cm hacia adelante
void retrocederCasilla();                 // 30 cm de reversa, sin girar
void asomarse();                          // avanza DIST_ASOMARSE para leer la casilla de enfrente
void terminarDeEntrar();                  // lo que le falta para llegar al centro de esa casilla
void echarseParaAtras();                  // regresa lo que avanzó al asomarse
bool agarrarPelota(int maxCm);            // abre la garra, avanza hasta tenerla, cierra y regresa
