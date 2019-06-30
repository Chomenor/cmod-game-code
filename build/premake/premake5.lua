-- premake5.lua
workspace "StefGame"
   configurations { "Debug", "Release" }

project "game"
   kind "SharedLib"
   language "C"
   targetdir "bin/%{cfg.buildcfg}"
   targetname ("qagamex86")

   files { "../../code/game/*.h",
      "../../code/game/*.c",
      "../../code/common/*.h",
      "../../code/common/*.c" }

   removefiles { "../../code/common/bg_lib.*" }

   includedirs{"../../code/game",
      "../../code/common"}

   defines { "_CRT_SECURE_NO_WARNINGS" }

   filter "configurations:Debug"
      defines { "DEBUG" }
      symbols "On"

   filter "configurations:Release"
      defines { "NDEBUG" }
      optimize "On"

project "cgame"
   kind "SharedLib"
   language "C"
   targetdir "bin/%{cfg.buildcfg}"
   targetname ("cgamex86")

   files { "../../code/cgame/*.h",
      "../../code/cgame/*.c",
      "../../code/common/*.h",
      "../../code/common/*.c" }

   removefiles { "../../code/common/bg_lib.*" }

   includedirs{"../../code/cgame",
      "../../code/common"}

   defines { "_CRT_SECURE_NO_WARNINGS" }

   filter "configurations:Debug"
      defines { "DEBUG" }
      symbols "On"

   filter "configurations:Release"
      defines { "NDEBUG" }
      optimize "On"

project "ui"
   kind "SharedLib"
   language "C"
   targetdir "bin/%{cfg.buildcfg}"
   targetname ("uix86")

   files { "../../code/ui/*.h",
      "../../code/ui/*.c",
      "../../code/common/bg_misc.c",
      "../../code/common/q_math.c",
      "../../code/common/q_shared.c",
      "../../code/common/bg_public.h",
      "../../code/common/tr_types.h" }

   includedirs{"../../code/cgame",
      "../../code/common"}

   defines { "_CRT_SECURE_NO_WARNINGS" }

   filter "configurations:Debug"
      defines { "DEBUG" }
      symbols "On"

   filter "configurations:Release"
      defines { "NDEBUG" }
      optimize "On"
