-- premake5.lua

workspace "StefGame"
   configurations { "Debug", "Release" }
   platforms { "Win64", "Win32" }

   kind "SharedLib"
   language "C"
   targetdir "bin/%{cfg.buildcfg}"

   defines { "_CRT_SECURE_NO_WARNINGS" }

   filter "platforms:Win64"
      architecture "x64"

   filter "configurations:Debug"
      defines { "DEBUG" }
      symbols "On"

   filter "configurations:Release"
      defines { "NDEBUG" }
      optimize "On"

project "game"
   files { "../../code/game/**.h",
      "../../code/game/**.c",
      "../../code/common/**.h",
      "../../code/common/**.c" }
   removefiles { "../../code/common/bg_lib.*",
      "../../code/common/aspect_correct.*" }

   defines { "MODULE_GAME" }
   includedirs { "../../code/game",
      "../../code/common" }

   targetname "qagamex86"
   filter "platforms:Win64"
      targetname "qagamex86_64"

project "cgame"
   files { "../../code/cgame/**.h",
      "../../code/cgame/**.c",
      "../../code/common/**.h",
      "../../code/common/**.c" }
   removefiles { "../../code/common/bg_lib.*" }

   defines { "MODULE_CGAME" }
   includedirs { "../../code/cgame",
      "../../code/common" }

   targetname "cgamex86"
   filter "platforms:Win64"
      targetname "cgamex86_64"

project "ui"
   files { "../../code/ui/**.h",
      "../../code/ui/**.c",
      "../../code/common/**.h",
      "../../code/common/**.c" }
   removefiles { "../../code/common/bg_*" }
   files { "../../code/common/bg_misc.c" }

   defines { "MODULE_UI" }
   includedirs { "../../code/ui",
      "../../code/common" }

   targetname "uix86"
   filter "platforms:Win64"
      targetname "uix86_64"
