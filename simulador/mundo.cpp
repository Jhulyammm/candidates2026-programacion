// mundo.cpp
// La pista falsa y el robot falso. Aquí se implementa hardware.h para la
// computadora: en lugar de mover motores y leer sensores, se mueve un punto
// sobre la pista y se calcula qué leería cada sensor.

#include "mundo.h"
#include "config.h"
#include "hardware.h"
#include <algorithm>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <random>

Mundo M;
RobotSim R;
Marcador marcador;
double velocidadCmS = 15;
bool detalle = false;
bool silencio = false;

const Rgb TONO_BLANCO = { 235, 235, 235 };
const Rgb TONO_GRIS = { 205, 205, 205 };        // escaleras y rampa
const Rgb TONO_CIAN = { 92, 225, 230 };
const Rgb TONO_AMARILLO = { 255, 217, 90 };
const Rgb TONO_NARANJA = { 255, 145, 77 };
const Rgb TONO_ROSA = { 255, 102, 196 };
const Rgb TONO_ROJO = { 255, 87, 87 };
const Rgb TONO_VERDE = { 126, 217, 87 };
const Rgb TONO_VERDE_SECCION2 = { 0, 191, 99 };
const Rgb TONO_VERDE_FIN = { 193, 255, 114 };

// ------------------------------------------------------------------ Medidas del robot simulado
// En cm, desde el centro del robot. Mide 20 x 20.
static const double MEDIO = LARGO_ROBOT / 2.0;
static const double LLANTA_ADELANTE = 5, LLANTA_LADO = 8.5, RADIO_LLANTA = 3;
static const double SENSOR_LINEA_ADELANTE = 10, SENSOR_LINEA_LADO = 8.5;
static const double RADIO_PELOTA = 2.1;
static const double GARRA_FONDO = 4, GARRA_ANCHO = 4;   // la pelota agarrada queda entre 4 y 10 cm
                                                        // delante del centro
static const double FRANJA = 1;   // franja blanca en la orilla de cada baldosa de color

// Ruido de los sensores. La semilla es fija para que cada corrida salga igual.
static std::mt19937 azar(2026);
static double ruido(double amplitud) {
  std::uniform_real_distribution<double> u(-amplitud, amplitud);
  return u(azar);
}

static void registrar();

// De "tantos cm adelante y tantos a la izquierda del robot" a coordenadas de la pista
static void aPista(double adelante, double izquierda, double& px, double& py) {
  int d = R.rumbo, iz = aLaIzquierda(R.rumbo);
  px = R.x + adelante * DX[d] + izquierda * DX[iz];
  py = R.y + adelante * DY[d] + izquierda * DY[iz];
}

// Y al revés: dónde queda un punto de la pista visto desde el robot
static void aRobot(double px, double py, double& adelante, double& izquierda) {
  int d = R.rumbo, iz = aLaIzquierda(R.rumbo);
  double rx = px - R.x, ry = py - R.y;
  adelante = rx * DX[d] + ry * DY[d];
  izquierda = rx * DX[iz] + ry * DY[iz];
}

// ------------------------------------------------------------------ Armar la pista
void limpiar() {
  M = Mundo();
  R = RobotSim();
  marcador = Marcador();
  azar.seed(2026);
}

void pared(double x1, double y1, double x2, double y2) { M.paredes.push_back({ x1, y1, x2, y2 }); }

void baldosa(double x, double y, double ancho, double alto, Color clase, Rgb tono,
             const std::string& etiqueta, TipoPiso tipo) {
  Baldosa b;
  b.r = { x, y, x + ancho, y + alto };
  b.clase = clase;
  b.tono = tono;
  b.tipo = tipo;
  b.etiqueta = etiqueta;
  M.baldosas.push_back(b);
}

void linea(double x, double y1, double y2) { M.lineas.push_back({ x - 1, y1, x + 1, y2 }); }  // 2 cm

void pelota(double x, double y, Rect casilla) {
  M.hayPelota = true;
  M.pelotaX = x;
  M.pelotaY = y;
  M.casillaPelota = casilla;
}

void robotEn(double x, double y, int rumbo) {
  R.x = x;
  R.y = y;
  R.rumbo = rumbo;
  registrar();
}

