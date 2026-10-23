# =============================================================================
#  GameRuntime - Export edilen oyunlarin standalone player'i
# -----------------------------------------------------------------------------
#  Vesta exporter (Engine/Project/project_exporter.cpp) bu executable'i
#  x64/<Config>/GameRuntime.exe konumunda arar ve <ProjeAdi>.exe olarak kopyalar.
# =============================================================================

add_executable(GameRuntime
    main.cpp
    $<TARGET_OBJECTS:qual_glad>
)

# <Engine/Runtime/...> seklindeki include'lar repo kokunu gerektirir.
target_include_directories(GameRuntime PRIVATE "${PROJECT_SOURCE_DIR}")
target_link_libraries(GameRuntime PRIVATE Engine qual::build_options)

set_target_properties(GameRuntime PROPERTIES FOLDER "QualEngine")

qual_copy_runtime_dlls(GameRuntime)

