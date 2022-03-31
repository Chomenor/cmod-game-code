@echo off
if not "%1" == "script" if exist output echo WARNING: Output already exists. To ensure a clean build delete this directory before starting build.&echo.

rem Verify tools exist
if not exist q3lcc.exe echo Missing q3lcc.exe&pause&exit
if not exist q3asm.exe echo Missing q3asm.exe&pause&exit

rem Set locations
set code_directory=..\..\code
set output_directory=output
set vm_directory=%output_directory%\vm
set output_file=%vm_directory%\qagame
set build_temp=%output_directory%\temp
set module_temp=%build_temp%\game
set q3asm_file=%module_temp%\game.q3asm

rem Create missing directories
if not exist %output_directory% mkdir %output_directory%
if not exist %vm_directory% mkdir %vm_directory%
if not exist %build_temp% mkdir %build_temp%
if not exist %module_temp% mkdir %module_temp%

echo Creating the GAME ASM files...
if exist %q3asm_file% del %q3asm_file%
set src=game&set name=g_main&call :compile
>>%q3asm_file% echo %code_directory%\game\g_syscalls
set src=common&set name=bg_misc&call :compile
set src=common&set name=bg_lib&call :compile
set src=common&set name=bg_pmove&call :compile
set src=common&set name=bg_slidemove&call :compile
set src=common&set name=q_math&call :compile
set src=common&set name=q_shared&call :compile
set src=game&set name=ai_chat&call :compile
set src=game&set name=ai_cmd&call :compile
set src=game&set name=ai_dmnet&call :compile
set src=game&set name=ai_dmq3&call :compile
set src=game&set name=ai_main&call :compile
set src=game&set name=ai_team&call :compile
set src=game&set name=g_active&call :compile
set src=game&set name=g_arenas&call :compile
set src=game&set name=g_bot&call :compile
set src=game&set name=g_breakable&call :compile
set src=game&set name=g_client&call :compile
set src=game&set name=g_cmds&call :compile
set src=game&set name=g_combat&call :compile
set src=game&set name=g_fx&call :compile
set src=game&set name=g_items&call :compile
set src=game&set name=g_log&call :compile
set src=game&set name=g_mem&call :compile
set src=game&set name=g_misc&call :compile
set src=game&set name=g_missile&call :compile
set src=game&set name=g_mover&call :compile
set src=game&set name=g_session&call :compile
set src=game&set name=g_spawn&call :compile
set src=game&set name=g_svcmds&call :compile
set src=game&set name=g_target&call :compile
set src=game&set name=g_team&call :compile
set src=game&set name=g_trigger&call :compile
set src=game&set name=g_turrets&call :compile
set src=game&set name=g_usable&call :compile
set src=game&set name=g_utils&call :compile
set src=game&set name=g_weapon&call :compile
set src=game\mods&set name=g_mod_main&call :compile
set src=game\mods&set name=g_mod_stubs&call :compile
set src=game\mods&set name=g_mod_utils&call :compile
set src=common&set name=logging&call :compile
set src=common&set name=vm_extensions&call :compile

echo Creating the QAGAME.QVM file...
q3asm.exe -o %output_file% -vq3 -f %q3asm_file%
if errorlevel 1 echo Error creating QAGAME.QVM&pause&exit

echo QAGAME.QVM compiled successfully.
echo.
if not "%1" == "script" pause
goto :eof

:compile
echo %src%\%name%.c
q3lcc.exe -o %module_temp%\%name%.asm -DMODULE_GAME -I%code_directory%\game -I%code_directory%\common %code_directory%\%src%\%name%.c
if errorlevel 1 echo Error compiling %src%\%name%.c&pause&exit
>>%q3asm_file% echo %module_temp%\%name%
goto :eof