// La pista A se dibuja con texto; ver LABERINTO_1_1 en simulador.cpp
void laberinto(const std::vector<std::string>& d) {
  int filas = ((int)d.size() - 1) / 2;
  int cols = ((int)d[0].size() - 1) / 4;
  double inicioX = 15, inicioY = 15;
  for (int r = 0; r < filas; r++) {
    for (int c = 0; c < cols; c++) {
      int cy = filas - 1 - r;                  // en el dibujo la fila 0 es la de arriba
      double x0 = c * 30.0, y0 = cy * 30.0;
      std::string arriba = d[2 * r].substr(4 * c + 1, 3);
      std::string abajo = d[2 * r + 2].substr(4 * c + 1, 3);
      char izq = d[2 * r + 1][4 * c];
      char der = d[2 * r + 1][4 * c + 4];
      char k = d[2 * r + 1][4 * c + 2];
      if (arriba == "---") pared(x0, y0 + 30, x0 + 30, y0 + 30);
      if (r == filas - 1 && abajo == "---") pared(x0, y0, x0 + 30, y0);
      if (izq == '|') pared(x0, y0, x0, y0 + 30);
      if (c == cols - 1 && der == '|') pared(x0 + 30, y0, x0 + 30, y0 + 30);
      if (arriba == " = ") M.topes.push_back({ c, cy, c, cy + 1 });
      if (izq == ':') M.topes.push_back({ c - 1, cy, c, cy });
      switch (k) {
        case 'C': baldosa(x0, y0, 30, 30, CIAN, TONO_CIAN); break;
        case 'A': baldosa(x0, y0, 30, 30, AMARILLO, TONO_AMARILLO); break;
        case 'N': baldosa(x0, y0, 30, 30, NARANJA, TONO_NARANJA); break;
        case 'P': baldosa(x0, y0, 30, 30, ROSA, TONO_ROSA); break;
        case 'R': baldosa(x0, y0, 30, 30, ROJO, TONO_ROJO, "rojo"); break;
        case 'V':
          baldosa(x0, y0, 30, 30, VERDE, TONO_VERDE, "inicio");
          inicioX = x0 + 15;
          inicioY = y0 + 15;
          break;
        case 'E': baldosa(x0, y0, 30, 30, BLANCO, TONO_GRIS, "", PISO_ESCALERA); break;
        case 'M': baldosa(x0, y0, 30, 30, BLANCO, TONO_GRIS, "", PISO_RAMPA); break;
        default: break;
      }
    }
  }
  robotEn(inicioX, inicioY, ARRIBA);
}

// ------------------------------------------------------------------ Consultas
static const Baldosa* baldosaEn(double x, double y) {
  for (int i = (int)M.baldosas.size() - 1; i >= 0; i--)
    if (M.baldosas[i].r.contiene(x, y)) return &M.baldosas[i];
  return nullptr;
}

// Qué hay en el piso en un punto: el tono y qué color es de verdad
static void pisoEn(double x, double y, Rgb& tono, Color& clase) {
  for (const Rect& l : M.lineas) {
    if (l.contiene(x, y)) {
      tono = TONO_BLANCO;
      clase = BLANCO;
      return;
    }
  }
  const Baldosa* b = baldosaEn(x, y);
  if (b == nullptr) {
    tono = TONO_BLANCO;
    clase = BLANCO;
    return;
  }
  bool orilla = x < b->r.x1 + FRANJA || x > b->r.x2 - FRANJA || y < b->r.y1 + FRANJA || y > b->r.y2 - FRANJA;
  if (orilla && b->tipo == PISO_NORMAL && b->clase != BLANCO) {
    tono = TONO_BLANCO;               // la franja blanca entre casillas de distinto color
    clase = BLANCO;
    return;
  }
  tono = b->tono;
  clase = b->clase;
}

Rgb normalizar(Rgb c) {
  long s = (long)c.r + c.g + c.b;
  if (s == 0) s = 1;
  Rgb n;
  n.r = (int)(c.r * 255L / s);
  n.g = (int)(c.g * 255L / s);
  n.b = (int)(c.b * 255L / s);
  return n;
}

bool hayPared(int cx, int cy, int d) {
  double mx = cx * 30.0 + 15 + DX[d] * 15;   // punto medio de ese lado de la casilla
  double my = cy * 30.0 + 15 + DY[d] * 15;
  for (const Pared& p : M.paredes) {
    if (p.x1 == p.x2 && DX[d] != 0 && std::fabs(p.x1 - mx) < 0.01 &&
        my >= std::min(p.y1, p.y2) && my <= std::max(p.y1, p.y2)) return true;
    if (p.y1 == p.y2 && DY[d] != 0 && std::fabs(p.y1 - my) < 0.01 &&
        mx >= std::min(p.x1, p.x2) && mx <= std::max(p.x1, p.x2)) return true;
  }
  return false;
}

