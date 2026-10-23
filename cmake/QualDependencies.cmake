# =============================================================================
#  QualEngine - Harici bagimliliklar (prebuilt)
# -----------------------------------------------------------------------------
#  GLFW, GLAD, GLM ve Assimp sisteme onceden kurulmus olarak beklenir.
#  Varsayilan yollar asagidaki cache degiskenleriyle tanimlidir; farkli bir
#  makinede su yontemlerden biriyle ezilebilir:
#
#    1) Ortam degiskeni :  set QUAL_GLFW_ROOT=D:/libs/glfw
#    2) Komut satiri    :  cmake --preset vs2026 -DQUAL_GLFW_ROOT=D:/libs/glfw
#    3) Kullanici preset:  CMakeUserPresets.json icinde "cacheVariables"
#
#  Bu dosya su hedefleri olusturur:
#    qual::glm     (INTERFACE)        - header-only matematik kutuphanesi
#    qual::glfw    (STATIC IMPORTED)  - pencere / input
#    qual::assimp  (SHARED IMPORTED)  - model import (DLL + import lib)
#    qual_glad     (OBJECT)           - OpenGL loader (her executable'a gomulur)
# =============================================================================

include_guard(GLOBAL)

# Ortam degiskeni varsa onu, yoksa verilen varsayilani kullanan yardimci.
function(qual_dependency_root VAR DEFAULT DOC)
    if(DEFINED ENV{${VAR}} AND NOT "$ENV{${VAR}}" STREQUAL "")
        set(_default "$ENV{${VAR}}")
    else()
        set(_default "${DEFAULT}")
    endif()
    set(${VAR} "${_default}" CACHE PATH "${DOC}")
endfunction()

qual_dependency_root(QUAL_GLFW_ROOT         "C:/Libraries/glfw"                 "GLFW kok dizini (include/ ve lib-vc2022/ iceren)")
qual_dependency_root(QUAL_GLAD_ROOT         "C:/Libraries/glad"                 "GLAD kok dizini (include/ ve src/glad.c iceren)")
qual_dependency_root(QUAL_GLM_ROOT          "C:/Libraries/glm"                  "GLM kok dizini (glm/glm.hpp iceren)")
qual_dependency_root(QUAL_ASSIMP_ROOT       "C:/dev/assimp-6.0.5"               "Assimp kaynak kok dizini (include/ iceren)")
qual_dependency_root(QUAL_ASSIMP_BUILD_DIR  "C:/dev/assimp-6.0.5/assimp_build"  "Assimp build dizini (include/, lib/, bin/ iceren)")

set(QUAL_GLFW_LIB_SUBDIR "lib-vc2022" CACHE STRING "GLFW kutuphane alt dizini")
set(QUAL_ASSIMP_LIB_NAME "assimp-vc145-mt" CACHE STRING "Assimp kutuphane adi (uzantisiz)")
set(QUAL_ASSIMP_CONFIG   "Release" CACHE STRING "Kullanilacak Assimp build konfigurasyonu (lib/<cfg>, bin/<cfg>)")

# Bulunamayan bagimlilik icin anlasilir hata mesaji.
function(qual_require_path VAR NAME HINT)
    if(NOT ${VAR} OR NOT EXISTS "${${VAR}}")
        message(FATAL_ERROR
            "\n[QualEngine] ${NAME} bulunamadi.\n"
            "  Beklenen : ${HINT}\n"
            "  Cozum    : -D${ARGV3}=<yol> ile veya ${ARGV3} ortam degiskeniyle dogru dizini belirtin.\n")
    endif()
endfunction()

# ---------------------------------------------------------------- GLM --------
find_path(QUAL_GLM_INCLUDE_DIR NAMES glm/glm.hpp HINTS "${QUAL_GLM_ROOT}" NO_DEFAULT_PATH)
qual_require_path(QUAL_GLM_INCLUDE_DIR "GLM" "${QUAL_GLM_ROOT}/glm/glm.hpp" QUAL_GLM_ROOT)

add_library(qual_glm INTERFACE)
add_library(qual::glm ALIAS qual_glm)
target_include_directories(qual_glm SYSTEM INTERFACE "${QUAL_GLM_INCLUDE_DIR}")

# ---------------------------------------------------------------- GLFW -------
find_path(QUAL_GLFW_INCLUDE_DIR NAMES GLFW/glfw3.h HINTS "${QUAL_GLFW_ROOT}/include" NO_DEFAULT_PATH)
find_library(QUAL_GLFW_LIBRARY NAMES glfw3 HINTS "${QUAL_GLFW_ROOT}/${QUAL_GLFW_LIB_SUBDIR}" NO_DEFAULT_PATH)
qual_require_path(QUAL_GLFW_INCLUDE_DIR "GLFW header" "${QUAL_GLFW_ROOT}/include/GLFW/glfw3.h" QUAL_GLFW_ROOT)
qual_require_path(QUAL_GLFW_LIBRARY     "GLFW library" "${QUAL_GLFW_ROOT}/${QUAL_GLFW_LIB_SUBDIR}/glfw3.lib" QUAL_GLFW_ROOT)
get_filename_component(QUAL_GLFW_LIB_DIR "${QUAL_GLFW_LIBRARY}" DIRECTORY)

