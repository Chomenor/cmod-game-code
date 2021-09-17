@echo off
if not "%1" == "script" if exist output echo WARNING: Output already exists. To ensure a clean build delete this directory before starting build.&echo.

rem Verify tools exist
if not exist q3lcc.exe echo Missing q3lcc.exe&pause&exit
if not exist q3asm.exe echo Missing q3asm.exe&pause&exit

rem Set locations
set code_directory=..\..\code
set output_directory=output
set vm_directory=%output_directory%\vm
set output_file=%vm_directory%\ui
set build_temp=%output_directory%\temp
set module_temp=%build_temp%\ui
set q3asm_file=%module_temp%\ui.q3asm

rem Create missing directories
if not exist %output_directory% mkdir %output_directory%
if not exist %vm_directory% mkdir %vm_directory%
if not exist %build_temp% mkdir %build_temp%
if not exist %module_temp% mkdir %module_temp%

echo Creating the UI ASM files...
if exist %q3asm_file% del %q3asm_file%
set src=ui&set name=ui_main&call :compile
>>%q3asm_file% echo %code_directory%\ui\ui_syscalls
set src=ui&set name=ui_addbots&call :compile
set src=ui&set name=ui_atoms&call :compile
set src=ui&set name=ui_cdkey&call :compile
set src=ui&set name=ui_confirm&call :compile
set src=ui&set name=ui_connect&call :compile
set src=ui&set name=ui_controls2&call :compile
set src=ui&set name=ui_cvars&call :compile
set src=ui&set name=ui_demo2&call :compile
set src=ui&set name=ui_fonts&call :compile
set src=ui&set name=ui_gameinfo&call :compile
set src=ui&set name=ui_ignores&call :compile
set src=ui&set name=ui_ingame&call :compile
set src=ui&set name=ui_menu&call :compile
set src=ui&set name=ui_mfield&call :compile
set src=ui&set name=ui_mods&call :compile
set src=ui&set name=ui_network&call :compile
set src=ui&set name=ui_playermodel&call :compile
set src=ui&set name=ui_players&call :compile
set src=ui&set name=ui_playersettings&call :compile
set src=ui&set name=ui_preferences&call :compile
set src=ui&set name=ui_qmenu&call :compile
set src=ui&set name=ui_removebots&call :compile
set src=ui&set name=ui_serverinfo&call :compile
set src=ui&set name=ui_servers2&call :compile
set src=ui&set name=ui_sound&call :compile
set src=ui&set name=ui_sparena&call :compile
set src=ui&set name=ui_specifyserver&call :compile
set src=ui&set name=ui_splevel&call :compile
set src=ui&set name=ui_sppostgame&call :compile
set src=ui&set name=ui_spskill&call :compile
set src=ui&set name=ui_startserver&call :compile
set src=ui&set name=ui_team&call :compile
set src=ui&set name=ui_teamorders&call :compile
set src=ui&set name=ui_video&call :compile
set src=common&set name=bg_misc&call :compile
set src=common&set name=bg_lib&call :compile
set src=common&set name=q_math&call :compile
set src=common&set name=q_shared&call :compile
set src=common&set name=vm_extensions&call :compile

echo Creating the UI.QVM file...
q3asm.exe -o %output_file% -vq3 -f %q3asm_file%
if errorlevel 1 echo Error creating UI.QVM&pause&exit

echo UI.QVM compiled successfully.
echo.
if not "%1" == "script" pause
goto :eof

:compile
echo %src%\%name%.c
q3lcc.exe -o %module_temp%\%name%.asm -DMODULE_UI -I%code_directory%\ui -I%code_directory%\common %code_directory%\%src%\%name%.c
if errorlevel 1 echo Error compiling %src%\%name%.c&pause&exit
>>%q3asm_file% echo %module_temp%\%name%
goto :eof