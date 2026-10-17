// explorador.cpp
#include "explorador.h"
#include "colores.h"
#include "config.h"
#include "hardware.h"
#include <stdlib.h>

Explorador explorador;


// Para los mensajes: coordenadas contando desde la salida, que queda en (0, 0)
static int rel(int v) { return v - TAM_MAPA / 2; }

bool Explorador::dentro(int cx, int cy) {
  return cx >= 0 && cx < TAM_MAPA && cy >= 0 && cy < TAM_MAPA;
}

// Una pared la comparten dos casillas: se anota de los dos lados.
void Explorador::ponerLado(int cx, int cy, int d, Lado valor) {
  lado[cx][cy][d] = valor;
  int vx = cx + DX[d];
  int vy = cy + DY[d];
  if (dentro(vx, vy)) lado[vx][vy][opuesta(d)] = valor;
}

static Lado segunDistancia(int cm) { return cm < DIST_PARED ? PARED : LIBRE; }

// pared[x][y][rumbo] <- distanciaFrente < 20 cm, y lo mismo con los dos lados.
// El sensor derecho ve hacia (rumbo + 1) y el izquierdo hacia (rumbo + 3).
void Explorador::anotarParedes() {
  ponerLado(x, y, rumbo, segunDistancia(hw::distFrente()));
  ponerLado(x, y, aLaDerecha(rumbo), segunDistancia(hw::distDerecha()));
  ponerLado(x, y, aLaIzquierda(rumbo), segunDistancia(hw::distIzquierda()));
}

// En la rampa el robot va inclinado y no confiamos en los sensores. El
// reglamento dice que la rampa tiene paredes a los lados: es un pasillo.
void Explorador::anotarRampa() {
  rampa[x][y] = true;
  ponerLado(x, y, rumbo, LIBRE);
  ponerLado(x, y, aLaDerecha(rumbo), PARED);
  ponerLado(x, y, aLaIzquierda(rumbo), PARED);
}

// Cruzar la rampa vale 20 puntos, pero solo si sube y baja. Si la salida de la
// rampa da a una casilla que ya conoce, el DFS normal se regresaría sin
// cruzarla (le pasó en el laberinto inventado del simulador). Por eso en la
// rampa se sigue derecho hasta salir, aunque del otro lado ya haya pasado. La
// casilla de salida entra a la pila como cualquier otra, así el regreso sigue
// funcionando igual.
bool Explorador::seguirPorLaRampa() {
  int d = rumbo;
  int vx = x + DX[d];
  int vy = y + DY[d];
  if (lado[x][y][d] != LIBRE || !dentro(vx, vy)) return false;
  if (vioFin && vx == finX && vy == finY && !permitirRojo) return false;
  if (!visitada[vx][vy]) return false;   // si no la conoce, la explora el paso normal (asomándose)
  avanzarCasilla();
  pila[tope++] = d;
  x = vx;
  y = vy;
  ultimaDireccion = d;
  anotarEnRecorrido();
  return true;
}

void Explorador::anotarEnRecorrido() {
  if (largoRecorrido < MAX_RECORRIDO) {
    recX[largoRecorrido] = x;
    recY[largoRecorrido] = y;
    largoRecorrido++;
  }
}

int Explorador::casillasVisitadas() {
  int n = 0;
  for (int i = 0; i < TAM_MAPA; i++)
    for (int j = 0; j < TAM_MAPA; j++)
      if (visitada[i][j]) n++;
  return n;
}

void Explorador::empezar() {
  for (int i = 0; i < TAM_MAPA; i++) {
    for (int j = 0; j < TAM_MAPA; j++) {
      visitada[i][j] = false;
      rampa[i][j] = false;
      for (int d = 0; d < 4; d++) lado[i][j][d] = NO_SE;
    }
  }
  for (int c = 0; c < NUM_COLORES; c++) coloresVistos[c] = false;
  tope = 0;
  largoRecorrido = 0;
  vioFin = false;
  enFin = false;
  tengoPelota = false;
  regresando = false;
  ultimaDireccion = ARRIBA;
  x = TAM_MAPA / 2;
  y = TAM_MAPA / 2;
  rumbo = ARRIBA;
  visitada[x][y] = true;
  anotarEnRecorrido();

  // El robot no tiene sensor atrás, así que en la salida nadie ha visto lo que
  // tiene a la espalda. Lee enfrente y a los lados, gira a la derecha, y ahora
  // su sensor derecho ve lo que tenía atrás.
  anotarParedes();
  girarHacia(rumbo, aLaDerecha(rumbo));
  anotarParedes();
}

