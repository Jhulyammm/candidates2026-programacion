// mundo.h
// Una pista falsa para probar la lógica en la computadora, sin robot.
//
// La simulación SÍ conoce la pista completa, porque hace de "mundo real". El
// robot no: la lógica solo recibe lecturas de sensores por hardware.h, igual
// que en la pista. Así se comprueba que el robot aprende el mapa solo, sin
// pre-mapeo.
//
// Medidas en cm. x crece a la derecha y y hacia arriba.

#pragma once
#include <string>
#include <utility>
#include <vector>
#include "base.h"

struct Rect {
  double x1, y1, x2, y2;
  bool contiene(double x, double y) const { return x >= x1 && x <= x2 && y >= y1 && y <= y2; }
};

enum TipoPiso { PISO_NORMAL, PISO_RAMPA, PISO_ESCALERA };

struct Baldosa {
  Rect r;
  Color clase;             // qué color es de verdad (para revisar lo que dice el LED)
  Rgb tono;                // qué tono tiene (lo que ve el sensor)
  TipoPiso tipo;
  std::string etiqueta;    // "inicio", "checkpoint 1", "fin"... para contar puntos
};

struct Pared {
  double x1, y1, x2, y2;   // siempre horizontal o vertical
};

struct Tope {              // speedbump entre dos casillas vecinas
  int ax, ay, bx, by;
};

struct Mundo {
  std::vector<Pared> paredes;
  std::vector<Baldosa> baldosas;   // lo que no cubre ninguna baldosa es piso blanco
  std::vector<Rect> lineas;        // líneas blancas de la sección 2: no se pueden pisar
  std::vector<Tope> topes;
  bool hayPelota = false;
  double pelotaX = 0, pelotaY = 0;
  Rect casillaPelota{ 0, 0, 0, 0 };
};

// Dónde está el robot de verdad (la lógica no lo sabe: lo calcula por su cuenta)
struct RobotSim {
  double x = 0, y = 0;             // centro
  int rumbo = ARRIBA;
  bool garraAbierta = false;
  bool pelotaAgarrada = false;
  double reloj = 0;                // ms desde que empezó la ronda
};

// Lo que va pasando, para calcular los puntos al final
struct Llegada {
  std::string etiqueta;
  double ms;
  bool conPelota;
};

struct Marcador {
  std::vector<std::pair<int, int>> casillas;   // cada casilla de 30 cm que pisa el centro
  std::vector<Color> coloresBien;              // colores que el LED mostró bien
  int coloresMal = 0;
  std::vector<int> topesCruzados;
  std::vector<std::pair<int, int>> escalerasCruzadas;
  bool rampaCruzada = false;
  std::pair<int, int> entradaRampa{ -99, -99 };
  int indiceRojo = -1;                         // en qué momento de "casillas" llegó al rojo
  double msRojo = -1;
  bool sacoPelota = false;
  double msPelota = -1;                        // cuándo cerró la garra con la pelota
  std::vector<Llegada> llegadas;
  std::string ultimaEtiqueta;
};

struct FinDeSimulacion {           // choque, línea pisada o se acabó el tiempo
  std::string motivo;
};

extern Mundo M;
extern RobotSim R;
extern Marcador marcador;
extern double velocidadCmS;        // qué tan rápido suponemos que va el robot
extern bool detalle;               // imprimir cada movimiento
extern bool silencio;              // no imprimir los mensajes del robot

// Tonos de la pista (sacados de las imágenes del reglamento)
extern const Rgb TONO_BLANCO, TONO_GRIS, TONO_CIAN, TONO_AMARILLO, TONO_NARANJA, TONO_ROSA,
                 TONO_ROJO, TONO_VERDE, TONO_VERDE_SECCION2, TONO_VERDE_FIN;

// Armar una pista
void limpiar();
void pared(double x1, double y1, double x2, double y2);
void baldosa(double x, double y, double ancho, double alto, Color clase, Rgb tono,
             const std::string& etiqueta = "", TipoPiso tipo = PISO_NORMAL);
void linea(double x, double y1, double y2);
void pelota(double x, double y, Rect casilla);
void robotEn(double x, double y, int rumbo);
void laberinto(const std::vector<std::string>& dibujo);   // pista A dibujada con texto

// Consultas para el reporte
Rgb normalizar(Rgb c);             // lo mismo que hace el sensor real: dividir entre el brillo
bool hayPared(int cx, int cy, int d);
TipoPiso tipoDeCasilla(int cx, int cy);
std::string mmss(double ms);
