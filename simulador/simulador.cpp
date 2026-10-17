// simulador.cpp
// Corre la lógica del robot sobre las pistas de ejemplo del reglamento y dice
// qué pasó: por dónde fue, cuánto tardó y cuántos puntos habría sacado.
//
//   compilar.bat             compila y corre todas las pruebas
//   simulador.exe A          solo la pista A
//   simulador.exe B          solo la pista B
//   simulador.exe -v         imprime también cada movimiento
//   simulador.exe --vel 25   otra velocidad del robot, en cm/s (por defecto 15)
//
// Los tiempos dependen de la velocidad que suponemos: sirven para comparar,
// no para prometer. Los puntos los calcula la simulación con la sección 4 del
// reglamento, no el robot.

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <string>
#include <utility>
#include <vector>
#include "mundo.h"
#include "colores.h"
#include "config.h"
#include "explorador.h"
#include "hardware.h"
#include "pista_a.h"
#include "pista_b.h"

#ifdef _WIN32
extern "C" __declspec(dllimport) int __stdcall SetConsoleOutputCP(unsigned int);
#endif

using std::string;
using std::vector;
typedef std::pair<int, int> Casilla;

struct Resultado {
  string nombre;
  string tiempo;
  int puntos;
  int maximo;
  string nota;
};
static vector<Resultado> resultados;

// Rellena con espacios contando letras, no bytes (las acentuadas ocupan 2 bytes)
static string rellenar(const string& s, int ancho) {
  int letras = 0;
  for (unsigned char c : s)
    if ((c & 0xC0) != 0x80) letras++;
  return letras >= ancho ? s : s + string(ancho - letras, ' ');
}

static void titulo(const string& t) {
  printf("\n===============================================================================\n");
  printf(" %s\n", t.c_str());
  printf("===============================================================================\n");
}

// Los 2 minutos de calibración: una referencia por cada tono que hay en la pista
static void calibrar(const vector<std::pair<Color, Rgb>>& tonos) {
  calibracion.cuantas = 0;
  for (const auto& t : tonos) {
    Referencia& r = calibracion.ref[calibracion.cuantas++];
    r.color = t.first;
    r.rgb = normalizar(t.second);
  }
}

// Corre una pista y atrapa lo que la detenga: un choque, una línea o el tiempo
template <typename Funcion>
static string correr(Funcion pista) {
  try {
    pista();
  } catch (const FinDeSimulacion& f) {
    hw::mensaje(TXT("ALTO: %s"), f.motivo.c_str());
    return f.motivo;
  }
  return "";
}

// ================================================================== Pista A
// La imagen 1.1 del reglamento, dibujada con texto. Cada casilla son 4 caracteres.
//   paredes: "---" y "|"          topes (speedbumps): " = " y ":"
//   V inicio   R final   C cian   A amarillo   N naranja   P rosa   E escalera   M rampa
// Para probar otro laberinto basta con dibujarlo aquí.
static const vector<string> LABERINTO_1_1 = {
  "+---+---+---+---+---+",
  "| N           E     |",
  "+---+   +   +   +   +",
  "|   : A   E     |   |",
  "+   +---+   +   +   +",
  "| C |       |   | M |",
  "+ = +   + = +   +   +",
  "|   |       |   | M |",
  "+   +   +   +---+   +",
  "| V   E   P :   | R |",
  "+---+---+---+---+---+",
};

// Uno inventado por nosotros, con la salida a media orilla y el final arriba:
// para comprobar que nada del código está amarrado al ejemplo del reglamento.
// Respeta los máximos: 1 rampa, 3 escaleras y 4 topes.
static const vector<string> LABERINTO_INVENTADO = {
  "+---+---+---+---+---+",
  "| R       A     |   |",
  "+   +---+   +   +   +",
  "|       | E     : C |",
  "+ = +   +---+   +   +",
  "| V         | M |   |",
  "+   +   +   +   + = +",
  "|   | N     | M |   |",
  "+   +---+   +   +   +",
  "|         P       E |",
  "+---+---+---+---+---+",
};

