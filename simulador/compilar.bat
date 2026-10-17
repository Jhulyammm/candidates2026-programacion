@echo off
rem Compila el simulador y lo corre. Los argumentos pasan al simulador:
rem   compilar.bat            todas las pruebas
rem   compilar.bat A          solo la pista A
rem   compilar.bat B -v       solo la pista B, con cada movimiento
rem
rem Compila la MISMA logica que va al robot (carpeta ..\robot), menos
rem hardware_robot.cpp, que en la computadora lo reemplaza mundo.cpp.

setlocal
pushd "%~dp0"
set GPP=C:\msys64\ucrt64\bin\g++.exe
set R=..\robot

"%GPP%" -std=c++17 -Wall -Wextra -O1 -I%R% ^
  %R%\base.cpp %R%\colores.cpp %R%\explorador.cpp %R%\pista_a.cpp %R%\pista_b.cpp ^
  mundo.cpp simulador.cpp -o simulador.exe
if errorlevel 1 (
  popd
  exit /b 1
)
"%~dp0simulador.exe" %*
popd