TipoPiso tipoDeCasilla(int cx, int cy) {
  const Baldosa* b = baldosaEn(cx * 30.0 + 15, cy * 30.0 + 15);
  return b ? b->tipo : PISO_NORMAL;
}

std::string mmss(double ms) {
  int s = (int)(ms / 1000);
  char t[16];
  snprintf(t, sizeof(t), "%d:%02d", s / 60, s % 60);
  return t;
}

// Distancia desde un sensor hasta la primera pared (o la pelota) en un rumbo
static double rayo(double ox, double oy, int d, bool veLaPelota) {
  double mejor = 200;
  double ux = DX[d], uy = DY[d];
  for (const Pared& p : M.paredes) {
    if (p.x1 == p.x2 && ux != 0) {            // pared vertical, rayo horizontal
      double t = (p.x1 - ox) / ux;
      if (t >= -0.01 && oy >= std::min(p.y1, p.y2) - 0.01 && oy <= std::max(p.y1, p.y2) + 0.01)
        mejor = std::min(mejor, t);
    } else if (p.y1 == p.y2 && uy != 0) {     // pared horizontal, rayo vertical
      double t = (p.y1 - oy) / uy;
      if (t >= -0.01 && ox >= std::min(p.x1, p.x2) - 0.01 && ox <= std::max(p.x1, p.x2) + 0.01)
        mejor = std::min(mejor, t);
    }
  }
  if (veLaPelota && M.hayPelota && !R.pelotaAgarrada) {
    double bx = M.pelotaX - ox, by = M.pelotaY - oy;
    double enfrente = bx * ux + by * uy;
    double deLado = std::fabs(bx * uy - by * ux);
    if (enfrente > 0 && deLado < RADIO_PELOTA)
      mejor = std::min(mejor, enfrente - std::sqrt(RADIO_PELOTA * RADIO_PELOTA - deLado * deLado));
  }
  return std::max(0.0, mejor);
}

// ------------------------------------------------------------------ La pelota
static bool pelotaEnZonaDeGarra() {
  if (!M.hayPelota) return false;
  double ad, iz;
  aRobot(M.pelotaX, M.pelotaY, ad, iz);
  return ad >= GARRA_FONDO - 0.6 && ad <= MEDIO + 0.6 && std::fabs(iz) < GARRA_ANCHO;
}

static bool conPelota() { return M.hayPelota && (R.pelotaAgarrada || pelotaEnZonaDeGarra()); }

static void llevarPelota() { aPista((GARRA_FONDO + MEDIO) / 2, 0, M.pelotaX, M.pelotaY); }

// Si el robot avanza contra la pelota, la empuja (o entra a la garra si está abierta)
static void empujarPelota() {
  if (!M.hayPelota) return;
  if (R.pelotaAgarrada) {
    llevarPelota();
    return;
  }
  double ad, iz;
  aRobot(M.pelotaX, M.pelotaY, ad, iz);
  if (ad < 0 || std::fabs(iz) > MEDIO + RADIO_PELOTA) return;   // no está enfrente
  bool entraALaGarra = R.garraAbierta && std::fabs(iz) < GARRA_ANCHO;
  double limite = entraALaGarra ? GARRA_FONDO : MEDIO + RADIO_PELOTA;
  if (ad < limite) aPista(limite, iz, M.pelotaX, M.pelotaY);
}

// ------------------------------------------------------------------ Revisiones en cada paso
static void revisarChoques() {
  const double e = 0.2;
  double x1 = R.x - MEDIO, x2 = R.x + MEDIO, y1 = R.y - MEDIO, y2 = R.y + MEDIO;
  for (const Pared& p : M.paredes) {
    bool choca;
    if (p.x1 == p.x2) {
      choca = p.x1 > x1 + e && p.x1 < x2 - e &&
              std::max(p.y1, p.y2) > y1 + e && std::min(p.y1, p.y2) < y2 - e;
    } else {
      choca = p.y1 > y1 + e && p.y1 < y2 - e &&
              std::max(p.x1, p.x2) > x1 + e && std::min(p.x1, p.x2) < x2 - e;
    }
    if (choca) {
      char t[120];
      snprintf(t, sizeof(t), "CHOQUE con una pared, con el robot en (%.0f, %.0f)", R.x, R.y);
      throw FinDeSimulacion{ t };
    }
  }
}

