@echo off
rem Abre o teste 3D. Se der erro, a janela fica aberta mostrando o motivo.
"%~dp0engine-cpp\build\release\korama3d.exe"
if errorlevel 1 pause
