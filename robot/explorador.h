// explorador.h
// El explorador: búsqueda en profundidad (DFS) con pila.
// Lo usan la pista A (el laberinto) y la sección 1 de la pista B (buscar la
// pelota). En la propuesta del 19 sep: "Un solo explorador para dos retos".

#pragma once
#include "base.h"

// El laberinto mide 5 x 5, pero el robot no sabe en qué orilla arranca. Por eso
// empieza en el centro de un mapa de 9 x 9: así cabe el laberinto hacia
// cualquier lado.
const int TAM_MAPA = 9;
// El recorrido más largo en el simulador fue de 51 casillas (imagen 1.1). En un
// 5 x 5 no pasa de unas 75: el DFS cruza cada pasillo a lo mucho dos veces (48)
// y el BFS al rojo son 24 más. Era 200 en el Mega; en el Uno cada casilla de
// más cuesta 2 bytes de RAM.
const int MAX_RECORRIDO = 100;

enum Lado { NO_SE, LIBRE, PARED };

class Explorador {
public:
  // ---- La memoria del robot. Empieza vacía cada ronda: nada de pre-mapeo.
  unsigned char lado[TAM_MAPA][TAM_MAPA][4];  // qué hay en cada lado de cada casilla
  bool visitada[TAM_MAPA][TAM_MAPA];
  bool rampa[TAM_MAPA][TAM_MAPA];             // casillas donde quedó inclinado
  unsigned char pila[TAM_MAPA * TAM_MAPA];    // por qué lado entró a cada casilla del camino
  int tope;                                   // cuántas hay en la pila
  unsigned char recX[MAX_RECORRIDO];          // el recorrido: cada casilla que pisa, en
  unsigned char recY[MAX_RECORRIDO];          // orden (para el bonus 1 de la pista A)
  int largoRecorrido;
  bool coloresVistos[NUM_COLORES];

  int x, y, rumbo;              // dónde cree que está y hacia dónde ve
  int ultimaDireccion;          // hacia dónde se movió la última vez
  bool vioFin;                  // ¿ya vio la casilla roja?
  int finX, finY;
  bool enFin;                   // ya está parado en la roja

  // ---- Reglas que pone cada pista
  bool permitirRojo;            // ¿puede entrar a la roja? Al principio no
  bool buscarPelota;            // sección 1 de la pista B
  bool tengoPelota;

  void empezar();               // memoria vacía, en (4, 4) viendo ARRIBA
  bool paso();                  // un paso del DFS; false = ya no queda nada que explorar
  bool irA(int destinoX, int destinoY);   // ruta más corta (BFS) por lo que ya conoce
  bool regresarPorElMismoCamino();        // el recorrido, leído al revés
  int casillasVisitadas();

private:
  bool regresando;              // para avisar una sola vez por callejón

  bool dentro(int cx, int cy);
  void ponerLado(int cx, int cy, int d, Lado valor);
  void anotarParedes();
  void anotarRampa();
  bool seguirPorLaRampa();
  bool sePuedeExplorar(int cx, int cy);
  int vecinoAbierto();
  bool quedaPorExplorar();
  void moverA(int d);
  void anotarEnRecorrido();
  bool pelotaEnfrente();
};

extern Explorador explorador;
