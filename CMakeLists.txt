cmake_minimum_required(VERSION 3.13)
project(Exchange)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
# 添加子目录
add_subdirectory(Sync)
#add_subdirectory(Async)

set(PROTOS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/protos)

set(PROTO_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})

file(GLOB_RECURSE PROTOS_SRC_LIST ${PROTOS_DIR}/*.proto)

foreach(__proto_file ${PROTOS_SRC_LIST})
    message(${__proto_file})
    #protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTO_SRC_DIR} ${__proto_file})
    #grpc_generate_cpp(GRPC_SRCS GRPC_HDRS ${PROTO_SRC_DIR} ${PROTOS})
    execute_process(COMMAND protoc -I=${PROTOS_DIR}
            --grpc_out=${PROTO_SRC_DIR}
            --cpp_out=${PROTO_SRC_DIR}
            --plugin=protoc-gen-grpc=/usr/local/bin/grpc_cpp_plugin
            ${__proto_file}
            WORKING_DIRECTORY ${PROTO_SRC_DIR})
endforeach(__proto_file)