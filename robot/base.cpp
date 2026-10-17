// base.cpp
#include "base.h"
#include "config.h"
#include "hardware.h"

const char* nombreColor(Color c) {
  switch (c) {
    case BLANCO: return "blanco";
    case CIAN: return "cian";
    case AMARILLO: return "amarillo";
    case NARANJA: return "naranja";
    case ROSA: return "rosa";
    case ROJO: return "rojo";
    case VERDE: return "verde";
    default: return "desconocido";
  }
}

bool esColorDePunto(Color c) {
  return c == CIAN || c == AMARILLO || c == NARANJA || c == ROSA;
}

// La misma resta sirve en todo el programa: en el laberinto, en la sección 3 y
// al regresar por el mismo camino.
//   giro = (nuevo - rumbo + 4) mod 4   ->   0 recto, 1 derecha, 2 media vuelta, 3 izquierda
void girarHacia(int& rumbo, int nuevo) {
  int giro = (nuevo - rumbo + 4) % 4;
  if (giro == 1) hw::girar(90);
  else if (giro == 2) hw::girar(180);
  else if (giro == 3) hw::girar(-90);
  rumbo = nuevo;
}

void avanzarCasilla() { hw::avanzar(CM_CASILLA); }
void retrocederCasilla() { hw::avanzar(-CM_CASILLA); }
void asomarse() { hw::avanzar(DIST_ASOMARSE); }
void terminarDeEntrar() { hw::avanzar(CM_CASILLA - DIST_ASOMARSE); }
void echarseParaAtras() { hw::avanzar(-DIST_ASOMARSE); }

bool agarrarPelota(int maxCm) {
  hw::garra(true);                      // abrir
  int avanzado = 0;
  while (!hw::pelotaEnGarra() && avanzado < maxCm) {
    hw::avanzar(1);
    avanzado++;
  }
  hw::garra(false);                     // cerrar
  bool laTengo = hw::pelotaEnGarra();
  hw::avanzar(-avanzado);               // regresa a donde estaba, ya con la pelota
  return laTengo;
}
