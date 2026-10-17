# Código de Candidates 2026 (borrador)

La propuesta de programación del 19 de septiembre, ya en código y adaptada al esquemático de electrónica del 15 oct (`candidates schematic.pdf`) y a su versión final del 16 oct (`Candidates_esquematicofinal.pdf`), que tiene las mismas conexiones. El robot lleva **dos Arduino Uno** unidos por serial, así que son tres partes:

- `robot/`: el programa del Arduino de los **sensores** (U2 en el esquemático). Tiene toda la lógica.
- `motores/`: el programa del Arduino de los **motores** (U4). Solo recibe órdenes, mueve el L298N y prende los LEDs del color.
- `simulador/`: una pista falsa en la computadora. Corre **la misma lógica** que va al robot y dice por dónde fue, cuánto tardó y cuántos puntos habría sacado.

Es un borrador. La lógica está probada en el simulador. La parte física (`robot/hardware_robot.cpp` y `motores/motores.ino`) sigue los pines del esquemático, pero todavía no se ha probado en un robot.

## Cómo correrlo

**El simulador**, desde la terminal de VS Code:

```
cd simulador
.\compilar.bat             todas las pruebas
.\compilar.bat A           solo la pista A
.\compilar.bat B -v        solo la B, con cada movimiento
.\simulador.exe --vel 25   otra velocidad del robot, en cm/s (ya compilado)
```

Usa el g++ de MSYS2 en `C:\msys64\ucrt64\bin`. Si está en otro lado, se cambia la línea `set GPP=` de `compilar.bat`.

**La prueba del serial entre los dos Arduino**, también sin robot: `.\probar_motores.bat` desde `simulador`. Compila el `motores.ino` de verdad con un Arduino falso y le manda texto revuelto con órdenes. Se corre cada vez que se toque cómo se hablan los dos Arduino.

**El robot**: son dos programas, uno por Arduino, y cada uno se sube con su propio cable USB. En el Arduino IDE la placa es **Arduino Uno** en los dos. **Antes de subir, desconectar el cable que llega al D0 del Arduino que se va a programar**: D0 y D1 son los mismos pines del USB, y el otro Arduino no deja pasar lo que manda la computadora. Al terminar se vuelve a conectar.

| Carpeta | Va en | Librerías | Memoria de programa | RAM fija |
|---|---|---|---|---|
| `robot/robot.ino` | U2, el de los ultrasónicos | Servo, Adafruit TCS34725 | 62% | 77% (1,577 de 2,048 bytes) |
| `motores/motores.ino` | U4, el del L298N y los LEDs | ninguna | 8% | 11% |

### Primera prueba con el robot

**1. Antes de conectar los Arduino**, medir con multímetro que el LM2596 dé 5.0 V.

**2. Qué se sube a cuál Arduino.** En el Arduino IDE la placa es Arduino Uno en los dos.

| Programa | Va en | Cómo reconocerlo | Librerías |
|---|---|---|---|
| `robot/robot.ino` | U2, el de sensores | el que tiene los 3 ultrasónicos, el servo y el switch del pocket | Adafruit TCS34725 (pide también Adafruit BusIO) y Servo |
| `motores/motores.ino` | U4, el de motores | el que va al L298N y a los LEDs | ninguna |

Antes de subir, desconectar el cable que llega al D0 de ese Arduino, y volverlo a conectar al terminar.

**3. Arrancar.** No hay botón: el botón es el **micro switch de la pelota** (A3), apretado con el dedo. Se aprieta y se suelta para arrancar. Si está apretado al prender, entra a calibrar colores, así que **no hay que prender el robot con la pelota dentro del pocket**. El switch de pista es D12: suelto corre la A y a GND corre la B; solo cuenta al prender.

Los dos micro switches (el de la pelota y cualquier otro) se conectan igual: la pata **C** a GND, la pata **NO** al pin y la **NC** sin nada. Con NC se leería al revés.

**4. El modo de pruebas** (`robot/pruebas.cpp`). Prueba cada pieza por separado y dice qué número cambiar:

1. Batería prendida y USB conectado al Arduino de sensores.
2. El cable del D0 de ese Arduino desconectado; el del D1 conectado, porque es el que lleva las órdenes a los motores.
3. Monitor serial a 115200. Reiniciar el Arduino y escribir `p` y Enter en los primeros 3 segundos.
4. Va a decir "No contesta el Arduino de motores". Con el D0 desconectado es normal.

Orden recomendado. Las dos primeras pruebas de motores se hacen **con las llantas en el aire**:

| Letra | Qué hace | Qué se ajusta |
|---|---|---|
| `s` | imprime distancias, color y switches; tapa cada ultrasónico con la mano | `TRIG_` y `ECO_` en `hardware_robot.cpp`, si un lado no coincide |
| `i` | dice qué hay conectado al I2C; el sensor de color sale en 0x29 | si no sale nada: cables del sensor (SDA en A4, SCL en A5, VIN, GND) |
| `a` | avanza 30 cm | `INVERTIR_IZQUIERDO`, `INVERTIR_DERECHO` (`motores.ino`); en el piso, lejos de paredes: `CM_POR_S` nuevo = `CM_POR_S` × cm reales / 30 |
| `g` | gira 90 grados a la derecha | si gira a la izquierda, `A_ES_IZQUIERDO = false` (`motores.ino`) |
| `v` | da 4 vueltas | `MS_POR_GRADO` nuevo = `MS_POR_GRADO` × 4 / vueltas reales |
| `c` | abre o cierra la compuerta | `GARRA_ABIERTA`, `GARRA_CERRADA` |
| `l` | prende los LEDs en cada color | nada: es para ver que funcionen |

