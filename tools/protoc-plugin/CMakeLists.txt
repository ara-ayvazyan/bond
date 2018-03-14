add_executable (protoc-plugin-bond-idl
    main.cpp)

target_include_directories (protoc-plugin-bond-idl PRIVATE
    "${CMAKE_SOURCE_DIR}/thirdparty/protobuf/src")

target_link_libraries(protoc-plugin-bond-idl libprotoc)
