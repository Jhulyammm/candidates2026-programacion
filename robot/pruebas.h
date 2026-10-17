// pruebas.h
// Modo de pruebas, para medir el robot de verdad. Solo existe en el robot: el
// simulador no compila pruebas.cpp.
#pragma once

bool quierenModoPruebas();   // al prender, espera 3 s una "p" desde la computadora
void modoPruebas();          // nunca regresa: se sale apagando o reiniciando el robot
