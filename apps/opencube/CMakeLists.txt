add_executable(opencube main.cpp)
target_link_libraries(opencube PRIVATE ocplatform ocrhi ocnet ocbase)

# Keep SDL3.dll next to the executable so it runs straight from the build tree.
set(OPENSDL3_DLL "${PROJECT_SOURCE_DIR}/third_party/SDL3/lib/x64/SDL3.dll")
if(EXISTS "${OPENSDL3_DLL}")
    add_custom_command(TARGET opencube POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${OPENSDL3_DLL}" "$<TARGET_FILE_DIR:opencube>"
        VERBATIM)
endif()