// Ninguna llanta puede pasar por una línea blanca (reglamento, imagen 2.5)
static void revisarLineas() {
  if (M.lineas.empty()) return;
  for (int lado = -1; lado <= 1; lado += 2) {
    for (int eje = -1; eje <= 1; eje += 2) {          // llantas de adelante y de atrás
      for (double t = -RADIO_LLANTA; t <= RADIO_LLANTA + 0.01; t += 1.0) {
        double px, py;
        aPista(eje * LLANTA_ADELANTE + t, lado * LLANTA_LADO, px, py);
        for (const Rect& l : M.lineas) {
          if (l.contiene(px, py)) {
            char txt[120];
            snprintf(txt, sizeof(txt), "Una llanta pisó una línea blanca en (%.0f, %.0f): lack of progress", px, py);
            throw FinDeSimulacion{ txt };
          }
        }
      }
    }
  }
}

static void revisarTiempo() {
  if (R.reloj > MS_RONDA) throw FinDeSimulacion{ "Se acabó el tiempo de la ronda (6:00)" };
}

static bool contiene(const std::vector<std::pair<int, int>>& v, std::pair<int, int> c) {
  return std::find(v.begin(), v.end(), c) != v.end();
}

// Puntos por obstáculos: se cuentan al pasar el centro del robot de una casilla a otra
static void alCambiarDeCasilla(std::pair<int, int> a, std::pair<int, int> b) {
  for (int i = 0; i < (int)M.topes.size(); i++) {
    const Tope& t = M.topes[i];
    bool cruza = (t.ax == a.first && t.ay == a.second && t.bx == b.first && t.by == b.second) ||
                 (t.bx == a.first && t.by == a.second && t.ax == b.first && t.ay == b.second);
    if (cruza && std::find(marcador.topesCruzados.begin(), marcador.topesCruzados.end(), i) ==
                     marcador.topesCruzados.end())
      marcador.topesCruzados.push_back(i);
  }
  if (tipoDeCasilla(a.first, a.second) == PISO_ESCALERA && !contiene(marcador.escalerasCruzadas, a))
    marcador.escalerasCruzadas.push_back(a);        // subió y bajó
  bool rampaA = tipoDeCasilla(a.first, a.second) == PISO_RAMPA;
  bool rampaB = tipoDeCasilla(b.first, b.second) == PISO_RAMPA;
  if (!rampaA && rampaB) marcador.entradaRampa = a;
  if (rampaA && !rampaB && b != marcador.entradaRampa) marcador.rampaCruzada = true;
}

static void registrar() {
  std::pair<int, int> c((int)std::floor(R.x / CM_CASILLA), (int)std::floor(R.y / CM_CASILLA));
  if (marcador.casillas.empty() || marcador.casillas.back() != c) {
    if (!marcador.casillas.empty()) alCambiarDeCasilla(marcador.casillas.back(), c);
    marcador.casillas.push_back(c);
    // La llegada al rojo se cuenta con la misma regla de casillas (el centro
    // adentro), para poder comparar después la ida con el regreso
    const Baldosa* bc = baldosaEn(c.first * 30.0 + 15, c.second * 30.0 + 15);
    if (bc && bc->etiqueta == "rojo" && marcador.indiceRojo < 0) {
      marcador.indiceRojo = (int)marcador.casillas.size() - 1;
      marcador.msRojo = R.reloj;
    }
  }
  const Baldosa* b = baldosaEn(R.x, R.y);
  std::string et = b ? b->etiqueta : "";
  if (et != marcador.ultimaEtiqueta) {
    marcador.ultimaEtiqueta = et;
    if (!et.empty()) marcador.llegadas.push_back({ et, R.reloj, conPelota() });
  }
  if (R.pelotaAgarrada && !marcador.sacoPelota && !M.casillaPelota.contiene(M.pelotaX, M.pelotaY))
    marcador.sacoPelota = true;
}

