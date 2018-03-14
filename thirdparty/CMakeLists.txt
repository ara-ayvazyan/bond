include (Compiler)

if (BOND_ENABLE_GRPC)
    cxx_add_compile_options (Clang -Wno-unused-value)
    add_subdirectory(grpc)
endif()

option(protobuf_BUILD_TESTS OFF)

    if (MSVC)
        option(protobuf_MSVC_STATIC_RUNTIME OFF)
    endif()

add_subdirectory(protobuf/cmake)