**5. Dónde se cambia cada cosa:**

- `motores/motores.ino`: hacia dónde gira cada motor y el tope de velocidad (`PWM_MAXIMO`). Después de cambiarlo, se vuelve a subir al Arduino de motores.
- `robot/hardware_robot.cpp`: pines, velocidades, `CM_POR_S`, `MS_POR_GRADO`, `AJUSTE_DER` y ángulos de la compuerta. Después, se vuelve a subir al Arduino de sensores.
- `robot/config.h`: medidas del robot y de la pista. Después, se corre el simulador antes de subirlo.

**6. Calibrar los colores** en la pista: prender con el switch de la pelota apretado, soltarlo y seguir los mensajes del monitor serial. Cada vez que pida una casilla, poner el sensor encima y apretar y soltar el switch.

En el monitor serial salen cosas como `~M9FDF` o `~?` entre los mensajes: son las órdenes a los motores, y es normal. Si el sensor de color no aparece ("No encuentro el sensor de color"), en el modo de pruebas la letra `i` dice qué hay conectado al I2C: el TCS34725 debe salir en 0x29.

En el Uno la RAM es lo que aprieta (ver "La RAM del Uno" más abajo). Al compilar `robot/`, el IDE avisa "Low memory available" porque pasa de 75%: está revisado y alcanza. Pero si un cambio sube la RAM fija arriba de 80%, hay que revisarlo antes de subirlo al robot: el programa no avisa cuando se queda sin memoria, solo empieza a hacer cosas raras.

## Qué salió en el simulador

Con el robot a 15 cm/s y 0.9 s por giro. Son velocidades supuestas: hay que medirlas con el robot.

| Prueba | Tiempo | Puntos |
|---|---|---|
| Pista A, laberinto de la imagen 1.1 | 4:32 | 165 de 195 |
| Pista A, un laberinto inventado (otra salida, otro final) | 4:19 | 140 de 195 |
| Pista B completa, imagen 2.1 | 1:53 | 240 de 240 |
| Pista B con las 4 posiciones de las paredes de la pelota (imagen 2.2) | de 1:53 a 2:18 | 240 de 240 en las 4 |
| Reinicio en el checkpoint 1, con las líneas de la imagen 2.4 | 1:18 | 155 de 155 |
| Reinicio en el checkpoint 2, con la sección 3 de la imagen 2.7 | 0:20 | 85 de 85 |

En la pista A faltan los 30 puntos del ArUco, que no está hecho. En el laberinto inventado además falta un tope (ver "Lo que falta").

El simulador también revisa dos cosas que no son puntos:

- **El mapa que aprende el robot coincide con la pista**: 96 de 96 lados bien en los dos laberintos. El robot nunca ve el mapa, solo recibe lecturas de sensores. Eso es lo que pide la regla de no pre-mapeo.
- **La sección 3 imprime la misma prueba de escritorio de la propuesta**, paso por paso. En el paso 5 la casilla dice "derecha" y el robot gira a la izquierda porque viene bajando.

A 10 cm/s el robot decide no intentar el bonus 1 porque no le alcanzaría, y se queda con 130 puntos en la imagen 1.1.

## Cómo está organizado

La idea que ordena todo: la lógica nunca sabe si está en el robot o en la computadora. Solo habla con `hardware.h`.

```
   pista_a.cpp      pista_b.cpp        qué hacer en cada pista
          \            /
        explorador.cpp                 DFS con pila, BFS y el recorrido
     colores.cpp    base.cpp           colores; rumbos y moverse por casillas
               |
          hardware.h                   lo que la lógica le pide al robot
          /         \
 hardware_robot.cpp   simulador/mundo.cpp
 (Arduino U2)         (la pista falsa)
       |
       |  serial: "llanta izquierda a tanto, derecha a tanto", "LEDs en naranja"
       v
 motores/motores.ino
 (Arduino U4 -> L298N y LEDs)
```

