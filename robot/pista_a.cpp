// pista_a.cpp
// Pista A: el laberinto de 5 x 5 (reglamento 3.1, propuesta "Pista A").
//
// 1. Explorar todo con el DFS, mostrando cada color nuevo con el LED.
//    La casilla roja se anota pero no se pisa: al pisarla se acaba la ronda.
// 2. Ir al rojo por la ruta más corta (BFS).
// 3. Bonus 1: regresar por las mismas casillas en orden inverso, si da tiempo.

#include "pista_a.h"
#include "explorador.h"
#include "config.h"
#include "hardware.h"


void correrPistaA() {
  Explorador& e = explorador;
  e.buscarPelota = false;
  e.permitirRojo = false;
  e.empezar();
  hw::mensaje(TXT("Pista A: exploro todo el laberinto y dejo el rojo para el final"));

  // 1. Explorar mientras quede algo nuevo y no se acabe el tiempo de explorar
  while (hw::ms() < MS_EXPLORAR && e.paso()) {
  }

  // 2. Llegar al rojo
  if (e.vioFin) {
    e.irA(e.finX, e.finY);
  } else {
    hw::mensaje(TXT("Todavía no veo el rojo: sigo explorando, ahora sí puedo entrar"));
    e.permitirRojo = true;
    while (!e.enFin && hw::ms() < MS_RONDA && e.paso()) {
    }
  }
  if (!e.enFin) {
    hw::mensaje(TXT("No encontré el rojo"));
    return;
  }
  hw::mensaje(TXT("Llegué al rojo"));

  // 3. Bonus 1 (35 puntos). Si se queda a medias se cancela, así que solo se
  //    intenta si el tiempo alcanza para todo el regreso. El regreso son las
  //    mismas casillas que ya recorrió, pero sin asomarse ni leer paredes: tarda
  //    menos de lo que lleva la ronda. Si le queda más tiempo del que ya usó,
  //    le alcanza. Así no hay que adivinar la velocidad del robot.
  unsigned long ahora = hw::ms();
  unsigned long queda = ahora < MS_RONDA ? MS_RONDA - ahora : 0;
  hw::mensaje(TXT("Bonus 1: el regreso es de %d casillas; llevo %lu s y quedan %lu s"),
              e.largoRecorrido - 1, ahora / 1000, queda / 1000);
  if (ahora < queda && e.largoRecorrido < MAX_RECORRIDO) {
    if (e.regresarPorElMismoCamino()) hw::mensaje(TXT("De regreso en la salida"));
    else hw::mensaje(TXT("El recorrido no cuadra: me detengo"));
  } else {
    hw::mensaje(TXT("No alcanza el tiempo: me quedo en el rojo"));
  }

  // Bonus 2 (ArUco, 30 puntos): pendiente. Pide una cámara y un procesador que
  // corra visión; va al final porque, si no sale, no le quita puntos a nada más.
}