add_library(qual::glfw STATIC IMPORTED GLOBAL)
set_target_properties(qual::glfw PROPERTIES
    IMPORTED_LOCATION "${QUAL_GLFW_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${QUAL_GLFW_INCLUDE_DIR}"
    INTERFACE_COMPILE_DEFINITIONS "GLFW_INCLUDE_NONE")

# ---------------------------------------------------------------- GLAD -------
find_path(QUAL_GLAD_INCLUDE_DIR NAMES glad/glad.h HINTS "${QUAL_GLAD_ROOT}/include" NO_DEFAULT_PATH)
find_file(QUAL_GLAD_SOURCE NAMES glad.c HINTS "${QUAL_GLAD_ROOT}/src" NO_DEFAULT_PATH)
qual_require_path(QUAL_GLAD_INCLUDE_DIR "GLAD header" "${QUAL_GLAD_ROOT}/include/glad/glad.h" QUAL_GLAD_ROOT)
qual_require_path(QUAL_GLAD_SOURCE      "GLAD source" "${QUAL_GLAD_ROOT}/src/glad.c" QUAL_GLAD_ROOT)

# glad.c bilerek Engine.lib icine konmaz: Editor, GameRuntime ve hot-reload
# GameModule.dll her biri kendi GL fonksiyon tablosunu tasir (onceki vcxproj
# davranisiyla birebir ayni).
add_library(qual_glad OBJECT "${QUAL_GLAD_SOURCE}")
target_include_directories(qual_glad PUBLIC "${QUAL_GLAD_INCLUDE_DIR}")
set_target_properties(qual_glad PROPERTIES FOLDER "ThirdParty")

# ---------------------------------------------------------------- Assimp -----
find_path(QUAL_ASSIMP_INCLUDE_DIR NAMES assimp/Importer.hpp HINTS "${QUAL_ASSIMP_ROOT}/include" NO_DEFAULT_PATH)
find_path(QUAL_ASSIMP_CONFIG_INCLUDE_DIR NAMES assimp/config.h HINTS "${QUAL_ASSIMP_BUILD_DIR}/include" NO_DEFAULT_PATH)
find_library(QUAL_ASSIMP_LIBRARY NAMES ${QUAL_ASSIMP_LIB_NAME}
    HINTS "${QUAL_ASSIMP_BUILD_DIR}/lib/${QUAL_ASSIMP_CONFIG}" NO_DEFAULT_PATH)
find_file(QUAL_ASSIMP_DLL NAMES ${QUAL_ASSIMP_LIB_NAME}.dll
    HINTS "${QUAL_ASSIMP_BUILD_DIR}/bin/${QUAL_ASSIMP_CONFIG}" NO_DEFAULT_PATH)

qual_require_path(QUAL_ASSIMP_INCLUDE_DIR        "Assimp header"        "${QUAL_ASSIMP_ROOT}/include/assimp/Importer.hpp" QUAL_ASSIMP_ROOT)
qual_require_path(QUAL_ASSIMP_CONFIG_INCLUDE_DIR "Assimp config header" "${QUAL_ASSIMP_BUILD_DIR}/include/assimp/config.h" QUAL_ASSIMP_BUILD_DIR)
qual_require_path(QUAL_ASSIMP_LIBRARY            "Assimp import lib"    "${QUAL_ASSIMP_BUILD_DIR}/lib/${QUAL_ASSIMP_CONFIG}/${QUAL_ASSIMP_LIB_NAME}.lib" QUAL_ASSIMP_BUILD_DIR)
qual_require_path(QUAL_ASSIMP_DLL                "Assimp DLL"           "${QUAL_ASSIMP_BUILD_DIR}/bin/${QUAL_ASSIMP_CONFIG}/${QUAL_ASSIMP_LIB_NAME}.dll" QUAL_ASSIMP_BUILD_DIR)
get_filename_component(QUAL_ASSIMP_LIB_DIR "${QUAL_ASSIMP_LIBRARY}" DIRECTORY)

add_library(qual::assimp SHARED IMPORTED GLOBAL)
set_target_properties(qual::assimp PROPERTIES
    IMPORTED_IMPLIB   "${QUAL_ASSIMP_LIBRARY}"
    IMPORTED_LOCATION "${QUAL_ASSIMP_DLL}"
    INTERFACE_INCLUDE_DIRECTORIES "${QUAL_ASSIMP_INCLUDE_DIR};${QUAL_ASSIMP_CONFIG_INCLUDE_DIR}")

# ---------------------------------------------------------------- OpenGL -----
find_package(OpenGL REQUIRED)

# Executable'in yanina calisma zamani DLL'lerini (assimp vb.) kopyalar.
function(qual_copy_runtime_dlls TARGET)
    add_custom_command(TARGET ${TARGET} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_RUNTIME_DLLS:${TARGET}> $<TARGET_FILE_DIR:${TARGET}>
        COMMAND_EXPAND_LISTS
        COMMENT "[QualEngine] ${TARGET}: runtime DLL'leri kopyalaniyor")
endfunction()

message(STATUS "[QualEngine] GLM    : ${QUAL_GLM_INCLUDE_DIR}")
message(STATUS "[QualEngine] GLFW   : ${QUAL_GLFW_LIBRARY}")
message(STATUS "[QualEngine] GLAD   : ${QUAL_GLAD_SOURCE}")
message(STATUS "[QualEngine] Assimp : ${QUAL_ASSIMP_DLL}")
