# Name of this driver project/program
set(CONTAM_CLIENT "ContamClient")
project(${CONTAM_CLIENT} LANGUAGES CXX)
add_library(${CONTAM_CLIENT} "${CONTAM_CLIENT}.cpp")

if(DEFINED ENV{gRPC_DIR})
  message(STATUS "gRPC_DIR: $ENV{gRPC_DIR}")
  list(APPEND CMAKE_PREFIX_PATH $ENV{gRPC_DIR})
endif()

include(grpc.cmake)

# Proto file
get_filename_component(contam_proto "proto/contam.proto" ABSOLUTE)
get_filename_component(contam_proto_path "${contam_proto}" PATH)

# Generated sources
set(contam_proto_srcs "${CMAKE_CURRENT_BINARY_DIR}/contam.pb.cc")
set(contam_proto_hdrs "${CMAKE_CURRENT_BINARY_DIR}/contam.pb.h")
set(contam_grpc_srcs "${CMAKE_CURRENT_BINARY_DIR}/contam.grpc.pb.cc")
set(contam_grpc_hdrs "${CMAKE_CURRENT_BINARY_DIR}/contam.grpc.pb.h")
add_custom_command(
      OUTPUT "${contam_proto_srcs}" "${contam_proto_hdrs}" "${contam_grpc_srcs}" "${contam_grpc_hdrs}"
      COMMAND ${_PROTOBUF_PROTOC}
      ARGS --grpc_out "${CMAKE_CURRENT_BINARY_DIR}"
        --cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
        -I "${contam_proto_path}"
        --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
        "${contam_proto}"
      DEPENDS "${contam_proto}")

# Include generated *.pb.h files
include_directories("${CMAKE_CURRENT_BINARY_DIR}")

# contam_grpc_proto
add_library(contam_grpc_proto
  ${contam_grpc_srcs}
  ${contam_grpc_hdrs}
  ${contam_proto_srcs}
  ${contam_proto_hdrs})
target_link_libraries(contam_grpc_proto
  absl::check
  ${_REFLECTION}
  ${_GRPC_GRPCPP}
  ${_PROTOBUF_LIBPROTOBUF})

# Target
#target_include_directories(${CONTAM_CLIENT} PUBLIC headers)
target_link_libraries(${CONTAM_CLIENT} PRIVATE
    contam_grpc_proto
    absl::check
    absl::flags
    absl::flags_parse
    absl::log
    absl::log_initialize
    ${_REFLECTION}
    ${_GRPC_GRPCPP}
    ${_PROTOBUF_LIBPROTOBUF})