static void imprimirMapas(const vector<string>& dibujo) {
  std::map<Casilla, int> orden;               // en qué orden pisó cada casilla por primera vez
  int n = 0;
  for (const Casilla& c : marcador.casillas)
    if (!orden.count(c)) orden[c] = ++n;
  int filas = ((int)dibujo.size() - 1) / 2;
  int cols = ((int)dibujo[0].size() - 1) / 4;
  printf("\n  La pista (el robot no la conoce)       En qué orden pisó cada casilla\n");
  for (int i = 0; i < (int)dibujo.size(); i++) {
    string der = dibujo[i];
    if (i % 2 == 1) {
      int cy = filas - 1 - i / 2;
      for (int c = 0; c < cols; c++) {
        auto it = orden.find(Casilla(c, cy));
        char num[8];
        if (it == orden.end()) snprintf(num, sizeof(num), "   ");
        else snprintf(num, sizeof(num), "%3d", it->second);
        der.replace(4 * c + 1, 3, num);
      }
    }
    printf("  %s                  %s\n", dibujo[i].c_str(), der.c_str());
  }
}

// Compara lo que el robot anotó con la pista de verdad
static void revisarMapaAprendido(Casilla inicio) {
  const Explorador& e = explorador;
  int bien = 0, total = 0;
  for (int rx = 0; rx < TAM_MAPA; rx++) {
    for (int ry = 0; ry < TAM_MAPA; ry++) {
      if (!e.visitada[rx][ry]) continue;
      // El robot arranca en (4, 4) viendo "arriba", igual que la pista: basta con restar
      int cx = inicio.first + rx - TAM_MAPA / 2;
      int cy = inicio.second + ry - TAM_MAPA / 2;
      for (int d = 0; d < 4; d++) {
        if (e.lado[rx][ry][d] == NO_SE) continue;
        total++;
        if ((e.lado[rx][ry][d] == PARED) == hayPared(cx, cy, d)) bien++;
      }
    }
  }
  printf("\n  Mapa que aprendió solo: %d de %d lados bien (paredes y huecos de las casillas que pisó)\n",
         bien, total);
}

static void pruebaPistaA(const string& nombre, const vector<string>& dibujo) {
  titulo("PISTA A: " + nombre);
  printf("  Velocidad supuesta %.0f cm/s y 0.9 s por giro. Arranca en el verde viendo\n", velocidadCmS);
  printf("  hacia arriba, con la memoria vacía.\n\n");
  limpiar();
  laberinto(dibujo);
  Casilla inicio = marcador.casillas.front();
  calibrar({ { BLANCO, TONO_BLANCO }, { VERDE, TONO_VERDE }, { ROJO, TONO_ROJO }, { CIAN, TONO_CIAN },
             { AMARILLO, TONO_AMARILLO }, { NARANJA, TONO_NARANJA }, { ROSA, TONO_ROSA } });

  string alto = correr([] { correrPistaA(); });
  double tiempo = R.reloj;

  // Puntos, sección 4.1 del reglamento
  int colores = 0;
  for (Color c : marcador.coloresBien)
    if (esColorDePunto(c)) colores++;
  int topes = (int)marcador.topesCruzados.size();
  int escaleras = (int)marcador.escalerasCruzadas.size();
  bool rampa = marcador.rampaCruzada;
  bool llego = marcador.indiceRojo >= 0;
  // Bonus 1: después del rojo, las mismas casillas en orden inverso hasta la salida
  bool bonus1 = false;
  if (llego) {
    vector<Casilla> ida(marcador.casillas.begin(), marcador.casillas.begin() + marcador.indiceRojo + 1);
    vector<Casilla> vuelta(marcador.casillas.begin() + marcador.indiceRojo, marcador.casillas.end());
    std::reverse(vuelta.begin(), vuelta.end());
    bonus1 = vuelta.size() > 1 && vuelta == ida;
    if (!bonus1 && vuelta.size() > 1) {
      size_t k = 0;
      while (k < ida.size() && k < vuelta.size() && ida[k] == vuelta[k]) k++;
      printf("\n  El regreso no coincide con la ida: ida de %d casillas, regreso de %d; se separan en la %d\n",
             (int)ida.size(), (int)vuelta.size(), (int)k + 1);
      for (size_t j = (k > 3 ? k - 3 : 0); j < k + 3; j++) {
        printf("    %2d  ida (%d, %d)   regreso (%d, %d)\n", (int)j + 1,
               j < ida.size() ? ida[j].first : -9, j < ida.size() ? ida[j].second : -9,
               j < vuelta.size() ? vuelta[j].first : -9, j < vuelta.size() ? vuelta[j].second : -9);
      }
    }
  }
  int puntos = colores * 5 + topes * 5 + escaleras * 15 + (rampa ? 20 : 0) + (llego ? 25 : 0) + (bonus1 ? 35 : 0);

  printf("\n  Puntos (reglamento 4.1)\n");
  printf("    %3d  colores mostrados con el LED: %d de 4\n", colores * 5, colores);
  printf("    %3d  topes: %d de 4\n", topes * 5, topes);
  printf("    %3d  escaleras: %d de 3\n", escaleras * 15, escaleras);
  printf("    %3d  rampa: %s\n", rampa ? 20 : 0, rampa ? "la cruzó" : "no la cruzó");
  printf("    %3d  checkpoint final: %s\n", llego ? 25 : 0,
         llego ? ("llegó a los " + mmss(marcador.msRojo)).c_str() : "no llegó");
  printf("    %3d  bonus 1, regresar por el mismo camino: %s\n", bonus1 ? 35 : 0, bonus1 ? "sí" : "no");
  printf("    %3d  bonus 2, ArUco: pendiente\n", 0);
  printf("    %3d  de 195 posibles. Tiempo usado: %s\n", puntos, mmss(tiempo).c_str());
  if (marcador.coloresMal) printf("    Ojo: el LED mostró %d colores equivocados\n", marcador.coloresMal);

  imprimirMapas(dibujo);
  revisarMapaAprendido(inicio);
  resultados.push_back({ "Pista A: " + nombre, mmss(tiempo), puntos, 195,
                         !alto.empty() ? alto : (bonus1 ? "con bonus 1" : "sin bonus 1") });
}