bool Explorador::sePuedeExplorar(int cx, int cy) {
  if (!dentro(cx, cy) || visitada[cx][cy]) return false;
  if (vioFin && cx == finX && cy == finY && !permitirRojo) return false;
  return true;
}

// Un vecino sin pared y sin visitar. Primero enfrente, luego a la derecha,
// luego a la izquierda y al último atrás: así gira lo menos posible.
int Explorador::vecinoAbierto() {
  const int orden[4] = { rumbo, aLaDerecha(rumbo), aLaIzquierda(rumbo), opuesta(rumbo) };
  for (int i = 0; i < 4; i++) {
    int d = orden[i];
    if (lado[x][y][d] == LIBRE && sePuedeExplorar(x + DX[d], y + DY[d])) return d;
  }
  return NINGUNA;
}

// ¿Queda alguna casilla ya vista con un lado libre hacia algo que no conoce?
// Si ya no queda ninguna, ya exploró todo: no tiene caso regresar hasta la
// salida para vaciar la pila.
bool Explorador::quedaPorExplorar() {
  for (int cx = 0; cx < TAM_MAPA; cx++) {
    for (int cy = 0; cy < TAM_MAPA; cy++) {
      if (!visitada[cx][cy]) continue;
      for (int d = 0; d < 4; d++) {
        if (lado[cx][cy][d] == LIBRE && sePuedeExplorar(cx + DX[d], cy + DY[d])) return true;
      }
    }
  }
  return false;
}

// Moverse a una casilla vecina que ya conoce. Si le queda justo atrás, se va de
// reversa en lugar de dar media vuelta: es más rápido y no gira sobre la rampa.
void Explorador::moverA(int d) {
  if (d == opuesta(rumbo)) {
    retrocederCasilla();
  } else {
    girarHacia(rumbo, d);
    avanzarCasilla();
  }
  x += DX[d];
  y += DY[d];
  ultimaDireccion = d;
}

// La pelota mide 4.2 cm y las paredes 15. El sensor bajito ve las dos cosas;
// el de paredes (más alto) solo ve la pared. Bajo cerca y alto lejos: pelota.
// Sin sensor bajito la pelota no se ve: se detecta al entrar al pocket (paso()).
bool Explorador::pelotaEnfrente() {
  return HAY_SENSOR_BAJO && hw::distBaja() < PELOTA_BAJO_MAX && hw::distFrente() > PELOTA_ALTO_MIN;
}

bool Explorador::paso() {
  if (!regresando && rampa[x][y] && seguirPorLaRampa()) return true;

  if (!quedaPorExplorar()) {
    hw::mensaje(TXT("Ya no queda nada por explorar (%d casillas vistas)"),
                casillasVisitadas());
    return false;
  }

  int d = vecinoAbierto();
  if (d == NINGUNA) {
    // Callejón sin salida: pop, y regresar una casilla por donde vino
    if (tope == 0) return false;
    if (!regresando) {
      hw::mensaje(TXT("Callejón en (%d, %d): regreso por donde vine"), rel(x), rel(y));
    }
    regresando = true;
    tope--;
    moverA(opuesta(pila[tope]));
    anotarEnRecorrido();
    return true;
  }
  regresando = false;

  int vx = x + DX[d];
  int vy = y + DY[d];
  girarHacia(rumbo, d);

  if (buscarPelota && !tengoPelota && pelotaEnfrente()) {
    hw::mensaje(TXT("Veo la pelota enfrente: la tomo y la saco de su casilla"));
    tengoPelota = agarrarPelota(MAX_AVANCE_PELOTA);
    visitada[vx][vy] = true;           // su casilla es un callejón: ya no hace falta entrar
    hw::mensaje(tengoPelota ? TXT("Tengo la pelota") : TXT("Se me fue la pelota"));
    return true;
  }

  asomarse();
  Color c = leerColor();
  if (c == ROJO && !permitirRojo) {
    vioFin = true;
    finX = vx;
    finY = vy;
    echarseParaAtras();
    hw::mensaje(TXT("Me asomé al rojo en (%d, %d): lo anoto y lo dejo para el final"),
                rel(vx), rel(vy));
    return true;
  }

  terminarDeEntrar();
  pila[tope++] = d;                    // push: por aquí entró
  x = vx;
  y = vy;
  ultimaDireccion = d;
  visitada[x][y] = true;
  anotarEnRecorrido();

  // Sin sensor bajito la compuerta va abierta en la sección 1 (pista_b.cpp). Si
  // al entrar la pelota quedó en el pocket, el micro switch del fondo ya está
  // apretado: se cierra la compuerta y la pelota queda adentro.
  if (buscarPelota && !tengoPelota && hw::pelotaEnGarra()) {
    hw::garra(false);
    tengoPelota = hw::pelotaEnGarra();
    hw::mensaje(tengoPelota ? TXT("La pelota entró al pocket: cierro la compuerta")
                            : TXT("Se apretó el switch, pero la pelota se fue"));
  }

  if (c == ROJO) {
    enFin = true;
    return true;
  }
  if (esColorDePunto(c) && !coloresVistos[c]) {
    coloresVistos[c] = true;
    hw::mostrarColor(c);
    hw::mensaje(TXT("Color nuevo: %s en (%d, %d), lo muestro con el LED"),
                nombreColor(c), rel(x), rel(y));
  }
  if (abs(hw::inclinacion()) > GRADOS_RAMPA) anotarRampa();
  else anotarParedes();
  return true;
}

