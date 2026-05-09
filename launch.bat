@echo off
title Rate Limiter Dashboard

:: Check if already running — don't start a second copy
tasklist /fi "imagename eq rate_limiter.exe" 2>nul | find /i "rate_limiter.exe" >nul
if not errorlevel 1 (
    echo Server already running — opening browser...
    start "" "http://localhost:8080"
    exit /b 0
)

echo Starting Rate Limiter server...
start /min "" "%~dp0rate_limiter.exe"

:: Give the server a moment to bind the port
timeout /t 2 /nobreak >nul

echo Opening dashboard at http://localhost:8080
start "" "http://localhost:8080"
