@echo off
rem Removes Project Lazarus. Pass a folder to skip auto-detection:  Uninstall.bat "E:\Games\TWD Destinies"
if "%~1"=="" (
  powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0Install-Lazarus.ps1" -Uninstall
) else (
  powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0Install-Lazarus.ps1" -Uninstall -GameDir "%~1"
)
pause
