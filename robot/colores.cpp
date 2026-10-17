// colores.cpp
#include "colores.h"
#include "config.h"
#include "hardware.h"

Calibracion calibracion;

// Qué tan distintos son dos colores: distancia al cuadrado entre sus RGB
static long distancia2(Rgb a, Rgb b) {
  long dr = a.r - b.r;
  long dg = a.g - b.g;
  long db = a.b - b.b;
  return dr * dr + dg * dg + db * db;
}

// Cada lectura se asigna a la referencia más cercana
Color clasificar(Rgb lectura) {
  Color mejor = DESCONOCIDO;
  long menor = 0;
  for (int i = 0; i < calibracion.cuantas; i++) {
    long d = distancia2(lectura, calibracion.ref[i].rgb);
    if (mejor == DESCONOCIDO || d < menor) {
      menor = d;
      mejor = calibracion.ref[i].color;
    }
  }
  return mejor;
}

Color leerColorUnaVez() { return clasificar(hw::leerRgb()); }

// 5 lecturas y gana el color que más se repite. Aguanta una lectura mala.
Color leerColor() {
  int votos[NUM_COLORES];
  for (int c = 0; c < NUM_COLORES; c++) votos[c] = 0;
  for (int i = 0; i < LECTURAS_COLOR; i++) {
    Color c = leerColorUnaVez();
    if (c < NUM_COLORES) votos[c]++;
  }
  Color ganador = DESCONOCIDO;
  int maximo = 0;
  for (int c = 0; c < NUM_COLORES; c++) {
    if (votos[c] > maximo) {
      maximo = votos[c];
      ganador = (Color)c;
    }
  }
  return ganador;
}

// ------------------------------------------------------------------ Calibración
// En los 2 minutos de calibración se pone el sensor sobre cada color de la
// pista y se presiona el botón. Se guarda en la EEPROM para que sobreviva al
// reinicio de un lack of progress (durante la ronda ya no se puede calibrar).
// Lo que se guarda son tonos, no posiciones: eso no es pre-mapeo.

// Cada paso dice qué color es y qué decirle al capitán. Es un switch y no un
// arreglo de textos porque TXT (dejar el texto en la memoria de programa) solo
// funciona dentro de una función. Cuando ya no hay pasos regresa DESCONOCIDO.
static Color pasoA(int i, const char*& aviso) {
  switch (i) {
    case 0: aviso = TXT("Calibrar: pon el sensor sobre el piso blanco y presiona"); return BLANCO;
    case 1: aviso = TXT("Calibrar: pon el sensor sobre la casilla de inicio y presiona"); return VERDE;
    case 2: aviso = TXT("Calibrar: pon el sensor sobre la casilla final y presiona"); return ROJO;
    case 3: aviso = TXT("Calibrar: pon el sensor sobre la casilla cian y presiona"); return CIAN;
    case 4: aviso = TXT("Calibrar: pon el sensor sobre la casilla amarilla y presiona"); return AMARILLO;
    case 5: aviso = TXT("Calibrar: pon el sensor sobre la casilla naranja y presiona"); return NARANJA;
    case 6: aviso = TXT("Calibrar: pon el sensor sobre la casilla rosa y presiona"); return ROSA;
    default: return DESCONOCIDO;
  }
}

// En la B el verde sale en tres tonos: el inicio, el piso de la sección 2 y el fin
static Color pasoB(int i, const char*& aviso) {
  switch (i) {
    case 0: aviso = TXT("Calibrar: pon el sensor sobre el piso blanco de la sección 1 y presiona"); return BLANCO;
    case 1: aviso = TXT("Calibrar: pon el sensor sobre la casilla de inicio y presiona"); return VERDE;
    case 2: aviso = TXT("Calibrar: pon el sensor sobre el checkpoint y presiona"); return ROJO;
    case 3: aviso = TXT("Calibrar: pon el sensor sobre el piso verde de la sección 2 y presiona"); return VERDE;
    case 4: aviso = TXT("Calibrar: pon el sensor sobre la casilla cian y presiona"); return CIAN;
    case 5: aviso = TXT("Calibrar: pon el sensor sobre la casilla amarilla y presiona"); return AMARILLO;
    case 6: aviso = TXT("Calibrar: pon el sensor sobre la casilla naranja y presiona"); return NARANJA;
    case 7: aviso = TXT("Calibrar: pon el sensor sobre la casilla rosa y presiona"); return ROSA;
    case 8: aviso = TXT("Calibrar: pon el sensor sobre la casilla FIN y presiona"); return VERDE;
    default: return DESCONOCIDO;
  }
}

void calibrarColores(bool pistaB) {
  calibracion.cuantas = 0;
  for (int i = 0; i < MAX_REFERENCIAS; i++) {
    const char* aviso = TXT("");
    Color color = pistaB ? pasoB(i, aviso) : pasoA(i, aviso);
    if (color == DESCONOCIDO) break;
    hw::mostrarColor(color);            // el LED dice qué color toca
    hw::mensaje(aviso);
    hw::esperarBoton();
    long r = 0, g = 0, b = 0;
    for (int k = 0; k < 10; k++) {
      Rgb m = hw::leerRgb();
      r += m.r;
      g += m.g;
      b += m.b;
    }
    Referencia& ref = calibracion.ref[calibracion.cuantas++];
    ref.color = color;
    ref.rgb.r = (int)(r / 10);
    ref.rgb.g = (int)(g / 10);
    ref.rgb.b = (int)(b / 10);
  }
  hw::guardarCalibracion(calibracion);
  hw::mensaje(TXT("Calibración guardada"));
}
