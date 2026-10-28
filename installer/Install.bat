@echo off
rem Double-click installer. Pass a folder to skip auto-detection:  Install.bat "E:\Games\TWD Destinies"
if "%~1"=="" (
  powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0Install-Lazarus.ps1"
) else (
  powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0Install-Lazarus.ps1" -GameDir "%~1"
)
pause
