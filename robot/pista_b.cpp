// pista_b.cpp
// Pista B: tres secciones seguidas, con la pelota en la garra todo el camino
// (reglamento 3.2, propuesta "Pista B").
//
// Es una máquina de estados: una variable dice en qué sección va el robot y
// llegar a una casilla roja lo pasa a la siguiente.

#include "pista_b.h"
#include "explorador.h"
#include "colores.h"
#include "config.h"
#include "hardware.h"


// ================================================================== Sección 1
// Encontrar la pelota escondida en el centro del cuadro de 3 x 3 y llevarla al
// checkpoint. No hay código especial: es el mismo DFS del laberinto con otro
// objetivo, y resuelve las 4 posiciones de las paredes sin un solo caso aparte.
static void seccion1() {
  Explorador& e = explorador;
  e.buscarPelota = true;
  e.permitirRojo = false;       // con pelota el checkpoint vale 70; sin pelota, 40
  e.empezar();
  hw::mensaje(TXT("Sección 1: busco la pelota con el mismo DFS del laberinto"));
  // Sin sensor bajito la pelota no se ve antes de llegar: se explora con la
  // compuerta abierta y se detecta cuando entra al pocket (explorador.cpp).
  if (!HAY_SENSOR_BAJO) hw::garra(true);

  while (!e.tengoPelota && hw::ms() < MS_RONDA / 2 && e.paso()) {
  }
  if (!e.tengoPelota) {
    hw::garra(false);
    hw::mensaje(TXT("No encontré la pelota: voy al checkpoint sin ella"));
  }

  if (e.vioFin) {
    e.irA(e.finX, e.finY);
  } else {
    e.permitirRojo = true;
    while (!e.enFin && e.paso()) {
    }
  }
  // Quedar viendo hacia la sección 2 (pudo haber entrado de reversa)
  girarHacia(e.rumbo, e.ultimaDireccion);
  hw::mensaje(TXT("Checkpoint 1"));
}

// ================================================================== Sección 2
// Cruzar la cuadrícula verde (2 x 4 casillas) sin pisar las líneas blancas.
// El robot lleva la cuenta de dónde va, en cm, desde el inicio de la sección:
// "avance" hacia adelante y "lateral" hacia los lados (+ es a la izquierda,
// 0 es el centro).
static int avance;
static int lateral;

static void adelante(int cm) {
  hw::avanzar(cm);
  avance += cm;
}

// Moverse de lado sin cambiar hacia dónde ve: girar, avanzar y girar de regreso
static void deLado(int cm) {
  if (cm == 0) return;
  int giro = cm > 0 ? -90 : 90;       // a la izquierda: primero gira a la izquierda
  hw::girar(giro);
  hw::avanzar(cm > 0 ? cm : -cm);
  hw::girar(-giro);
  lateral += cm;
}

static bool hayLinea() { return hw::lineaIzquierda() || hw::lineaDerecha(); }

// Se topó con una línea: buscar el hueco de 30 cm moviéndose de lado.
// El hueco puede quedar en cualquier altura, no siempre en una fila completa
// (la primera línea de la imagen 2.4 lo tiene a media altura).
static void rodearLinea() {
  bool izq = hw::lineaIzquierda();
  bool der = hw::lineaDerecha();

  // Si la ve un solo sensor, el hueco está del otro lado. Si la ven los dos,
  // se va hacia donde haya más espacio.
  int haciaDonde;
  if (izq && !der) haciaDonde = -1;
  else if (der && !izq) haciaDonde = +1;
  else haciaDonde = (lateral <= 0) ? +1 : -1;

  hw::mensaje(TXT("Línea a %d cm (la ve: %s): busco el hueco hacia la %s"),
              avance + 10, (izq && der) ? "los dos sensores" : (izq ? "el izquierdo" : "el derecho"),
              haciaDonde > 0 ? "izquierda" : "derecha");

  adelante(-RETROCESO_LINEA);          // atrás, para poder girar sin pisarla
  bool encontrado = false;
  for (int intento = 0; intento < 12 && !encontrado; intento++) {
    int siguiente = lateral + haciaDonde * PASO_LATERAL;
    if (siguiente > LATERAL_MAX || siguiente < -LATERAL_MAX) haciaDonde = -haciaDonde;  // se acabó la sección
    deLado(haciaDonde * PASO_LATERAL);
    adelante(RETROCESO_LINEA);         // regresa hasta la línea: los sensores quedan encima
    izq = hw::lineaIzquierda();
    der = hw::lineaDerecha();
    if (!izq && !der) {
      encontrado = true;
    } else {
      adelante(-RETROCESO_LINEA);
      if (izq && !der) haciaDonde = -1;
      else if (der && !izq) haciaDonde = +1;
    }
  }
  if (!encontrado) {
    hw::mensaje(TXT("No encontré el hueco (lack of progress)"));
    return;
  }

  // Un poco más hacia el mismo lado, para que las llantas pasen holgadas
  int margen = haciaDonde * MARGEN_LATERAL;
  if (lateral + margen > LATERAL_MAX || lateral + margen < -LATERAL_MAX) margen = 0;
  if (margen != 0) {
    adelante(-RETROCESO_LINEA);
    deLado(margen);
    adelante(RETROCESO_LINEA);
  }
  // Cruzar: hasta que las llantas de atrás pasen la línea
  adelante(LARGO_ROBOT + 4);
}

