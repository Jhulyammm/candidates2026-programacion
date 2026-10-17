// colores.h
// Leer el color del piso: calibrar en los 2 minutos de cada ronda y comparar
// cada lectura contra esas referencias. Los tonos cambian de pista a pista, por
// eso no hay umbrales fijos.

#pragma once
#include "base.h"

extern Calibracion calibracion;

Color clasificar(Rgb lectura);      // el color de la referencia más parecida
Color leerColorUnaVez();            // una sola lectura: rápida, para ir avanzando
Color leerColor();                  // 5 lecturas y gana la mayoría: para decidir
void calibrarColores(bool pistaB);  // uno por uno, con el botón