// ================================================================== Pista B
// Tonos que se calibran en la B: el verde sale tres veces con tono distinto
static const vector<std::pair<Color, Rgb>> TONOS_B = {
  { BLANCO, TONO_BLANCO }, { VERDE, TONO_VERDE }, { ROJO, TONO_ROJO },
  { VERDE, TONO_VERDE_SECCION2 }, { CIAN, TONO_CIAN }, { AMARILLO, TONO_AMARILLO },
  { NARANJA, TONO_NARANJA }, { ROSA, TONO_ROSA }, { VERDE, TONO_VERDE_FIN },
};

enum Lineas { LINEAS_2_1, LINEAS_2_4 };

// La pista B de la imagen 2.1, en cm (x a la derecha, y hacia arriba):
//   inicio x 0-30, y -30-0      sección 1 (3 x 3) x 0-90, y 0-90     checkpoint 1 x 90-120, y 30-60
//   sección 2 (2 x 4) x 120-240, y 15-75                              checkpoint 2 x 240-270, y 30-60
//   sección 3 (3 x 3) x 270-360, y 0-90                               fin x 330-360, y 90-120
static void armarPistaB(int ladoAbierto, Lineas lineas) {
  limpiar();
  // Inicio
  baldosa(0, -30, 30, 30, VERDE, TONO_VERDE, "inicio");
  pared(0, -30, 0, 0);
  pared(30, -30, 30, 0);
  pared(0, -30, 30, -30);
  // Sección 1: paredes de afuera, con la entrada desde el inicio y la salida al checkpoint
  pared(0, 0, 0, 90);
  pared(0, 90, 90, 90);
  pared(30, 0, 90, 0);
  pared(90, 0, 90, 30);
  pared(90, 60, 90, 90);
  // La casilla de la pelota, al centro, con tres paredes (imagen 2.2)
  if (ladoAbierto != ARRIBA) pared(30, 60, 60, 60);
  if (ladoAbierto != ABAJO) pared(30, 30, 60, 30);
  if (ladoAbierto != IZQUIERDA) pared(30, 30, 30, 60);
  if (ladoAbierto != DERECHA) pared(60, 30, 60, 60);
  pelota(45, 45, Rect{ 30, 30, 60, 60 });
  // Checkpoint 1
  baldosa(90, 30, 30, 30, ROJO, TONO_ROJO, "checkpoint 1");
  pared(90, 30, 120, 30);
  pared(90, 60, 120, 60);
  // Sección 2: una sola baldosa verde (entre casillas del mismo color no hay franja blanca)
  baldosa(120, 15, 120, 60, VERDE, TONO_VERDE_SECCION2);
  pared(120, 15, 240, 15);
  pared(120, 75, 240, 75);
  pared(120, 15, 120, 30);
  pared(120, 60, 120, 75);
  pared(240, 15, 240, 30);
  pared(240, 60, 240, 75);
  if (lineas == LINEAS_2_1) {
    linea(150, 15, 45);                  // tapa la fila de abajo
    linea(180, 45, 75);                  // tapa la de arriba
    linea(210, 15, 29);                  // hueco a media altura
    linea(210, 59, 75);
  } else {                               // imagen 2.4
    linea(150, 15, 37.5);                // hueco a media altura
    linea(150, 67.5, 75);
    linea(180, 45, 75);                  // tapa la de arriba
    linea(210, 15, 45);                  // tapa la de abajo
  }
  // Checkpoint 2
  baldosa(240, 30, 30, 30, ROJO, TONO_ROJO, "checkpoint 2");
  pared(240, 30, 270, 30);
  pared(240, 60, 270, 60);
  // Sección 3: naranja, amarillo, naranja, amarillo y el fin
  baldosa(270, 30, 30, 30, NARANJA, TONO_NARANJA);
  baldosa(300, 30, 30, 30, AMARILLO, TONO_AMARILLO);
  baldosa(300, 60, 30, 30, NARANJA, TONO_NARANJA);
  baldosa(330, 60, 30, 30, AMARILLO, TONO_AMARILLO);
  baldosa(330, 90, 30, 30, VERDE, TONO_VERDE, "fin");
  pared(270, 0, 270, 30);
  pared(270, 60, 270, 90);
  pared(270, 0, 360, 0);
  pared(360, 0, 360, 120);
  pared(270, 90, 330, 90);
  pared(330, 90, 330, 120);
  pared(330, 120, 360, 120);
}