| Archivo | Qué tiene | En la propuesta del 19 sep |
|---|---|---|
| `config.h` | Todos los números que se ajustan con pruebas | Los "20 cm" y "5 lecturas" de ejemplo |
| `base.h`, `base.cpp` | Rumbos del 0 al 3, `girarHacia` con la resta módulo 4, moverse por casillas | La base: casillas y rumbos |
| `colores.h`, `colores.cpp` | Calibrar y leer el color con votación de 5 lecturas | Por qué calibrar y no fijar umbrales |
| `explorador.h`, `explorador.cpp` | DFS con pila, BFS al rojo y el recorrido del bonus 1 | Pista A y sección 1 de la B |
| `pista_a.cpp` | Explorar, ir al rojo y el bonus 1 | Pista A, partes 1 y 2 |
| `pista_b.cpp` | Saber en qué sección arranca y las 3 secciones | Pista B |
| `hardware.h` | La frontera entre la lógica y el robot, y la macro `TXT` | Lo que el software le pide al robot |
| `hardware_robot.cpp` | Pines del esquemático, mover el robot sin encoders, hablar con U4 | Nuevo |
| `robot.ino` | Al prender: calibrar, elegir pista, esperar el botón | Nuevo |
| `pruebas.cpp` | Modo de pruebas para medir el robot de verdad (solo en el robot) | Nuevo (16 oct) |
| `motores/motores.ino` | Recibir órdenes por serial, mover el L298N y prender los LEDs | Nuevo (9 oct; serial y LEDs desde el 16 oct) |
| `simulador/prueba_motores.cpp` | Probar el serial entre los dos Arduino sin robot | Nuevo (16 oct) |

Para probar otro laberinto se dibuja con texto en `simulador/simulador.cpp` (ver `LABERINTO_1_1`).

## Qué cambió respecto a la propuesta, y por qué

Cada cambio salió de revisar la propuesta contra el reglamento o de una prueba que falló en el simulador.

### Pista A

1. **En la salida nadie ve atrás.** El robot no tiene sensor atrás, así que en la casilla de inicio no sabía si detrás había pared o camino. Ahora al empezar gira una vez a la derecha y el sensor derecho lee lo que tenía atrás.
2. **La rampa tiene un tramo plano arriba.** Mide 15 + 30 + 15 cm (imagen 3.2). La propuesta decía "al volver a quedar plano, cuenta 2 casillas", pero arriba de la rampa el robot también queda plano y contaba mal. Ahora el robot se para en el centro de cada casilla y, si queda inclinado (unos 7 grados), sabe que está en la rampa. Ahí no confía en los sensores: la rampa es un pasillo con paredes a los lados.
3. **La rampa se cruza completa.** En el laberinto inventado la rampa une dos zonas que el robot ya conocía. El DFS se metía, veía que la salida ya estaba visitada y se regresaba sin cruzarla: perdía 20 puntos. Ahora en la rampa siempre sigue derecho hasta salir del otro lado.
4. **Ya no regresa hasta la salida al terminar de explorar.** En la propuesta el DFS terminaba cuando la pila quedaba vacía, o sea cuando ya había regresado hasta el inicio. Es tiempo perdido, porque luego tiene que ir al rojo. Ahora deja de explorar en cuanto no queda ninguna casilla conocida con un lado libre hacia algo nuevo, y de ahí va directo al rojo por BFS. Eso también acorta el regreso del bonus 1.
5. **De reversa en vez de media vuelta.** Cuando la casilla a la que va le queda atrás (al salir de un callejón), se va de reversa. Es más rápido y nunca gira encima de la rampa.
6. **El bonus 1 se decide con el tiempo que ya usó.** El regreso son las mismas casillas, pero sin asomarse ni leer paredes, así que tarda menos que la ida. Si le queda más tiempo del que ya usó, le alcanza. Con un número fijo de segundos por casilla, a 10 cm/s el robot empezaba el regreso y se le acababa la ronda a medio camino.

### Pista B

7. **El hueco de la sección 2 no siempre es una fila.** En la imagen 2.4 la primera línea deja el hueco de 30 cm a media altura. Con un robot de 20 cm de ancho, centrado en cualquiera de las dos filas, una llanta pisa la línea: "cambiar de fila" no alcanza. Ahora, al ver una línea, el robot se recorre de lado de 5 en 5 cm hasta que los dos sensores de piso quedan libres, se recorre 3 cm más para ir holgado y cruza. Funciona con las líneas de la imagen 2.1 y con las de la 2.4.
8. **La franja blanca del checkpoint parece línea.** Entre casillas de distinto color hay una franja blanca (reglamento, sección 3), y los sensores de piso la ven igual que una línea de 2 cm. Al salir del checkpoint 1 el robot no les hace caso hasta que el sensor de color ve verde. Al final, pasando los 106 cm ya no puede haber líneas si siempre van en las divisiones entre casillas (pregunta 3): el robot vuelve al centro y entra derecho al checkpoint 2, que mide 30 cm y es de donde sale la sección 3.
9. **El verde tiene tres tonos.** El inicio, el piso de la sección 2 y el fin son verdes distintos. Con los tonos de las imágenes del reglamento y una sola referencia de verde, el piso de la sección 2 queda más cerca del cian que del verde: después de un reinicio en el checkpoint 1, el robot creería que está en la sección 3. Ahora la calibración acepta varias referencias por color y en la pista B se calibran los tres verdes. Además el sensor divide cada lectura entre el brillo, para que una sombra no cambie el color.
10. **La calibración se guarda en la EEPROM.** Después de un lack of progress el capitán reinicia el robot, y durante la ronda ya no se puede calibrar. En la memoria normal se perdería con el reinicio. Se guardan tonos, no posiciones: no es pre-mapeo.
11. **Al llegar al checkpoint 1 se voltea hacia la sección 2.** El BFS a veces entra de reversa, y la sección 2 necesita arrancar viendo hacia adelante.

