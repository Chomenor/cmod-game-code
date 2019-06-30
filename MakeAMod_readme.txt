STEF Game Source. Copyright (C) 1999-2001 Raven Softare
Edited by Chomenor (2019-06-28)

The Game Source is broken out into 3 areas.

game - governs the game, runs on the server side.
cgame - governs the client side of the game, runs on the client side.
ui - handles the ui on the client side.

Making a quick mod is very straightforward. It covers making a slight mod to the game source, recompiling for debugging and rebuilding the vm's for distribution.

Slow Stasis Projectiles (Rockets) - TestMod
----------------------
1. Open the "g_local.h" file and change the GAMEVERSION define from "baseef" to "TestMod"
2. Save "g_local.h"
3. Open the "g_missile.c" file.
4. Go to line 557 and change the 900 to 300. The old line reads:
	
	VectorScale( dir, 900, bolt->s.pos.trDelta );

The new line should read

	VectorScale( dir, 300, bolt->s.pos.trDelta );

5. Save "g_missile.c"
6. Run the build\qvm_build\build_all.bat file and it should build the mod QVM files to the output\vm directory.

To run the mod in Elite Force:
1. Make a "TestMod" path under your STEF directory. This will be a sibling to 'baseef'
2. Copy the build\qvm_build\output\vm directory to TestMod, so you have a TestMod\vm directory.
    (note that you really only need to include the qagame.qvm file in the vm directory since this mod
     only changes the server module)
3. Run STEF with the following command line "stvoyhm +set fs_game TestMod"

For distribution of the mod, package the vm directory into a pk3 file (typically just a zip file with a pk3 extension).
Refer to online guides for working with pk3 files.

Using Visual Studio to Build and Debug your Mod:
Visual Studio can be used to compile native dlls for your mod which are suitable for debugging. To load dll files:
1. Load the included visual studio project files (e.g. in build\msvc_2019) and run the build
     If you need to use a different Visual Studio version you can try using the Premake script
     in build\premake to generate new project files.
2. Copy the output files (qagamex86.dll, cgamex86.dll, and uix86.dll) directly to the TestMod directory
     from the previous example (NOT under the vm directory)
3. Run STEF with the following command line "stvoyhm +set vm_game 0 +set vm_cgame 0 +set vm_ui 0 +set fs_game TestMod"