// Puntos de la B (reglamento 4.2). Lo de donde arrancó no cuenta.
static int puntosB(bool imprimir, bool contarSacarPelota) {
  int pts = 0;
  if (contarSacarPelota && marcador.sacoPelota) {
    pts += 15;
    if (imprimir) printf("     15  sacó la pelota de su casilla (a los %s)\n", mmss(marcador.msPelota).c_str());
  }
  bool visto[3] = { false, false, false };
  const char* nombres[3] = { "checkpoint 1", "checkpoint 2", "fin" };
  const int con[3] = { 70, 70, 85 }, sin[3] = { 40, 40, 50 };
  for (const Llegada& l : marcador.llegadas) {
    if (l.ms == 0) continue;
    for (int k = 0; k < 3; k++) {
      if (l.etiqueta == nombres[k] && !visto[k]) {
        visto[k] = true;
        int p = l.conPelota ? con[k] : sin[k];
        pts += p;
        if (imprimir)
          printf("    %3d  %s %s pelota (a los %s)\n", p, nombres[k], l.conPelota ? "con" : "sin",
                 mmss(l.ms).c_str());
      }
    }
  }
  return pts;
}

static string momento(const string& etiqueta) {
  for (const Llegada& l : marcador.llegadas)
    if (l.etiqueta == etiqueta && l.ms > 0) return mmss(l.ms);
  return "--";
}

static void pruebaPistaBCompleta() {
  titulo("PISTA B: la pista de la imagen 2.1, de principio a fin");
  printf("  Pelota con el lado abierto hacia abajo, como en la imagen 2.1. Arranca en\n");
  printf("  el inicio viendo hacia arriba, con la memoria vacía.\n\n");
  armarPistaB(ABAJO, LINEAS_2_1);
  calibrar(TONOS_B);
  robotEn(15, -15, ARRIBA);
  string alto = correr([] { correrPistaB(); });
  double tiempo = R.reloj;
  printf("\n  Puntos (reglamento 4.2)\n");
  int pts = puntosB(true, true);
  printf("    %3d  de 240 posibles. Tiempo usado: %s\n", pts, mmss(tiempo).c_str());
  resultados.push_back({ "Pista B: imagen 2.1 completa", mmss(tiempo), pts, 240, alto });

  titulo("PISTA B: sección 1 con las cuatro posiciones de las paredes (imagen 2.2)");
  printf("  Misma pista; solo cambia hacia dónde queda abierta la casilla de la pelota.\n\n");
  const int lados[4] = { ARRIBA, DERECHA, ABAJO, IZQUIERDA };
  const char* nombres[4] = { "arriba", "la derecha", "abajo", "la izquierda" };
  for (int i = 0; i < 4; i++) {
    armarPistaB(lados[i], LINEAS_2_1);
    calibrar(TONOS_B);
    robotEn(15, -15, ARRIBA);
    silencio = true;
    string motivo = correr([] { correrPistaB(); });
    silencio = false;
    int p = puntosB(false, true);
    printf("  Abierta hacia %s pelota a los %s, checkpoint 1 a los %s, fin a los %s: %d puntos%s%s\n",
           rellenar(string(nombres[i]) + ":", 13).c_str(), mmss(marcador.msPelota).c_str(),
           momento("checkpoint 1").c_str(), momento("fin").c_str(), p, motivo.empty() ? "" : ". ALTO: ",
           motivo.c_str());
    resultados.push_back({ string("Pista B: pelota abierta hacia ") + nombres[i], mmss(R.reloj), p, 240, motivo });
  }
}