// ================================================================== hardware.h en la simulación
namespace hw {

void iniciar() {}
void reiniciarReloj() { R.reloj = 0; }
unsigned long ms() { return (unsigned long)R.reloj; }

void avanzar(int cm) {
  if (cm == 0) return;
  R.reloj += (std::abs(cm) >= 5) ? 250 : 20;          // arrancar y frenar
  double paso = cm > 0 ? 0.5 : -0.5;
  int pasos = std::abs(cm) * 2;
  for (int i = 0; i < pasos; i++) {
    R.x += paso * DX[R.rumbo];
    R.y += paso * DY[R.rumbo];
    R.reloj += 0.5 / velocidadCmS * 1000.0;
    empujarPelota();
    revisarChoques();
    revisarLineas();
    registrar();
  }
  if (detalle) printf("                avanza %d cm -> (%.0f, %.0f)\n", cm, R.x, R.y);
  revisarTiempo();
}

void girar(int grados) {
  int cuartos = ((grados / 90) % 4 + 4) % 4;
  if (cuartos == 0) return;
  R.rumbo = (R.rumbo + cuartos) % 4;
  if (R.pelotaAgarrada) llevarPelota();
  R.reloj += (cuartos == 2) ? 1600 : 900;
  revisarChoques();
  revisarLineas();
  if (detalle) printf("                gira %d\n", grados);
  revisarTiempo();
}

static int medir(double adelante, double izquierda, int d, bool veLaPelota) {
  double px, py;
  aPista(adelante, izquierda, px, py);
  R.reloj += 20;
  double cm = rayo(px, py, d, veLaPelota) + ruido(1.0);
  return (int)std::lround(std::max(0.0, cm));
}

int distFrente() { return medir(MEDIO, 0, R.rumbo, false); }
int distIzquierda() { return medir(0, MEDIO, aLaIzquierda(R.rumbo), false); }
int distDerecha() { return medir(0, -MEDIO, aLaDerecha(R.rumbo), false); }
int distBaja() { return HAY_SENSOR_BAJO ? medir(MEDIO, 0, R.rumbo, true) : 200; }   // igual que el robot

static int canal(double v) { return (int)std::max(0.0, std::min(255.0, v)); }

Rgb leerRgb() {
  double px, py;
  aPista(SENSOR_COLOR_ADELANTE, 0, px, py);
  Rgb tono;
  Color clase;
  pisoEn(px, py, tono, clase);
  R.reloj += 30;
  Rgb m;
  m.r = canal(tono.r + ruido(12));
  m.g = canal(tono.g + ruido(12));
  m.b = canal(tono.b + ruido(12));
  return normalizar(m);   // igual que el sensor real
}

static bool blancoEn(double adelante, double izquierda) {
  double px, py;
  aPista(adelante, izquierda, px, py);
  Rgb tono;
  Color clase;
  pisoEn(px, py, tono, clase);
  return clase == BLANCO;
}

bool lineaIzquierda() { return blancoEn(SENSOR_LINEA_ADELANTE, SENSOR_LINEA_LADO); }
bool lineaDerecha() { return blancoEn(SENSOR_LINEA_ADELANTE, -SENSOR_LINEA_LADO); }

int inclinacion() {
  const Baldosa* b = baldosaEn(R.x, R.y);
  return (b && b->tipo == PISO_RAMPA) ? 7 : 0;
}

void garra(bool abierta) {
  if (!abierta && pelotaEnZonaDeGarra() && !R.pelotaAgarrada) {
    R.pelotaAgarrada = true;
    llevarPelota();
    if (marcador.msPelota < 0) marcador.msPelota = R.reloj;
  }
  if (abierta) R.pelotaAgarrada = false;
  R.garraAbierta = abierta;
  R.reloj += 400;
}

bool pelotaEnGarra() { return R.pelotaAgarrada || (R.garraAbierta && pelotaEnZonaDeGarra()); }

void mostrarColor(Color c) {
  double px, py;
  aPista(SENSOR_COLOR_ADELANTE, 0, px, py);
  Rgb tono;
  Color real;
  pisoEn(px, py, tono, real);
  if (real == c) {
    if (std::find(marcador.coloresBien.begin(), marcador.coloresBien.end(), c) == marcador.coloresBien.end())
      marcador.coloresBien.push_back(c);
  } else {
    marcador.coloresMal++;
    if (!silencio)
      printf("  %5s  ERROR: el LED dice %s pero abajo hay %s\n", mmss(R.reloj).c_str(), nombreColor(c),
             nombreColor(real));
  }
}

void mostrarNumero(int n) {
  if (!silencio) printf("  %5s  pantalla: %d\n", mmss(R.reloj).c_str(), n);
}

// Como printf. En el robot se imprime letra por letra y no se corta; aquí se
// arma en un arreglo de sobra para ponerle el tiempo adelante.
void mensaje(const char* formato, ...) {
  if (silencio) return;
  char texto[300];
  va_list datos;
  va_start(datos, formato);
  vsnprintf(texto, sizeof(texto), formato, datos);
  va_end(datos);
  printf("  %5s  %s\n", mmss(R.reloj).c_str(), texto);
}

bool botonPresionado() { return false; }
void esperarBoton() {}
bool selectorPistaB() { return false; }

static Calibracion guardada;
static bool hayGuardada = false;
void guardarCalibracion(const Calibracion& c) {
  guardada = c;
  hayGuardada = true;
}
bool cargarCalibracion(Calibracion& c) {
  if (!hayGuardada) return false;
  c = guardada;
  return true;
}

}  // namespace hw
