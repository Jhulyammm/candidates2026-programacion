// config.h
// Todos los números que se ajustan con pruebas en la pista. Si cambia el robot
// (su tamaño, dónde van los sensores, qué tan rápido va) se cambia aquí y en
// ningún otro archivo.
//
// Los valores de ahorita salen de las medidas del reglamento y del modelo 3D de
// mecánica (16 oct): un robot de 20.8 x 23.4 cm. Hay que medirlos con el robot
// de verdad.
//
// "El centro del robot" es el punto alrededor del cual gira: con 4 llantas, a
// la mitad entre el eje de adelante y el de atrás.

#pragma once

// ------------------------------------------------------------------ La pista
const int CM_CASILLA = 30;           // una "unidad" del reglamento: 30 x 30 cm (± 2)

// ------------------------------------------------------------------ El robot
const int LARGO_ROBOT = 24;          // de la defensa a la cola, en cm (modelo 3D: 23.4)
// OJO: en el modelo 3D los dos sensores de color van a 1 cm del centro, no a 8.
// Con 1 cm, al asomarse el sensor sigue leyendo su propia casilla y el robot no
// reconoce el rojo hasta pisarlo (simulado: la pista A baja de 165 a 110). Se
// deja en 8 hasta que mecánica los mueva hacia adelante (ver el README).
const int SENSOR_COLOR_ADELANTE = 8; // el sensor de color va 8 cm delante del centro

// ------------------------------------------------------------------ Paredes
// Con el sensor de enfrente a 10 cm del centro, una pared pegada queda a unos
// 5 cm y la de la casilla siguiente a unos 35 cm. 20 cm queda en medio.
// OJO: en el modelo 3D el ultrasónico de enfrente va a 13.6 cm del centro, así
// que una pared pegada le queda a menos de 2 cm, donde el HC-SR04 ya no mide.
const int DIST_PARED = 20;

// ------------------------------------------------------------------ Asomarse
// Antes de entrar a una casilla nueva el robot avanza un poco y lee su color.
// Con el sensor a 8 cm del centro, 10 cm de avance lo dejan 3 cm adentro de la
// casilla de enfrente (ya pasó la franja blanca entre casillas) y el robot
// todavía no tiene la mitad adentro, así que no cuenta como que entró.
const int DIST_ASOMARSE = 10;

// ------------------------------------------------------------------ Color
const int LECTURAS_COLOR = 5;        // 5 lecturas y gana la que más se repite

// ------------------------------------------------------------------ Rampa
// Parado en el centro de una casilla de la rampa, el robot queda inclinado unos
// 7 grados (la mitad de sus llantas en la subida). En topes y escaleras queda plano.
const int GRADOS_RAMPA = 5;

// ------------------------------------------------------------------ Tiempos (ms)
const unsigned long MS_RONDA = 360000UL;      // 6 minutos por pista
const unsigned long MS_EXPLORAR = 210000UL;   // 3:30 para explorar el laberinto; lo demás
                                              // es para llegar al rojo y el bonus 1

// ------------------------------------------------------------------ Pelota (pista B, sección 1)
// El robot de mecánica (instructivo y modelo 3D) NO trae sensor bajito: la
// pelota se detecta cuando entra al pocket y aprieta el micro switch del fondo.
// Por eso, sin sensor bajito, en la sección 1 el robot explora con la compuerta
// abierta. Si algún día ponen el Sharp, con true vuelve a ver la pelota antes de
// llegar a ella. Con false, el robot (y el simulador) nunca leen ese sensor: un
// pin analógico sin nada conectado lee valores al azar.
const bool HAY_SENSOR_BAJO = false;
const int PELOTA_BAJO_MAX = 25;      // el sensor bajito ve algo a menos de 25 cm...
const int PELOTA_ALTO_MIN = 30;      // ...y el de paredes ve libre a más de 30 cm: es la pelota
const int MAX_AVANCE_PELOTA = 25;    // lo más que avanza para meter la pelota en la garra

// ------------------------------------------------------------------ Sección 2 (líneas blancas)
const int SECCION2_LARGO = 120;      // 4 casillas
const int RETROCESO_LINEA = 5;       // al ver una línea se echa 5 cm para atrás para poder girar
const int PASO_LATERAL = 5;          // busca el hueco moviéndose de lado de 5 en 5 cm
const int MARGEN_LATERAL = 3;        // al encontrarlo se recorre 3 cm más, para ir holgado
const int LATERAL_MAX = 18;          // no se aleja más de 18 cm del centro (la sección mide 60)
const int X_RECENTRAR = 106;         // la última línea puede estar a 90 cm del inicio de la
                                     // sección; pasando 106 cm ya no hay líneas