## El robot del esquemático (15 oct)

El borrador del 26 sep suponía un Arduino Mega con encoders, IMU y pantalla. El esquemático v1.0 de electrónica (28 sep, `esquematico1candidates.pdf`) era otro robot, y el 9 oct el código se adaptó a él. El 15 oct electrónica lo cambió (`candidates schematic.pdf`: sigue diciendo V1.0, pero trae "Update at 2026-10-15"). Lo que trae ahora, trazado cable por cable desde el PDF:

| Pieza | Conectada a | Pines |
|---|---|---|
| Ultrasónico U1 | U2 | TRIG D2, ECHO D3 |
| Ultrasónico U5 | U2 | TRIG D9, ECHO D10 |
| Ultrasónico U6 | U2 | TRIG D5, ECHO D4 |
| Sensor de color 1 (VIN, GND, SDA, SCL, LED, INT) | U2 | I2C: A4 y A5. Su LED va a 5 V: siempre prendido |
| Sensor de color 2 | U4 | I2C: A4 y A5, igual que el otro. El código todavía no lo usa |
| Driver L298N | U4 | ENA D9 (ver "Para revisar con electrónica"), IN1 D8, IN2 D7, ENB D10, IN3 D6, IN4 D5 |
| 3 LEDs, cada uno con su resistencia de 4.7 kΩ a GND | U4 | D4, D3 y D2 |
| U2 con U4 | entre ellos | serial: el D1 (TX) de cada uno al D0 (RX) del otro |
| Batería (XT60) | L298N y regulador LM2596 de 5 V | el regulador alimenta los dos Arduino por su pin 5V y los sensores |

### Qué cambió el 15 oct y qué se hizo en el código

- **Dos sensores de color, uno por Arduino.** El TCS34725 tiene una dirección I2C fija (0x29), así que dos no pueden ir en los mismos cables: el Arduino no sabría a cuál le está hablando. Por eso cada uno quedó en el I2C de su Arduino, y los Arduino dejaron de compartir el I2C. La lógica sigue usando el de U2. El de U4 no se usa hasta saber para qué lo pusieron.
- **Los Arduino ahora se hablan por serial.** Los cables de D0 y D1 ya van cruzados bien (era el punto 1 de la revisión del 9 oct). `motores.ino` y `hardware_robot.cpp` pasaron de I2C a serial; abajo está cómo.
- **Tres LEDs en U4.** U2 le manda a U4 el color que vio y U4 prende los LEDs. El código los trata como rojo (D4), verde (D3) y azul (D2), pero en el esquemático los tres son rojos (ver "Para revisar con electrónica").
- **Se fueron las resistencias de 4.7 kΩ del I2C** (era el punto 5). Ahora son las de los LEDs. El módulo de Adafruit trae las suyas, así que el I2C no las necesita.
- **El cable de ENA quedó encimado sobre la línea de 5 V.** Es lo más urgente para electrónica.

Lo demás no se movió: los ultrasónicos y los pines del L298N son los mismos. La lógica de las pistas tampoco cambió: el simulador saca las mismas 199 líneas que el 9 oct.

**La versión final del 16 oct** (`Candidates_esquematicofinal.pdf`) ya no dibuja cables largos: cada patita lleva una etiqueta, y las patitas con el mismo nombre quedan conectadas. Revisé que cada etiqueta de señal aparezca exactamente dos veces, escrita igual en los dos extremos. Las conexiones son las mismas del 15 oct, **ya sin el cable de ENA encimado en 5 V**: ENA va solo a D9 de U4. No hubo que cambiar nada del código. Sigue sin botón, switch, garra, sensores de piso ni motores, y los LEDs siguen siendo rojos con 4.7 kΩ.

Lo que contestó electrónica (16 oct): los dos sensores de color son para tener más precisión; la garra funciona con un servo normal; no hay botón ni switch, y el equipo está viendo poner un switch abajo de la garra para arrancar y otro para elegir la pista.

### Mapa de pines

Para mover una pieza de pin solo se cambia el número en su constante: todo lo demás usa la constante.

**U2, el Arduino de sensores** (`robot/hardware_robot.cpp`):

| Pieza | Pin | ¿En el esquemático? | Constante |
|---|---|---|---|
| Ultrasónico de enfrente (U1, supuesto) | TRIG D2, ECHO D3 | sí | `TRIG_FRENTE`, `ECO_FRENTE` |
| Ultrasónico izquierdo (U5, supuesto) | TRIG D9, ECHO D10 | sí | `TRIG_IZQ`, `ECO_IZQ` |
| Ultrasónico derecho (U6, supuesto) | TRIG D5, ECHO D4 | sí | `TRIG_DER`, `ECO_DER` |
| Sensor de color 1 | A4 (SDA), A5 (SCL) | sí | fijos: son los únicos pines I2C del Uno |
| Serial con U4 | D0 (RX), D1 (TX) | sí | fijos: es el serial del Uno |
| Servo SG90 de la compuerta | D7 | no, propuesto | `SERVO_GARRA`; ángulos en `GARRA_ABIERTA`, `GARRA_CERRADA` |
| Botón de arranque | A3, el mismo micro switch de la pelota | no hay botón aparte (16 oct) | `BOTON` |
| Switch de pista A o B | D12 | no, propuesto | `SELECTOR` |
| Micro switch del fondo del pocket (ve la pelota) | A3 | no, propuesto | `SENSOR_GARRA`, `PELOTA_DETECTADA` |
| Sensores de piso | A0 y A1 | no, propuesto | `LINEA_IZQ`, `LINEA_DER`, `LINEA_ES_BLANCO` |
| Sensor bajito (Sharp) | A2 | no, y el diseño de mecánica no lo trae | `SHARP_BAJO`; se activa con `HAY_SENSOR_BAJO` en `config.h` |

