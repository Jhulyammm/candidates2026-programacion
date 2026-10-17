// robot.ino
// RoBorregos Candidates 2026, categoría Principiantes. BORRADOR.
//
// Al encender:
//   0. Si desde la computadora escriben "p" en los primeros 3 s, entra al modo
//      de pruebas (pruebas.cpp) y ya no corre ninguna pista.
//   1. Si se prende con el botón apretado, calibra los colores (2 minutos de
//      calibración). Si no, carga la última calibración de la EEPROM.
//   2. El switch dice qué pista es.
//   3. Espera el botón para arrancar la ronda.
//
// Después de un lack of progress el capitán reinicia el robot siempre igual:
// apagar, prender y botón. La pista B deduce sola en qué sección quedó.

#include "config.h"
#include "base.h"
#include "hardware.h"
#include "colores.h"
#include "pista_a.h"
#include "pista_b.h"
#include "pruebas.h"

void setup() {
  hw::iniciar();
  if (quierenModoPruebas()) modoPruebas();
  bool pistaB = hw::selectorPistaB();

  if (hw::botonPresionado() || !hw::cargarCalibracion(calibracion)) {
    while (hw::botonPresionado()) {
    }
    calibrarColores(pistaB);
  }

  hw::mensaje(pistaB ? TXT("Pista B. Botón para arrancar.") : TXT("Pista A. Botón para arrancar."));
  hw::esperarBoton();
  hw::reiniciarReloj();

  if (pistaB) correrPistaB();
  else correrPistaA();
  hw::mensaje(TXT("Terminé"));
}

void loop() {
}
