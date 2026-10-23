# =============================================================================
#  QualEngine - Ortak derleyici ayarlari
# -----------------------------------------------------------------------------
#  Eski .vcxproj dosyalarindaki ayarlarin CMake karsiligi:
#    WarningLevel=Level3       -> /W3
#    SDLCheck=true             -> /sdl
#    ConformanceMode=true      -> /permissive-
#    /utf-8                    -> kaynak ve calisma zamani karakter seti UTF-8
#    DisableSpecificWarnings   -> /wd4996 (CRT "unsafe" uyarilari)
#    MultiProcessorCompilation -> /MP (yalnizca Visual Studio generator)
#    CharacterSet=Unicode      -> UNICODE;_UNICODE
#  Release: FunctionLevelLinking (/Gy), IntrinsicFunctions (/Oi),
#           EnableCOMDATFolding (/OPT:ICF), OptimizeReferences (/OPT:REF)
# =============================================================================

include_guard(GLOBAL)

add_library(qual_build_options INTERFACE)
add_library(qual::build_options ALIAS qual_build_options)

target_compile_features(qual_build_options INTERFACE cxx_std_20)

if(MSVC)
    target_compile_options(qual_build_options INTERFACE
        /W3
        /sdl
        /permissive-
        /utf-8
        /wd4996
        $<$<CONFIG:Release>:/Gy /Oi>
    )

    if(CMAKE_GENERATOR MATCHES "Visual Studio")
        target_compile_options(qual_build_options INTERFACE /MP)
    endif()

    target_compile_definitions(qual_build_options INTERFACE
        UNICODE
        _UNICODE
        $<$<CONFIG:Debug>:_DEBUG>
        $<$<CONFIG:Release>:NDEBUG>
    )

    target_link_options(qual_build_options INTERFACE
        $<$<CONFIG:Release>:/OPT:REF /OPT:ICF>
        # Release'te de PDB uretilir (vcxproj: GenerateDebugInformation=true)
        $<$<CONFIG:Release>:/DEBUG>
        # glfw3.lib /MD ile derlenmistir; Debug'da /MDd ile karisinca cikan
        # zararsiz LNK4098 uyarisini bastir (eski build'de de mevcuttu).
        $<$<CONFIG:Debug>:/IGNORE:4098>
    )
else()
    message(WARNING "[QualEngine] Su an yalnizca MSVC (Windows x64) resmi olarak desteklenmektedir.")
endif()

# Ucuncu parti kodlar icin uyari seviyesi dusurulur; kendi kodumuzun
# uyari gurultusunu kirletmesinler.
add_library(qual_thirdparty_options INTERFACE)
target_compile_features(qual_thirdparty_options INTERFACE cxx_std_20)
if(MSVC)
    target_compile_options(qual_thirdparty_options INTERFACE
        /W0
        /utf-8
        /permissive-
        /wd4996
        $<$<CONFIG:Release>:/Gy /Oi>
    )
    if(CMAKE_GENERATOR MATCHES "Visual Studio")
        target_compile_options(qual_thirdparty_options INTERFACE /MP)
    endif()
    target_compile_definitions(qual_thirdparty_options INTERFACE
        UNICODE
        _UNICODE
        $<$<CONFIG:Debug>:_DEBUG>
        $<$<CONFIG:Release>:NDEBUG>
    )
endif()