Libres en U2: D6, D11 y D13.

**U4, el Arduino de motores** (`motores/motores.ino`):

| Pieza | Pin | ¿En el esquemático? | Constante |
|---|---|---|---|
| Motor A del L298N | ENA D9, IN1 D8, IN2 D7 | sí | `ENA`, `IN1`, `IN2` |
| Motor B del L298N | ENB D10, IN3 D6, IN4 D5 | sí | `ENB`, `IN3`, `IN4` |
| LEDs | rojo D4, verde D3, azul D2 | sí, pero los tres son rojos | `LED_ROJO`, `LED_VERDE`, `LED_AZUL` |
| Sensor de color 2 | A4 (SDA), A5 (SCL) | sí | todavía no se lee en el código |
| Serial con U2 | D0 (RX), D1 (TX) | sí | fijos |

Libres en U4: D11, D12, D13 y de A0 a A3. Cuál motor es el izquierdo y si alguno va al revés se cambia en `A_ES_IZQUIERDO`, `INVERTIR_IZQUIERDO` e `INVERTIR_DERECHO`.

Reglas para mover un pin:

- **Lo que necesita PWM** (la velocidad de los motores, ENA y ENB, y el LED verde) solo puede ir en D3, D5, D6, D9, D10 o D11.
- **El servo se queda en U2.** La librería Servo le quita el PWM a D9 y D10 del Arduino donde corre, y en U4 esos dos son la velocidad de los motores.
- **Un switch va entre el pin y GND**, sin resistencia: el código prende la resistencia interna del Arduino (`INPUT_PULLUP`), así que suelto se lee HIGH y apretado LOW.
- **A0 a A3 sirven también como pines digitales.**
- **No usar D0, D1, A4 ni A5** para otra cosa.

### Por qué la lógica va en U2 y U4 solo mueve motores

Los ultrasónicos se miden con un cronómetro (`pulseIn`) en el mismo Arduino donde están conectados, y la lógica los lee todo el tiempo. A los motores, en cambio, basta con mandarles una orden cada pocos milisegundos. Por eso el cerebro va donde están los sensores, y U4 queda como un traductor: recibe "izquierda a tanto, derecha a tanto" y se lo pasa al L298N. Si un día se quiere cambiar algo de cómo se mueve el robot, se cambia en U2; `motores.ino` casi no se toca.

Se hablan por serial, con un cable de ida y uno de vuelta. El serial de un Uno es el mismo del USB, así que por ese cable a U4 también le llegan los mensajes de texto que U2 imprime para nosotros (`hw::mensaje`). Para que U4 distinga una orden de un texto, cada orden empieza con `~`, que ningún mensaje usa, y los números van en hexadecimal (dos letras de 0-9 y A-F por cada byte):

| Orden | Ejemplo | Qué hace U4 |
|---|---|---|
| `M` | `~M9FDF` | pone las velocidades de la izquierda y la derecha |
| `F` | `~F` | frena en seco 80 ms y suelta |
| `L` | `~L03` | prende los LEDs de ese color (el número del `enum Color` de `base.h`: 03 es naranja) |
| `?` | `~?` | contesta `~!`. U2 pregunta al arrancar, hasta por 2 s, para saber que U4 está vivo |

Cada velocidad va en un solo byte: la mitad de la velocidad, más 127 para que no haya negativos. Queda de 00 a FE (7F es parado). Se pierde la mitad de la precisión, porque va de 2 en 2, y el motor no lo nota. Lo que llegue entre una orden y la siguiente es texto, y U4 lo ignora. Si una orden llega cortada o con una letra que no es hexadecimal, se tira, y el siguiente `~` vuelve a empezar. Todo esto lo comprueba `simulador/prueba_motores.cpp`.

La primera versión (16 oct) empezaba cada orden con el byte 255 y mandaba los números como bytes sueltos. Al probarla en el robot, el monitor serial del Arduino IDE se comía las letras que venían después del 255 y del 0, y no se podían leer los mensajes de la calibración. Por eso ahora todo va como texto.

Si U4 pasa 300 ms sin recibir órdenes de motor, apaga los motores. Así, si U2 se reinicia o se zafa un cable, el robot se para en vez de seguir avanzando contra una pared.

Dos efectos de usar el serial del USB: con U2 conectado a la computadora, el monitor serial muestra las órdenes (`~M9FDF`) entre los mensajes (es normal), y para subir un programa hay que desconectar el cable que llega al D0 de ese Arduino.

### Moverse sin encoders ni IMU

