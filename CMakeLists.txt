# =============================================================================
#  QualEngine - Kok CMake yapilandirmasi
# -----------------------------------------------------------------------------
#  Hedefler:
#    Engine       (STATIC)  - Motor cekirdegi + Vesta editor + ucuncu parti kod
#    Application  (EXE)     - Vesta editor uygulamasi
#    GameRuntime  (EXE)     - Export edilen oyunlarin calistirdigi player
#
#  Hizli baslangic:
#    cmake --preset vs2026
#    cmake --build --preset debug
#
#  Cikti dizini:
#    <repo>/x64/<Config>/   (Engine.lib, Application.exe, GameRuntime.exe, *.pdb)
#  Bu yerlesim bilincli olarak korunur; hot-reload script sistemi
#  (Engine/Project/project_module.cpp) ve exporter bu dizinleri kullanir.
# =============================================================================

cmake_minimum_required(VERSION 3.25)

project(QualEngine
    VERSION 0.1.0
    DESCRIPTION "Modern C++20 game engine and Vesta editor"
    HOMEPAGE_URL "https://github.com/yusufkerman/qual_engine"
    LANGUAGES C CXX)

if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
    message(FATAL_ERROR "In-source build desteklenmez. 'cmake --preset vs2026' kullanin.")
endif()

if(NOT WIN32 OR NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "QualEngine su an yalnizca Windows x64 hedefini destekler.")
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# ------------------------------------------------------------- Konfigurasyon -
get_property(QUAL_MULTI_CONFIG GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(QUAL_MULTI_CONFIG)
    set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "" FORCE)
elseif(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build tipi (Debug/Release)" FORCE)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Dinamik CRT (/MD, /MDd) - GameModule.dll ile ayni CRT'yi paylasmak zorunlu.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")

option(QUAL_ENABLE_LTO "Release build'de Link Time Optimization (/GL + /LTCG) etkinlestir" OFF)

# ------------------------------------------------------------- Cikti dizini --
set(QUAL_OUTPUT_ROOT "${PROJECT_SOURCE_DIR}/x64")
foreach(cfg IN ITEMS Debug Release)
    string(TOUPPER "${cfg}" CFG)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CFG}     "${QUAL_OUTPUT_ROOT}/${cfg}")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${CFG}     "${QUAL_OUTPUT_ROOT}/${cfg}")
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${CFG}     "${QUAL_OUTPUT_ROOT}/${cfg}")
    set(CMAKE_PDB_OUTPUT_DIRECTORY_${CFG}         "${QUAL_OUTPUT_ROOT}/${cfg}")
    set(CMAKE_COMPILE_PDB_OUTPUT_DIRECTORY_${CFG} "${QUAL_OUTPUT_ROOT}/${cfg}")
endforeach()
if(NOT QUAL_MULTI_CONFIG)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${QUAL_OUTPUT_ROOT}/${CMAKE_BUILD_TYPE}")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${QUAL_OUTPUT_ROOT}/${CMAKE_BUILD_TYPE}")
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${QUAL_OUTPUT_ROOT}/${CMAKE_BUILD_TYPE}")
endif()

# ------------------------------------------------------------- Moduller ------
include(QualCompilerOptions)
include(QualDependencies)

if(QUAL_ENABLE_LTO)
    include(CheckIPOSupported)
    check_ipo_supported(RESULT _qual_ipo_ok OUTPUT _qual_ipo_msg)
    if(_qual_ipo_ok)
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
    else()
        message(WARNING "[QualEngine] LTO desteklenmiyor: ${_qual_ipo_msg}")
    endif()
endif()

# Calisma zamaninda kullanilan build bilgileri (hot-reload GameModule
# derleyicisi bagimlilik yollarini buradan okur).
configure_file(
    "${PROJECT_SOURCE_DIR}/cmake/qual_build_config.h.in"
    "${PROJECT_BINARY_DIR}/generated/qual_build_config.h"
    @ONLY)

add_subdirectory(Engine)
add_subdirectory(Application)
add_subdirectory(GameRuntime)

set_property(DIRECTORY "${PROJECT_SOURCE_DIR}" PROPERTY VS_STARTUP_PROJECT Application)

message(STATUS "[QualEngine] Cikti dizini: ${QUAL_OUTPUT_ROOT}/<Config>")