static void pruebaReinicioCheckpoint1() {
  titulo("PISTA B: reinicio en el checkpoint 1, con las líneas de la imagen 2.4");
  printf("  Lack of progress en la sección 2: el capitán reinicia el robot en el checkpoint\n");
  printf("  1 con la pelota enfrente (imagen 3.1). El robot tiene que darse cuenta solo de\n");
  printf("  que va en la sección 2.\n\n");
  armarPistaB(ABAJO, LINEAS_2_4);
  calibrar(TONOS_B);
  robotEn(105, 45, DERECHA);
  M.pelotaX = 105 + 10 + 2.1;          // la pelota tocando el frente del robot
  M.pelotaY = 45;
  string alto = correr([] { correrPistaB(); });
  printf("\n  Puntos desde el reinicio\n");
  int pts = puntosB(true, false);
  printf("    %3d  Tiempo usado: %s\n", pts, mmss(R.reloj).c_str());
  resultados.push_back({ "Pista B: reinicio en checkpoint 1, líneas de la 2.4", mmss(R.reloj), pts, 155, alto });
}

static void pruebaReinicioCheckpoint2() {
  titulo("PISTA B: reinicio en el checkpoint 2, con la sección 3 de la imagen 2.7");
  printf("  El ejemplo del reglamento: la ruta es arriba, arriba, derecha, abajo, derecha.\n");
  printf("  Es la misma prueba de escritorio de la propuesta del 19 sep.\n\n");
  limpiar();
  baldosa(30, -30, 30, 30, ROJO, TONO_ROJO, "checkpoint 2");
  baldosa(30, 0, 30, 30, NARANJA, TONO_NARANJA);
  baldosa(30, 30, 30, 30, NARANJA, TONO_NARANJA);
  baldosa(30, 60, 30, 30, CIAN, TONO_CIAN);
  baldosa(60, 60, 30, 30, ROSA, TONO_ROSA);
  baldosa(60, 30, 30, 30, CIAN, TONO_CIAN);
  baldosa(90, 30, 30, 30, VERDE, TONO_VERDE_FIN, "fin");
  calibrar(TONOS_B);
  robotEn(45, -15, ARRIBA);
  pelota(45, -15 + 10 + 2.1, Rect{ 0, 0, 0, 0 });
  string alto = correr([] { correrPistaB(); });
  printf("\n  Puntos desde el reinicio\n");
  int pts = puntosB(true, false);
  printf("    %3d  Tiempo usado: %s\n", pts, mmss(R.reloj).c_str());
  resultados.push_back({ "Pista B: reinicio en checkpoint 2, sección 3 de la 2.7", mmss(R.reloj), pts, 85, alto });
}

// ================================================================== main
int main(int argc, char** argv) {
#ifdef _WIN32
  SetConsoleOutputCP(65001);           // para que se vean los acentos
#endif
  bool soloA = false, soloB = false;
  for (int i = 1; i < argc; i++) {
    string a = argv[i];
    if (a == "A" || a == "a") soloA = true;
    else if (a == "B" || a == "b") soloB = true;
    else if (a == "-v") detalle = true;
    else if (a == "--vel" && i + 1 < argc) velocidadCmS = atof(argv[++i]);
  }
  printf("Simulador de Candidates 2026 (borrador). Velocidad supuesta: %.0f cm/s\n", velocidadCmS);

  if (soloA || !soloB) {
    pruebaPistaA("laberinto de la imagen 1.1", LABERINTO_1_1);
    pruebaPistaA("laberinto inventado (otra salida, otro final)", LABERINTO_INVENTADO);
  }
  if (soloB || !soloA) {
    pruebaPistaBCompleta();
    pruebaReinicioCheckpoint1();
    pruebaReinicioCheckpoint2();
  }

  titulo("RESUMEN");
  for (const Resultado& r : resultados) {
    printf("  %s %5s  %3d de %3d  %s\n", rellenar(r.nombre, 56).c_str(), r.tiempo.c_str(), r.puntos,
           r.maximo, r.nota.c_str());
  }
  printf("\n");
  return 0;
}
