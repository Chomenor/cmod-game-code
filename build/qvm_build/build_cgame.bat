@echo off
if not "%1" == "script" if exist output echo WARNING: Output already exists. To ensure a clean build delete this directory before starting build.&echo.

rem Verify tools exist
if not exist q3lcc.exe echo Missing q3lcc.exe&pause&exit
if not exist q3asm.exe echo Missing q3asm.exe&pause&exit

rem Set locations
set code_directory=..\..\code
set output_directory=output
set vm_directory=%output_directory%\vm
set output_file=%vm_directory%\cgame
set build_temp=%output_directory%\temp
set module_temp=%build_temp%\cgame
set q3asm_file=%module_temp%\cgame.q3asm

rem Create missing directories
if not exist %output_directory% mkdir %output_directory%
if not exist %vm_directory% mkdir %vm_directory%
if not exist %build_temp% mkdir %build_temp%
if not exist %module_temp% mkdir %module_temp%

echo Creating the CGAME ASM files...
if exist %q3asm_file% del %q3asm_file%
set src=cgame&set name=cg_main&call :compile
>>%q3asm_file% echo %code_directory%\cgame\cg_syscalls
set src=cgame&set name=cg_consolecmds&call :compile
set src=cgame&set name=cg_draw&call :compile
set src=cgame&set name=cg_drawtools&call :compile
set src=cgame&set name=cg_effects&call :compile
set src=cgame&set name=cg_ents&call :compile
set src=cgame&set name=cg_env&call :compile
set src=cgame&set name=cg_event&call :compile
set src=cgame&set name=cg_info&call :compile
set src=cgame&set name=cg_localents&call :compile
set src=cgame&set name=cg_marks&call :compile
set src=cgame&set name=cg_players&call :compile
set src=cgame&set name=cg_playerstate&call :compile
set src=cgame&set name=cg_predict&call :compile
set src=cgame&set name=cg_scoreboard&call :compile
set src=cgame&set name=cg_screenfx&call :compile
set src=cgame&set name=cg_servercmds&call :compile
set src=cgame&set name=cg_snapshot&call :compile
set src=cgame&set name=cg_view&call :compile
set src=cgame&set name=cg_weapons&call :compile
set src=common&set name=bg_slidemove&call :compile
set src=common&set name=bg_pmove&call :compile
set src=common&set name=bg_lib&call :compile
set src=common&set name=bg_misc&call :compile
set src=common&set name=q_math&call :compile
set src=common&set name=q_shared&call :compile
set src=cgame&set name=fx_borg&call :compile
set src=cgame&set name=fx_compression&call :compile
set src=cgame&set name=fx_dreadnought&call :compile
set src=cgame&set name=fx_grenade&call :compile
set src=cgame&set name=fx_imod&call :compile
set src=cgame&set name=fx_item&call :compile
set src=cgame&set name=fx_lib&call :compile
set src=cgame&set name=fx_misc&call :compile
set src=cgame&set name=fx_phaser&call :compile
set src=cgame&set name=fx_quantum&call :compile
set src=cgame&set name=fx_scavenger&call :compile
set src=cgame&set name=fx_stasis&call :compile
set src=cgame&set name=fx_tetrion&call :compile
set src=cgame&set name=fx_transporter&call :compile
set src=common&set name=vm_extensions&call :compile

echo Creating the CGAME.QVM file...
q3asm.exe -o %output_file% -vq3 -f %q3asm_file%
if errorlevel 1 echo Error creating CGAME.QVM&pause&exit

echo CGAME.QVM compiled successfully.
echo.
if not "%1" == "script" pause
goto :eof

:compile
echo %src%\%name%.c
q3lcc.exe -o %module_temp%\%name%.asm -DMODULE_CGAME -I%code_directory%\cgame -I%code_directory%\common %code_directory%\%src%\%name%.c
if errorlevel 1 echo Error compiling %src%\%name%.c&pause&exit
>>%q3asm_file% echo %module_temp%\%name%
goto :eof