static void seccion2() {
  hw::mensaje(TXT("Sección 2: cruzo sin pisar las líneas blancas"));
  avance = -CM_CASILLA / 2;            // el centro del checkpoint queda 15 cm antes de la sección
  lateral = 0;

  // Salir del checkpoint. La franja blanca que separa el rojo del verde se ve
  // igual que una línea: hasta que el sensor de color vea verde, los sensores
  // de línea no cuentan.
  while (leerColorUnaVez() != VERDE && avance < CM_CASILLA) adelante(1);

  // Cruzar. Las líneas van en las divisiones entre casillas, así que la
  // última puede estar a 90 cm del inicio.
  while (avance < X_RECENTRAR) {
    if (hayLinea()) rodearLinea();
    else adelante(1);
  }

  // Ya pasó las líneas: volver al centro para entrar derecho al checkpoint 2,
  // que mide 30 cm y está centrado (de ahí arranca la sección 3).
  deLado(-lateral);
  while (leerColorUnaVez() != ROJO && avance < SECCION2_LARGO + CM_CASILLA) adelante(1);
  adelante(CM_CASILLA / 2 + SENSOR_COLOR_ADELANTE);   // hasta el centro del checkpoint
  hw::mensaje(TXT("Checkpoint 2"));
}

// ================================================================== Sección 3
// Cada color dice a qué casilla pasar, en rumbos de la pista vista desde el
// checkpoint (no del robot): naranja arriba, cian derecha, rosa abajo,
// amarillo izquierda.
static int direccionDeColor(Color c) {
  switch (c) {
    case NARANJA: return ARRIBA;
    case CIAN: return DERECHA;
    case ROSA: return ABAJO;
    case AMARILLO: return IZQUIERDA;
    default: return NINGUNA;
  }
}

static const char* nombreRumbo(int d) {
  static const char* nombres[4] = { "arriba", "derecha", "abajo", "izquierda" };
  return (d >= 0 && d < 4) ? nombres[d] : "?";
}

static const char* nombreGiro(int giro) {
  static const char* nombres[4] = { "recto", "derecha", "media vuelta", "izquierda" };
  return nombres[giro];
}

static void seccion3() {
  hw::mensaje(TXT("Sección 3: sigo los colores"));
  int rumbo = ARRIBA;                  // al salir del checkpoint ve hacia "arriba"
  avanzarCasilla();
  for (int paso = 1; paso <= 30; paso++) {
    Color c = leerColor();             // 5 lecturas en el centro de la casilla
    if (c == VERDE) {
      hw::mensaje(TXT("Llegué al fin"));
      return;
    }
    int nuevo = direccionDeColor(c);
    if (nuevo == NINGUNA) {
      hw::mensaje(TXT("Me salí del camino (lack of progress)"));
      return;
    }
    int giro = (nuevo - rumbo + 4) % 4;
    hw::mensaje(TXT("Paso %d: %s. Rumbo %d (%s) -> %d (%s). (%d - %d + 4) mod 4 = %d: %s"),
                paso, nombreColor(c), rumbo, nombreRumbo(rumbo), nuevo, nombreRumbo(nuevo),
                nuevo, rumbo, giro, nombreGiro(giro));
    girarHacia(rumbo, nuevo);
    avanzarCasilla();
  }
}

// ================================================================== Arranque
void correrPistaB() {
  // ¿En qué sección estoy? Se deduce del piso. Así funciona igual al empezar
  // la ronda que después de un lack of progress, cuando el reinicio borra la
  // memoria y el capitán pone el robot en el último checkpoint.
  int seccion = 1;
  if (leerColor() == ROJO) {
    hw::mensaje(TXT("Arranco en un checkpoint: tomo la pelota que me pusieron enfrente"));
    agarrarPelota(8);
    asomarse();
    Color enfrente = leerColor();
    echarseParaAtras();
    seccion = (enfrente == VERDE) ? 2 : 3;
  }
  hw::mensaje(TXT("Empiezo en la sección %d"), seccion);

  if (seccion == 1) {
    seccion1();
    seccion = 2;
  }
  if (seccion == 2) {
    seccion2();
    seccion = 3;
  }
  seccion3();
}