Sin encoders el robot no puede contar cuánto avanzó, y sin IMU no sabe cuánto giró. `hardware_robot.cpp` hace lo mejor que se puede con tres ultrasónicos:

- **Avanzar con la pared de enfrente.** Si hay pared a menos de 70 cm, avanzar 10 cm es "que la pared quede 10 cm más cerca". Es más exacto que el reloj. Más lejos de 70 cm el eco puede rebotar en las paredes de los lados, y se usa el tiempo.
- **Ir derecho con la pared de un lado.** Si hay una pared a menos de 12 cm a un lado, el robot trata de quedarse a la distancia a la que empezó. Es lo que hacía el IMU en el borrador del Mega.
- **Girar por tiempo.** Es lo más débil: el error de cada giro se va sumando. Lo arregla un IMU.

Hay que medir con el robot (están al principio de `hardware_robot.cpp`): `CM_POR_S`, `MS_POR_GRADO` y `AJUSTE_DER`. **Cambian con la carga de la batería**, así que se miden con la batería como va a estar en la ronda.

### La RAM del Uno

El Uno tiene 2,048 bytes de RAM; el Mega tenía 8,192. El borrador del 26 sep, compilado para Uno, pedía **185% de la RAM y 101% de la memoria de programa**: no cabía. Ahora pide 77% y 62%. Lo que se hizo, sin cambiar la lógica:

1. **Los textos de los mensajes a la memoria de programa.** En el Uno cada texto entre comillas se copia a la RAM; los mensajes solos ocupaban 1.5 KB. La macro `TXT("...")` de `hardware.h` los deja en la memoria de programa, y `hw::mensaje` ahora funciona como `printf`. Por eso todos los mensajes van como `hw::mensaje(TXT("Casilla (%d, %d)"), x, y)`.
2. **Fuera la pantalla y el IMU**, que no están en el esquemático. La pantalla sola pedía 1 KB al arrancar.
3. **`MAX_RECORRIDO` de 200 a 100.** El recorrido más largo en el simulador es de 51 casillas.
4. **La cola del BFS guarda cada casilla como un solo número** (`x * 9 + y`) y la ruta reusa ese mismo arreglo.
5. **La calibración se lee de la EEPROM directo a donde se usa**, sin una copia de 100 bytes que se quedaba viva todo el programa.

La RAM fija no es todo: la pila crece con cada función que se llama. Sumando los tamaños de cada función en el binario compilado, el peor momento (el BFS imprimiendo un mensaje) pide unos 300 bytes de los 471 que quedan.

**Comprobado:** el simulador saca exactamente la misma salida, las 199 líneas, que la versión del 26 sep.

### Lo que falta en el esquemático

Cada pieza tiene un pin propuesto en los pines libres de U2. Mientras electrónica no la ponga, el código no hace nada raro: los pines tienen pull-up y se leen como "no hay línea", "no hay pelota", "botón suelto" y "pista A".

| Pieza | Para qué | Sin ella | Pin propuesto |
|---|---|---|---|
| LEDs de colores | mostrar el color detectado | ya están, pero los tres son rojos: en la pista A se pierden los 20 puntos de color (4 × 5) | ya tienen pin en U4: rojo D4, verde D3, azul D2 |
| Botón | calibrar y arrancar | se usa el micro switch de la pelota (A3) apretado con el dedo | A3 |
| Selector | elegir pista A o B | siempre corre la A | D12 |
| Compuerta con servo y micro switch en el pocket | tomar y llevar la pelota | pista B: 130 de 240 como máximo | servo D7, switch A3 |
| Sensor bajito (Sharp) | ver la pelota antes de llegar | ya no hace falta: la pelota se detecta con el micro switch del pocket | A2 (y `HAY_SENSOR_BAJO = true`) |
| 2 sensores de piso | ver las líneas de la sección 2 | pisar una línea es lack of progress | A0 y A1 |
| IMU (MPU-6050) | girar exacto y saber si está en la rampa | giros por tiempo; la rampa (20 puntos) se cruza a ciegas | ninguno: va al I2C de U2, con el sensor de color (tienen distinta dirección) |
| Encoders | avanzar exacto donde no hay pared enfrente | avance por tiempo, que cambia con la batería | D2 y D3 (los únicos con interrupción) ya están ocupados en los dos Arduino. Si se ponen, los LEDs de U4 se pasan a D11 (el verde, que necesita PWM), A0 y A1 |

El servo va en U2 a propósito: la librería Servo le quita el PWM a los pines 9 y 10 del Arduino donde corre, y en U4 esos dos pines son la velocidad de los motores.

### Para revisar con electrónica

Del 9 oct ya quedaron resueltos D0 y D1 (ahora cruzados bien) y las resistencias del I2C a 5 V (ya no están). El punto 1 quedó resuelto en la versión final del 16 oct; lo demás sigue abierto:

