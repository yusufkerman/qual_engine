# =============================================================================
#  Engine - QualEngine cekirdek statik kutuphanesi
# -----------------------------------------------------------------------------
#  Engine.lib; motor, Vesta editor ve gomulu ucuncu parti kodu (ImGui, Jolt,
#  NFD, stb) tek bir arsivde toplar. Hot-reload ile derlenen GameModule.dll
#  yalnizca Engine.lib'e link ettigi icin bu "tek arsiv" yapisi zorunludur.
#
#  Kaynak dosyalari otomatik toplanir (CONFIGURE_DEPENDS): yeni bir .cpp/.h
#  eklediginizde CMakeLists'e dokunmaniza gerek yoktur; bir sonraki build
#  CMake'i otomatik olarak yeniden yapilandirir.
# =============================================================================

set(QUAL_EXTERNALS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Externals")

# ---------------------------------------------------------------- ImGui ------
add_library(qual_imgui OBJECT
    "${QUAL_EXTERNALS_DIR}/imgui/imgui.cpp"
    "${QUAL_EXTERNALS_DIR}/imgui/imgui_draw.cpp"
    "${QUAL_EXTERNALS_DIR}/imgui/imgui_tables.cpp"
    "${QUAL_EXTERNALS_DIR}/imgui/imgui_widgets.cpp"
    "${QUAL_EXTERNALS_DIR}/imgui/backends/imgui_impl_glfw.cpp"
    "${QUAL_EXTERNALS_DIR}/imgui/backends/imgui_impl_opengl3.cpp"
)
target_include_directories(qual_imgui PUBLIC
    "${QUAL_EXTERNALS_DIR}/imgui"
    "${QUAL_EXTERNALS_DIR}/imgui/backends"
)
target_include_directories(qual_imgui PRIVATE "${QUAL_GLFW_INCLUDE_DIR}")
target_link_libraries(qual_imgui PRIVATE qual_thirdparty_options)

# ---------------------------------------------------------------- Jolt -------
file(GLOB_RECURSE QUAL_JOLT_SOURCES CONFIGURE_DEPENDS "${QUAL_EXTERNALS_DIR}/Jolt/*.cpp")
add_library(qual_jolt OBJECT ${QUAL_JOLT_SOURCES})
target_include_directories(qual_jolt PUBLIC "${QUAL_EXTERNALS_DIR}")
target_link_libraries(qual_jolt PRIVATE qual_thirdparty_options)

# ---------------------------------------------------------------- Diger ------
add_library(qual_externals_misc OBJECT
    "${QUAL_EXTERNALS_DIR}/nfd/nfd_win.cpp"
    "${QUAL_EXTERNALS_DIR}/stb/stb_image.cpp"
)
target_include_directories(qual_externals_misc PUBLIC
    "${QUAL_EXTERNALS_DIR}"
    "${QUAL_EXTERNALS_DIR}/nfd"
)
target_link_libraries(qual_externals_misc PRIVATE qual_thirdparty_options)

set_target_properties(qual_imgui qual_jolt qual_externals_misc PROPERTIES FOLDER "ThirdParty")

# ---------------------------------------------------------------- Engine -----
file(GLOB_RECURSE QUAL_ENGINE_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
)
# Ucuncu parti kod ayri OBJECT hedeflerinde derlenir; eski MSBuild ara
# ciktilari (x64/) da kaynak listesine girmemeli.
list(FILTER QUAL_ENGINE_SOURCES EXCLUDE REGEX "/Externals/")
list(FILTER QUAL_ENGINE_SOURCES EXCLUDE REGEX "/x64/")

file(GLOB_RECURSE QUAL_ENGINE_SHADERS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/Resources/*.vert"
    "${CMAKE_CURRENT_SOURCE_DIR}/Resources/*.frag"
    "${CMAKE_CURRENT_SOURCE_DIR}/Resources/*.glsl"
    "${CMAKE_CURRENT_SOURCE_DIR}/Shaders/*.vert"
    "${CMAKE_CURRENT_SOURCE_DIR}/Shaders/*.frag"
    "${CMAKE_CURRENT_SOURCE_DIR}/Shaders/*.glsl"
)
set_source_files_properties(${QUAL_ENGINE_SHADERS} PROPERTIES HEADER_FILE_ONLY ON)

add_library(Engine STATIC
    ${QUAL_ENGINE_SOURCES}
    ${QUAL_ENGINE_SHADERS}
    $<TARGET_OBJECTS:qual_imgui>
    $<TARGET_OBJECTS:qual_jolt>
    $<TARGET_OBJECTS:qual_externals_misc>
)
add_library(qual::engine ALIAS Engine)

# IDE'de (Visual Studio / VS Code) klasor yapisini koru.
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${QUAL_ENGINE_SOURCES} ${QUAL_ENGINE_SHADERS})

target_include_directories(Engine
    PUBLIC
        "${QUAL_EXTERNALS_DIR}"
        "${QUAL_EXTERNALS_DIR}/nfd"
        "${QUAL_EXTERNALS_DIR}/imgui"
        "${QUAL_EXTERNALS_DIR}/imgui/backends"
        "${CMAKE_CURRENT_SOURCE_DIR}"
    PRIVATE
        "${PROJECT_BINARY_DIR}/generated"
)

target_link_libraries(Engine
    PUBLIC
        qual::glm
        qual::glfw
        qual::assimp
        OpenGL::GL
    PRIVATE
        qual::build_options
        dbghelp
)

# GLAD header'lari Engine'i kullanan herkese acik olmali; glad.c ise her
# executable'a ayri derlenir (bkz. cmake/QualDependencies.cmake).
target_include_directories(Engine PUBLIC "${QUAL_GLAD_INCLUDE_DIR}")

set_target_properties(Engine PROPERTIES
    OUTPUT_NAME "Engine"
    COMPILE_PDB_NAME "Engine"
    FOLDER "QualEngine"
)
