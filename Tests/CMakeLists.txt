# =============================================================================
#  QualEngine - Otomatize Birim ve Entegrasyon Test Paketi (doctest)
# =============================================================================

set(QUAL_TEST_SOURCES
    main.cpp
    test_ecs.cpp
    test_transforms.cpp
    test_frustum.cpp
    test_logger.cpp
    test_crash_barrier.cpp
    test_undo_redo.cpp
    test_character_controller.cpp
    test_vfs.cpp
    test_csm.cpp
    test_script_preservation.cpp
    test_project_paths.cpp
)

add_executable(QualTests
    ${QUAL_TEST_SOURCES}
    $<TARGET_OBJECTS:qual_glad>
)

target_link_libraries(QualTests PRIVATE
    Engine
    qual::build_options
)

set_target_properties(QualTests PROPERTIES
    FOLDER "Tests"
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

qual_copy_runtime_dlls(QualTests)

add_test(NAME QualEngineTests COMMAND QualTests)