1. **Resuelto el 16 oct. El cable de ENA quedó encimado sobre la línea de 5 V.** En el esquemático anterior, ENA (pin 6 del L298N) iba a D9 de U4 por su propio camino. En el nuevo sigue la misma ruta, pero su tramo horizontal quedó justo encima de la línea de 5 V que va al VCC del ultrasónico U6, y EasyEDA los unió con dos puntos. Así como está dibujado, **D9 de U4 queda conectado directo a 5 V**: cada vez que el programa pone D9 en bajo para bajar la velocidad, hace un corto y el pin se quema. El código sigue la intención (ENA a D9 y a nada más). Hay que corregir el dibujo antes de cablear. Si en cambio querían ENA fijo a 5 V, hay que quitar el cable de D9 y avisarme, porque entonces la llanta del motor A ya no puede ir a media velocidad.
2. **Los tres LEDs son rojos** (XL-1608SURC-06, los tres iguales). El reglamento pide mostrar el color "con un LED de este color, display, pantalla", y con tres rojos no se pueden mostrar el cian, el amarillo, el naranja ni el rosa, los cuatro colores de la pista A: son 20 puntos. Tienen que ser uno rojo en D4, uno verde en D3 y uno azul en D2, o un LED RGB con esos tres pines. El verde va en D3 porque es el único de los tres pines con PWM, y el naranja y el amarillo necesitan el verde a medias.
3. **4.7 kΩ es mucha resistencia para un LED.** Con 5 V pasan entre 0.4 y 0.7 mA, y con la luz de la pista apenas se van a ver. Con 220 Ω quedan entre 8 y 14 mA, según el color, dentro de lo que aguanta un pin (20 mA).
4. **¿Para qué es el segundo sensor de color?** Está en el I2C de U4 y el código no lo usa. Dependiendo de para qué sea (¿la pelota?, ¿uno de cada lado para las líneas de la sección 2?), U4 lo lee y le manda el resultado a U2 por el cable de regreso, que hoy solo contesta "aquí estoy".
5. **¿Cuál ultrasónico quedó adelante?** El código supone U1 adelante, U5 a la izquierda y U6 a la derecha. Si no es así, se cambian 6 números al principio de `hardware_robot.cpp`.
6. **El L298N está dibujado como el chip suelto.** Si es el módulo rojo, está bien que VSS y Sense no lleven cable, pero hay que **quitar los jumpers de ENA y ENB** para que D9 y D10 controlen la velocidad. Si es el chip suelto, faltan VSS a 5 V, Sense A y B a GND y los diodos de los motores.
7. **El LM2596 también está dibujado como chip.** Si es el módulo azul ajustable, hay que **medir con multímetro que dé 5.0 V antes de conectar los Arduino**: de fábrica pueden venir en otro voltaje, y los 5 V entran directo al pin 5V, sin protección.
8. **Los motores no están dibujados.** Si al probar una llanta gira al revés o están cambiadas, se arregla en las primeras líneas de `motores/motores.ino`.

## El robot de mecánica (16 oct)

Mecánica tiene dos documentos: el instructivo de armado estilo LEGO y un modelo 3D (los dos en claude.ai). El instructivo no trae pines, pero el modelo 3D trae la posición de cada pieza. Las medidas de abajo son desde el centro del robot, el punto alrededor del cual gira: con 4 llantas, a la mitad entre los dos ejes.

| Pieza | Dónde va |
|---|---|
| Robot completo | 20.8 cm de ancho, 23.4 de largo y 9.1 de alto; llantas de 6.5 cm |
| 4 motores TT | dos por lado; los ejes a 6.5 cm adelante y 6.5 cm atrás del centro |
| Ultrasónico de enfrente | al frente, al centro, a 13.6 cm del centro del robot |
| Ultrasónicos de los lados | uno a cada lado, a la altura del centro del robot |
| 2 sensores de color (TCS34725) | uno a cada lado, a 3.5 cm del centro; 1.2 cm adelante del centro; a 1.6 cm del piso, en cajas que tapan la luz |
| Pocket de la pelota | al frente, al centro, con un micro switch en el fondo que se aprieta cuando entra la pelota |
| Compuerta | una barra al frente que encierra la pelota; la mueve un servo SG90 en la esquina delantera derecha. Abre 45 grados |
| Batería | LiPo 3S de 2200 mAh, con interruptor general y XT60 atrás a la izquierda |
| Los dos Arduino, el L298N y el LM2596 | arriba del chasis; el servo se alimenta del LM2596, no del pin 5V del Arduino |

### Lo que se alineó en el código

- **La pelota se detecta con el micro switch del pocket.** El código la buscaba con un sensor bajito que este robot no tiene, así que nunca la hubiera agarrado: en el simulador, sin ese sensor, la pista B bajaba de 240 a 130. Ahora, en la sección 1, el robot explora con la compuerta abierta. Cuando entra a la casilla de la pelota, la pelota entra al pocket, aprieta el switch y la compuerta se cierra. Simulado: 240 de 240 en las cuatro posiciones de la pelota. Si algún día ponen el sensor bajito, se activa con `HAY_SENSOR_BAJO` en `config.h`, y el simulador obedece la misma constante.
- **El largo del robot** en `config.h` pasó de 20 a 24 cm. No cambia los resultados.
- **Nombres y comentarios:** la "garra" es una compuerta con un SG90, el sensor de la garra es el micro switch del pocket, y los 4 motores van de dos en dos en cada canal del L298N.

