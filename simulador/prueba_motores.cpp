// prueba_motores.cpp
// Prueba el serial entre los dos Arduino sin robot. Compila el motores.ino de
// verdad con un Arduino falso y le mete letras como las que manda U2: mensajes
// de texto (con acentos) revueltos con órdenes ("~M7F7F"), órdenes cortadas y
// silencio.
//
//   .\probar_motores.bat
//
// Si se cambia el protocolo (motores.ino o hardware_robot.cpp), se corre esto.

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

const int HIGH = 1, LOW = 0, OUTPUT = 1, INPUT = 0;
unsigned long falsoMillis = 1000;
unsigned long millis() { return falsoMillis; }
void delay(unsigned long ms) { falsoMillis += ms; }
int pin[20], pwm[20];
void pinMode(int, int) {}
void digitalWrite(int p, int v) { pin[p] = v; }
void analogWrite(int p, int v) { pwm[p] = v; }
template <class T> T constrain(T x, T a, T b) { return x < a ? a : (x > b ? b : x); }
struct SerialFalso {
  std::vector<uint8_t> sale, entra;
  size_t pos = 0;
  void begin(long) {}
  int available() { return (int)(entra.size() - pos); }
  int read() { return entra[pos++]; }
  void write(uint8_t b) { sale.push_back(b); }
} Serial;

#include "motores.ino"

// Copias de velocidadEnByte() y enviarHex() de robot/hardware_robot.cpp (U2).
// Si cambian allá, cambian aquí.
static uint8_t velocidadEnByte(int v) {
  v = constrain(v, -254, 254);
  return (uint8_t)(v / 2 + 127);
}
static std::string hex(uint8_t b) {
  const char DIGITOS[] = "0123456789ABCDEF";
  return std::string(1, DIGITOS[b >> 4]) + DIGITOS[b & 15];
}

// Una orden como la arma U2: "~", la letra y cada dato en hexadecimal
static std::string armarOrden(char letra, std::initializer_list<int> datos = {}) {
  std::string s = std::string("~") + letra;
  for (int d : datos) s += hex((uint8_t)d);
  return s;
}
static std::string ordenDeVelocidad(int i, int d) { return armarOrden('M', { velocidadEnByte(i), velocidadEnByte(d) }); }

static int velMotorA() { return (pin[IN1] ? 1 : pin[IN2] ? -1 : 0) * pwm[ENA]; }
static int velMotorB() { return (pin[IN3] ? 1 : pin[IN4] ? -1 : 0) * pwm[ENB]; }

static void texto(const std::string& t) { for (char c : t) Serial.entra.push_back((uint8_t)c); }
static void ordenM(int i, int d) { texto(ordenDeVelocidad(i, d)); }
static void correr() { loop(); }

static int fallas = 0;
static void revisa(bool ok, const char* que) {
  printf("%s  %s\n", ok ? "OK   " : "FALLA", que);
  if (!ok) fallas++;
}

int main() {
  setup();

  // 1. Toda orden es texto que el monitor serial muestra tal cual (de ' ' a '~'),
  //    y la velocidad hace bien la ida y la vuelta en todo el rango
  bool imprimible = true, ida = true;
  for (int v = -300; v <= 300; v++) {
    std::string o = ordenDeVelocidad(v, -v) + armarOrden('L', { (v + 300) % 9 }) + armarOrden('F') + armarOrden('?');
    for (char c : o) if (c < ' ' || c > '~') imprimible = false;
    uint8_t b = velocidadEnByte(v);
    int vuelta = (b - 127) * 2;
    int esperado = constrain(v, -254, 254);
    if (abs(vuelta - esperado) > 1) ida = false;
  }
  revisa(imprimible, "toda orden es texto imprimible (el monitor serial no se come letras)");
  revisa(ida, "la velocidad que llega difiere a lo mucho en 1 de la que se mandó");

  // 2. Texto con acentos y letras de órdenes, luego una orden
  texto("Casilla (3, 4): más allá, niño. M L F ? 7F\r\n");
  ordenM(160, -160);
  correr();
  revisa(velMotorA() == 160 && velMotorB() == -160, "texto ignorado y 'M' 160 -160 llega igual");

  // 3. Texto justo después de una orden no mueve nada
  texto("MMMM LLL 7F7F\r\n");
  correr();
  revisa(velMotorA() == 160 && velMotorB() == -160, "texto después de la orden no cambia las velocidades");
  revisa(llevamos <= 2, "el texto no se escribe en datos[] (no se sale del arreglo)");

  // 4. Orden cortada a la mitad: el siguiente ~ vuelve a sincronizar
  texto("~MC8");
  ordenM(-90, 90);
  correr();
  revisa(velMotorA() == -90 && velMotorB() == 90, "orden cortada se descarta y la siguiente llega bien");

  // 4b. Algo que no es hexadecimal a media orden: se tira la orden entera
  texto("~M7Z7F7F hola\r\n");
  correr();
  revisa(velMotorA() == -90 && velMotorB() == 90, "una letra que no es hexadecimal tira la orden");

  // 5. Letra desconocida: se ignora hasta el siguiente ~
  texto("~X0A14hola\r\n");
  correr();
  revisa(velMotorA() == -90 && velMotorB() == 90, "orden desconocida no cambia nada");

  // 6. LEDs: naranja (3 en enum Color)
  texto(armarOrden('L', { 3 }));
  correr();
  revisa(pwm[LED_ROJO] == 255 && pwm[LED_VERDE] == 50 && pwm[LED_AZUL] == 0, "'L' 03 prende el naranja (255, 50, 0)");
  texto(armarOrden('L', { 8 }));   // DESCONOCIDO: no toca los LEDs
  correr();
  revisa(pwm[LED_ROJO] == 255 && pwm[LED_VERDE] == 50, "'L' con un color fuera de la tabla no cambia los LEDs");

  // 7. ¿Estás ahí?
  Serial.sale.clear();
  texto(armarOrden('?'));
  correr();
  revisa(Serial.sale.size() == 2 && Serial.sale[0] == '~' && Serial.sale[1] == '!', "'~?' contesta '~!'");

  // 8. Frenar
  ordenM(200, 200);
  texto(armarOrden('F'));
  correr();
  revisa(velMotorA() == 0 && velMotorB() == 0, "'F' frena y deja los motores en 0");

  // 9. Sin órdenes por más de 300 ms: se apaga solo
  ordenM(120, 120);
  correr();
  revisa(velMotorA() == 120, "antes del silencio va a 120");
  falsoMillis += 250;
  correr();
  revisa(velMotorA() == 120, "a los 250 ms sin órdenes sigue andando");
  falsoMillis += 100;
  correr();
  revisa(velMotorA() == 0 && velMotorB() == 0, "a los 350 ms sin órdenes se apaga");
  // un 'L' no cuenta como orden de motores
  ordenM(120, 120);
  correr();
  falsoMillis += 200;
  texto(armarOrden('L', { 1 }));
  correr();
  falsoMillis += 150;
  correr();
  revisa(velMotorA() == 0, "un 'L' no mantiene vivos los motores");

  // 10. El tope de PWM (batería 3S): aunque pidan 254, la llanta no pasa de PWM_MAXIMO
  ordenM(254, -254);
  correr();
  revisa(velMotorA() == PWM_MAXIMO && velMotorB() == -PWM_MAXIMO, "254 se queda en el tope PWM_MAXIMO");

  printf("\n%d fallas\n", fallas);
  return fallas;
}
