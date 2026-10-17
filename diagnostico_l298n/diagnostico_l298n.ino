// diagnostico_l298n.ino
// Prueba directa del L298N desde el Arduino de MOTORES (U4). NO es el programa de
// la ronda: después de usarlo se vuelve a subir motores/motores.ino.
//
// No usa el otro Arduino ni el serial: mueve cada lado por su cuenta, con pausas
// largas para medir con el multímetro. Los pines son los mismos del esquemático:
// ENA D9, IN1 D8, IN2 D7, ENB D10, IN3 D6, IN4 D5.
//
// Con las llantas en el aire. Cada paso dura 4 segundos y se anuncia por el
// serial del USB (monitor a 115200):
//
//   1. lado A hacia adelante    2. lado A de reversa
//   3. lado B hacia adelante    4. lado B de reversa
//   5. los dos hacia adelante
//
// Si en el paso 1 no se mueve nada, mide en ese momento:
//   - la entrada de 12V del L298N contra GND: debe dar la tensión de la batería
//   - el pin ENA: unos 3 a 4 V (es PWM; el multímetro lo promedia)
//   - el pin IN1: 5 V; el pin IN2: 0 V
// Lo que salga mal dice dónde está la falla.

const int ENA = 9, IN1 = 8, IN2 = 7;
const int ENB = 10, IN3 = 6, IN4 = 5;
const int VELOCIDAD = 180;           // el mismo tope que PWM_MAXIMO en motores.ino
const unsigned long PASO_MS = 4000;

static void lado(int en, int in1, int in2, int velocidad) {
  digitalWrite(in1, velocidad > 0 ? HIGH : LOW);
  digitalWrite(in2, velocidad < 0 ? HIGH : LOW);
  analogWrite(en, abs(velocidad));
}

static void paso(const __FlashStringHelper* texto, int a, int b) {
  Serial.println(texto);
  lado(ENA, IN1, IN2, a);
  lado(ENB, IN3, IN4, b);
  delay(PASO_MS);
  lado(ENA, IN1, IN2, 0);
  lado(ENB, IN3, IN4, 0);
  delay(1500);
}

void setup() {
  for (int p = 5; p <= 10; p++) pinMode(p, OUTPUT);
  Serial.begin(115200);
}

void loop() {
  Serial.println(F("--- Prueba del L298N, con las llantas en el aire ---"));
  paso(F("1. Lado A (OUT1 y OUT2) hacia adelante"), VELOCIDAD, 0);
  paso(F("2. Lado A de reversa"), -VELOCIDAD, 0);
  paso(F("3. Lado B (OUT3 y OUT4) hacia adelante"), 0, VELOCIDAD);
  paso(F("4. Lado B de reversa"), 0, -VELOCIDAD);
  paso(F("5. Los dos hacia adelante"), VELOCIDAD, VELOCIDAD);
  delay(3000);
}
