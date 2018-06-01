add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/perf.pb.cc
         ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/perf.pb.h
         ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/perf.grpc.pb.cc
         ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/perf.grpc.pb.h
  DEPENDS protoc perf.proto
  COMMAND protoc perf.proto
      --cpp_out=${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}
      --grpc_out=${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}
      --plugin=protoc-gen-grpc=$<TARGET_FILE:grpc_cpp_plugin>
      -I${CMAKE_CURRENT_SOURCE_DIR}
      -I${PROJECT_SOURCE_DIR}/thirdparty/grpc/third_party/protobuf/src
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

add_bond_test (perf perf.bond
    main.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/perf.pb.cc
    ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/perf.grpc.pb.cc
    GRPC)

cxx_target_compile_definitions (MSVC perf PRIVATE -D_WIN32_WINNT=0x0600)
cxx_target_compile_options (MSVC perf PRIVATE /wd4100 /wd4146 /wd4800)

target_link_libraries(perf PRIVATE grpc++ ${Boost_PROGRAM_OPTIONS_LIBRARY})
