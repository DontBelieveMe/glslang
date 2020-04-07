set(SOURCES CppGen.cpp)
add_executable(CppGen ${SOURCES})
target_link_libraries(CppGen glslang
    glslang-default-resource-limits)
target_compile_features(CppGen PRIVATE cxx_std_17)