La referencia nueva del simulador es `bitacora/simulador_2026-10-16.txt`, de 198 líneas. Contra la del 9 oct solo cambia la sección 1 de la pista B.

### Lo que no cuadra (para mecánica)

Esto no se arregla en el código: o cambia una pieza, o cambia la estrategia. Lo probé en el simulador poniéndole las medidas del modelo 3D una por una.

1. **Los sensores de color están casi en el centro del robot.** El código lee el color de la casilla de enfrente asomándose 10 cm, y supone que el sensor va 8 cm delante del centro. Con el sensor a 1.2 cm, al asomarse el sensor sigue leyendo su propia casilla: el robot anota los colores en la casilla equivocada y no reconoce el rojo hasta que ya está encima. Simulado, la pista A baja de 165 a 110, y de 140 a 85 en el laberinto inventado, sin bonus 1. Asomarse más (17 cm) tampoco sirve: para que el sensor llegue a la casilla de enfrente, el centro del robot tiene que pasar a esa casilla, o sea que ya la pisó. **Los sensores de color tienen que ir unos 8 a 10 cm delante del centro.** En el modelo hay unos 3 cm libres a cada lado del pocket, entre el pocket y la llanta delantera, donde cabría cada caja si se gira 90 grados.
2. **El ultrasónico de enfrente queda demasiado cerca de la pared.** Con el robot en el centro de la casilla, una pared enfrente le queda a menos de 2 cm, y el HC-SR04 no mide bien a menos de 2 o 3 cm: puede leer "lejos" y el robot avanzaría contra la pared. Conviene montarlo unos 4 cm más atrás.
3. **El robot casi no cabe girando dentro de una casilla.** Al girar en su lugar, la punta más lejana (los brazos de la compuerta y las llantas) describe un círculo de unos 14.5 cm de radio, y la casilla da 15 cm menos el grosor de la pared. Va a rozar si no gira exacto.
4. **No hay sensores de piso para la sección 2.** Para no pisar una línea, el robot tiene que verla antes de que la llanta delantera llegue a ella. Los sensores de color van detrás de las llantas de adelante y a 3.5 cm del centro, así que no sirven para eso.
5. **Batería 3S con motores TT.** Una LiPo 3S llena da 12.6 V, y con el L298N a los motores les llegan unos 10 V a toda velocidad. Si sus motores TT son de 3 a 6 V, hay que limitar la velocidad máxima en `motores.ino`. Confirmar de cuántos volts son.

## Lo que falta

- **ArUco (bonus 2).** No está empezado. Pide una cámara y un procesador que corra visión.
- **Probar en el robot.** Medir `CM_POR_S`, `MS_POR_GRADO` y `AJUSTE_DER`, confirmar qué ultrasónico quedó dónde, y las piezas de "Lo que falta en el esquemático".
- **Topes que no pisa.** El DFS pisa todas las casillas, pero no todos los pasos entre casillas. Un tope en un paso que cierra un ciclo se puede quedar sin pisar: en el laberinto inventado pisó 2 de 3 (5 puntos cada uno).
- **Avanzar de 1 en 1 cm.** La sección 2 avanza de centímetro en centímetro revisando los sensores de piso. En el robot cada `hw::avanzar(1)` arranca y frena, así que va a ir lento; conviene una función que avance sin frenar mientras revisa los sensores.
- **La desviación de lado en la sección 2.** El simulador mide perfecto. El robot real se va a desviar unos centímetros con cada movimiento de lado; si la sección tiene paredes, los sensores de los lados pueden corregirlo.
- **Pelota perdida.** Si se le cae la pelota a medio camino, el robot no la vuelve a buscar.
- **La RAM del Uno.** Va en 77%. Lo que se puede recortar después: los nombres de colores y rumbos que siguen en RAM (unos 150 bytes) y empacar el mapa del explorador (324 bytes en `lado`).

## Preguntas para los organizadores

Las seis de la propuesta, actualizadas, y tres nuevas.

1. **Bonus 1:** ¿"el mismo camino" incluye las casillas de los callejones? El código hoy regresa por todas. En la imagen 1.1 son 50 casillas y a 15 cm/s tarda 2:09.
2. ¿Pisar el rojo antes de terminar de explorar acaba la ronda? El código supone que sí y lo deja para el final.
3. **Sección 2:** la imagen 2.4 ya contesta que el hueco puede quedar a media altura. Falta saber si las líneas siempre van en las divisiones entre casillas y si hay paredes alrededor de la sección.
4. ¿Podemos usar librerías de sensores (hoy: Adafruit TCS34725 para el color y Servo para la garra) y OpenCV para el ArUco?
5. ¿Cuánto tiempo tiene que verse el color en el LED? Hoy se queda prendido hasta el siguiente color.
6. ¿De qué color es el piso del laberinto y de la sección 1? ¿La sección 1 tiene paredes alrededor? El DFS las necesita para saber dónde se acaba.
7. **Nueva:** ¿cuenta la rampa si el robot sube y se regresa por el mismo lado? El código la cruza completa de todos modos.
8. **Nueva:** ¿qué tan ancha es la franja blanca entre casillas de distinto color?
9. **Nueva:** ¿el checkpoint 2 siempre queda centrado con la sección 2?
