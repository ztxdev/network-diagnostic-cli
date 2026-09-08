add_executable(ztxdiag_core_tests
    test_core.cpp
)

target_link_libraries(ztxdiag_core_tests PRIVATE ztxdiag::core)
target_compile_features(ztxdiag_core_tests PRIVATE cxx_std_20)

if(MSVC)
    target_compile_options(ztxdiag_core_tests PRIVATE /W4 /permissive- /EHsc)
else()
    target_compile_options(ztxdiag_core_tests PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_test(NAME ztxdiag_core_tests COMMAND ztxdiag_core_tests)
