@echo off
rem Compila y corre la prueba del serial entre los dos Arduino.
rem Usa el motores.ino de verdad (carpeta ..\motores) con un Arduino falso.

setlocal
pushd "%~dp0"
set GPP=C:\msys64\ucrt64\bin\g++.exe

"%GPP%" -std=c++17 -Wall -Wextra -x c++ -I..\motores prueba_motores.cpp -o prueba_motores.exe
if errorlevel 1 (
  popd
  exit /b 1
)
"%~dp0prueba_motores.exe"
popd
