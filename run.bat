@echo off
set "PATH=C:\msys64\ucrt64\bin;%PATH%"
cd /d "%~dp0build\gui\GUI_APP"
start "" "GUI_App.exe"

PAUSE