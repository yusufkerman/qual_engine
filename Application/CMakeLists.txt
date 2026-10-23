# =============================================================================
#  Application - Vesta Editor
# =============================================================================

file(GLOB_RECURSE QUAL_APP_SHADERS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/Shaders/*.vert"
    "${CMAKE_CURRENT_SOURCE_DIR}/Shaders/*.frag"
    "${CMAKE_CURRENT_SOURCE_DIR}/Shaders/*.glsl"
)
set_source_files_properties(${QUAL_APP_SHADERS} PROPERTIES HEADER_FILE_ONLY ON)

add_executable(Application
    Application.cpp
    $<TARGET_OBJECTS:qual_glad>
    ${QUAL_APP_SHADERS}
)
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES Application.cpp ${QUAL_APP_SHADERS})

target_link_libraries(Application PRIVATE Engine qual::build_options)

set_target_properties(Application PROPERTIES
    FOLDER "QualEngine"
    # Editor, varsayilan proje olarak bu dizini kullanir (Assets/, Shaders/ ...)
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

qual_copy_runtime_dlls(Application)

