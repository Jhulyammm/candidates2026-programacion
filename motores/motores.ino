// motores.ino
// El Arduino de los MOTORES (U4 en el esquemático del 15 oct).
//
// No decide nada. Recibe por serial qué tan rápido mover cada llanta y se lo
// pasa al L298N, y prende los LEDs que le muestran al juez el color. Todo lo
// que piensa el robot está en el otro Arduino (U2, carpeta robot/), que es el
// que tiene los ultrasónicos.
//
// Cómo se hablan: por serial, un cable de ida y uno de vuelta. El D1 (TX) de U2
// llega al D0 (RX) de este, y el D1 de este al D0 de U2. Es el mismo serial del
// USB, así que por ese cable también llegan los mensajes de texto que U2
// imprime para nosotros. Para no confundirlos, cada orden empieza con "~", que
// ningún mensaje usa, y los números van en hexadecimal (dos letras de 0-9 y
// A-F por cada byte):
//
//   ~M7F7F    velocidades: izquierda y derecha, un byte cada una (00 a FE, 7F = parado)
//   ~F        frenar
//   ~L03      prender los LEDs de ese color (el orden de robot/base.h: 03 = naranja)
//   ~?        "¿estás ahí?": contesta ~!
//
// Lo que llega entre una orden y la siguiente es texto, y se ignora.
//
// Por qué texto y no bytes sueltos: la primera versión empezaba cada orden con el
// byte 255, y en la prueba del 16 oct el monitor serial del Arduino IDE se comía
// las letras que venían después de ese byte (y del 0): no se podían leer los
// mensajes de la calibración. Con texto, el monitor muestra la orden tal cual.
//
// Si el otro Arduino deja de mandar órdenes (se reinició, se trabó, se zafó un
// cable), a los 300 ms se apagan los motores. Así el robot nunca se queda
// avanzando solo contra una pared.
//
// PARA SUBIR ESTE PROGRAMA hay que desconectar el cable que llega a D0. Por ahí
// entra también lo que manda la computadora, y el otro Arduino no lo deja pasar.
//
// El esquemático le pone a este Arduino un sensor de color propio (en A4 y A5).
// Todavía no se usa: falta saber para qué lo pusieron.

const char INICIO = '~';             // el mismo que en robot/hardware_robot.cpp

// ------------------------------------------------------------------ Pines del esquemático del 15 oct
// El robot lleva 4 motores TT (modelo 3D de mecánica) y el L298N tiene 2
// canales: cada canal mueve las 2 llantas de un lado, conectadas juntas.
const int ENA = 9, IN1 = 8, IN2 = 7;    // motor A: salidas OUT1 y OUT2 del L298N
const int ENB = 10, IN3 = 6, IN4 = 5;   // motor B: salidas OUT3 y OUT4
// En el módulo rojo del L298N, ENA y ENB traen un jumper puesto que los deja
// siempre al máximo. Hay que quitarlos para que D9 y D10 controlen la velocidad.
//
// CUIDADO: en el esquemático del 15 oct el cable de ENA (con D9) quedó encimado
// sobre la línea de 5 V que va al ultrasónico U6. Si se arma así, cada vez que
// este programa pone D9 en LOW lo une directo con 5 V y el pin se quema. Ese
// cable tiene que ir de ENA a D9 y a nada más, como en el esquemático anterior.

// Tres LEDs para mostrar el color: rojo en D4, verde en D3 y azul en D2. El
// verde va en D3 porque es el único de los tres pines con PWM, y el naranja y
// el amarillo necesitan el verde a medias. En el esquemático del 15 oct los
// tres son rojos: hay que cambiarlos (ver el README).
const int LED_ROJO = 4, LED_VERDE = 3, LED_AZUL = 2;

// ------------------------------------------------------------------ Lo que falta saber de mecánica
// El esquemático no dibuja los motores. Si al avanzar el robot gira o va de
// reversa, se arregla aquí y en ningún otro lado.
const bool A_ES_IZQUIERDO = true;
const bool INVERTIR_IZQUIERDO = false;
const bool INVERTIR_DERECHO = false;

// La batería es una LiPo 3S (12.6 V llena) y con el L298N a los motores les
// llegan unos 10 V a todo PWM. Los motores TT suelen ser de 3 a 6 V: con este
// tope les llegan como 7 V en promedio, lo que daría una batería 2S. Si
// confirman que sus motores aguantan más, se sube.
const int PWM_MAXIMO = 180;

const unsigned long MS_SIN_ORDENES = 300;
const unsigned long MS_FRENADO = 80;
unsigned long ultimaOrden = 0;

