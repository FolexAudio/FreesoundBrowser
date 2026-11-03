@echo off
REM PTSL MCP Server - Windows Setup Script
REM Run this once to install dependencies

echo ============================================
echo   PTSL MCP Server - Setup
echo ============================================
echo.

REM Check Python
python --version >nul 2>&1
if errorlevel 1 (
    echo ERROR: Python not found. Install Python 3.10+ from python.org
    echo Make sure to check "Add Python to PATH" during install.
    pause
    exit /b 1
)

echo Creating virtual environment...
python -m venv venv

echo Activating virtual environment...
call venv\Scripts\activate.bat

echo Installing dependencies...
pip install grpcio grpcio-tools protobuf mcp

echo.
echo ============================================
echo   Setup complete!
echo.
echo   Next steps:
echo   1. Make sure Pro Tools is running
echo   2. Test the server:  venv\Scripts\python ptsl_mcp_server.py
echo      (It will appear to hang - that's normal, it's waiting
echo       for the AI to connect via stdin)
echo   3. Ctrl+C to stop the test
echo   4. Add to Claude Desktop config (see README.md)
echo ============================================
pause
