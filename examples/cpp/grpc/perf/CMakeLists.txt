add_bond_test (perf perf.bond main.cpp GRPC)

cxx_target_compile_definitions (MSVC perf PRIVATE -D_WIN32_WINNT=0x0600)

target_link_libraries(perf PRIVATE grpc++ ${Boost_PROGRAM_OPTIONS_LIBRARY})