// ------------------------------------------------------------------ L298N
// Adelante: IN1 alto e IN2 bajo. Atrás: al revés. La velocidad es el PWM del EN.
static void motor(int en, int in1, int in2, int velocidad) {
  velocidad = constrain(velocidad, -PWM_MAXIMO, PWM_MAXIMO);
  digitalWrite(in1, velocidad > 0 ? HIGH : LOW);
  digitalWrite(in2, velocidad < 0 ? HIGH : LOW);
  analogWrite(en, abs(velocidad));
}

static void mover(int izq, int der) {
  if (INVERTIR_IZQUIERDO) izq = -izq;
  if (INVERTIR_DERECHO) der = -der;
  if (A_ES_IZQUIERDO) {
    motor(ENA, IN1, IN2, izq);
    motor(ENB, IN3, IN4, der);
  } else {
    motor(ENA, IN1, IN2, der);
    motor(ENB, IN3, IN4, izq);
  }
}

// Las dos entradas iguales con el EN prendido frenan el motor en seco. Soltarlo
// nada más (EN en 0) lo deja seguir rodando unos centímetros.
static void frenar() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, HIGH);
  analogWrite(ENA, 255);
  analogWrite(ENB, 255);
  delay(MS_FRENADO);
  mover(0, 0);
}

// ------------------------------------------------------------------ LEDs
// Rojo, verde y azul de cada color, en el orden de enum Color (robot/base.h).
// En los pines sin PWM (rojo y azul), analogWrite prende de 128 para arriba.
const int NUM_COLORES = 7;
const uint8_t COLOR_LED[NUM_COLORES][3] = {
  { 255, 255, 255 },   // blanco
  { 0, 255, 255 },     // cian
  { 255, 160, 0 },     // amarillo
  { 255, 50, 0 },      // naranja
  { 255, 0, 140 },     // rosa
  { 255, 0, 0 },       // rojo
  { 0, 255, 0 },       // verde
};

static void mostrarColor(uint8_t c) {
  if (c >= NUM_COLORES) return;
  analogWrite(LED_ROJO, COLOR_LED[c][0]);
  analogWrite(LED_VERDE, COLOR_LED[c][1]);
  analogWrite(LED_AZUL, COLOR_LED[c][2]);
}

// ------------------------------------------------------------------ Leer las órdenes
// Llegan letra por letra. Cuántos bytes de datos trae cada orden (cada byte son
// dos letras hexadecimales); -1 = no es orden.
static int largoDe(char orden) {
  switch (orden) {
    case 'M': return 2;
    case 'L': return 1;
    case 'F': return 0;
    case '?': return 0;
    default: return -1;
  }
}

// '7' -> 7, 'F' -> 15. Lo que no es hexadecimal regresa -1.
static int valorHex(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  return -1;
}

static bool esperandoLetra = false;   // acaba de llegar un ~
static char orden = 0;                // la orden que se está leyendo; 0 = llega texto
static uint8_t datos[2];
static int llevamos = 0;              // cuántos bytes completos van
static int mitad = -1;                // la primera letra del byte que va a medias; -1 = ninguna

static void ejecutar() {
  switch (orden) {
    case 'M':
      // De vuelta a -254..254: es lo contrario de velocidadEnByte() en U2
      mover((datos[0] - 127) * 2, (datos[1] - 127) * 2);
      ultimaOrden = millis();
      break;
    case 'F':
      frenar();
      ultimaOrden = millis();
      break;
    case 'L':
      mostrarColor(datos[0]);
      break;
    case '?':
      Serial.write(INICIO);
      Serial.write('!');
      break;
  }
  orden = 0;
}

static void leerLetra(char c) {
  if (c == INICIO) {                  // empieza una orden, aunque la anterior no se haya completado
    esperandoLetra = true;
    orden = 0;
    return;
  }
  if (esperandoLetra) {
    esperandoLetra = false;
    llevamos = 0;
    mitad = -1;
    if (largoDe(c) < 0) return;       // letra desconocida: se ignora hasta el siguiente ~
    orden = c;
    if (largoDe(orden) == 0) ejecutar();
    return;
  }
  if (orden == 0) return;             // texto de los mensajes de U2: no es para este Arduino
  int v = valorHex(c);
  if (v < 0) {                        // a media orden llegó algo que no es número: se tira
    orden = 0;
    return;
  }
  if (mitad < 0) {                    // primera letra del byte: se guarda y se espera la otra
    mitad = v;
    return;
  }
  datos[llevamos++] = mitad * 16 + v;
  mitad = -1;
  if (llevamos == largoDe(orden)) ejecutar();
}

void setup() {
  pinMode(ENA, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(ENB, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(LED_ROJO, OUTPUT);
  pinMode(LED_VERDE, OUTPUT);
  pinMode(LED_AZUL, OUTPUT);
  mover(0, 0);
  Serial.begin(115200);               // la misma velocidad que en U2
}

void loop() {
  while (Serial.available()) leerLetra((char)Serial.read());
  if (millis() - ultimaOrden > MS_SIN_ORDENES) mover(0, 0);
}
