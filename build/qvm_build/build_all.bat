@echo off
if exist output echo WARNING: Output already exists. To ensure a clean build delete this directory before starting build.&echo.

call build_game.bat script
call build_cgame.bat script
call build_ui.bat script

pause