// BFS: la ruta más corta desde donde está hasta el destino, pasando solo por
// casillas que ya conoce y por lados que sabe que están libres.
bool Explorador::irA(int destinoX, int destinoY) {
  signed char vino[TAM_MAPA][TAM_MAPA];     // por qué rumbo llegó el BFS a cada casilla
  for (int i = 0; i < TAM_MAPA; i++)
    for (int j = 0; j < TAM_MAPA; j++) vino[i][j] = -1;   // -1 = todavía no llega

  // Cada casilla entra a la cola como un solo número: x * TAM_MAPA + y. Es como
  // acomodar la matriz en un solo renglón, y ahorra un arreglo entero de RAM.
  // Para regresar: x = número / TAM_MAPA, y = número % TAM_MAPA.
  unsigned char cola[TAM_MAPA * TAM_MAPA];
  int leer = 0, escribir = 0;
  cola[escribir++] = x * TAM_MAPA + y;
  vino[x][y] = 4;                           // de aquí sale el BFS

  while (leer < escribir) {
    int cx = cola[leer] / TAM_MAPA;
    int cy = cola[leer] % TAM_MAPA;
    leer++;
    if (cx == destinoX && cy == destinoY) break;
    for (int d = 0; d < 4; d++) {
      if (lado[cx][cy][d] != LIBRE) continue;
      int nx = cx + DX[d];
      int ny = cy + DY[d];
      if (!dentro(nx, ny) || vino[nx][ny] != -1) continue;
      bool esDestino = (nx == destinoX && ny == destinoY);
      if (!visitada[nx][ny] && !esDestino) continue;
      vino[nx][ny] = d;
      cola[escribir++] = nx * TAM_MAPA + ny;
    }
  }
  if (vino[destinoX][destinoY] == -1) return false;

  // La ruta se arma del destino hacia atrás y se recorre al derecho. Se guarda
  // en el arreglo de la cola, que ya no se usa: son 81 bytes menos de RAM.
  unsigned char* ruta = cola;
  int largo = 0;
  int cx = destinoX, cy = destinoY;
  while (cx != x || cy != y) {
    int d = vino[cx][cy];
    ruta[largo++] = d;
    cx -= DX[d];
    cy -= DY[d];
  }
  hw::mensaje(TXT("Voy a (%d, %d) por la ruta más corta (BFS): %d casillas"),
              rel(destinoX), rel(destinoY), largo);
  for (int i = largo - 1; i >= 0; i--) {
    moverA(ruta[i]);
    anotarEnRecorrido();
  }
  if (vioFin && x == finX && y == finY) enFin = true;
  return true;
}

// Bonus 1: las mismas casillas en orden inverso. Es leer la lista del
// recorrido del final al principio.
bool Explorador::regresarPorElMismoCamino() {
  for (int i = largoRecorrido - 2; i >= 0; i--) {
    int d = NINGUNA;
    for (int k = 0; k < 4; k++) {
      if (x + DX[k] == recX[i] && y + DY[k] == recY[i]) d = k;
    }
    if (d == NINGUNA) return false;         // no son vecinas: algo se desacomodó
    moverA(d);
  }
  return true;
}
