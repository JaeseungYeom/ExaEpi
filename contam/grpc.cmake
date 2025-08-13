cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(Threads REQUIRED)


# Silently check if gRPC and all its dependencies are already installed
option(protobuf_MODULE_COMPATIBLE TRUE)
find_package(Protobuf CONFIG QUIET)
find_package(gRPC CONFIG QUIET)

if(NOT Protobuf_FOUND OR NOT gRPC_FOUND)
  # if gRPC or any of its dependeicies is missing, we need to install it
  set(GRPC_FETCHCONTENT ON)
else()
  message(STATUS "gRPC_DIR: ${gRPC_DIR}")
endif()


if(GRPC_FETCHCONTENT)
  message(STATUS "Installing gRPC via FetchContent.")
  include(FetchContent)
  set(ABSL_ENABLE_INSTALL ON)
  FetchContent_Declare(
    grpc
    GIT_REPOSITORY https://github.com/grpc/grpc.git
    GIT_TAG        v1.73.0)
  FetchContent_MakeAvailable(grpc)

  # Since FetchContent uses add_subdirectory under the hood, we can use
  # the grpc targets directly from this build.
  set(_PROTOBUF_LIBPROTOBUF libprotobuf)
  set(_REFLECTION grpc++_reflection)
  set(_PROTOBUF_PROTOC $<TARGET_FILE:protoc>)
  set(_GRPC_GRPCPP grpc++)
  if(CMAKE_CROSSCOMPILING)
    find_program(_GRPC_CPP_PLUGIN_EXECUTABLE grpc_cpp_plugin)
  else()
    set(_GRPC_CPP_PLUGIN_EXECUTABLE $<TARGET_FILE:grpc_cpp_plugin>)
  endif()
else()
  # Use the gRPC that is already installed
  # Find Protobuf installation
  option(protobuf_MODULE_COMPATIBLE TRUE)
  find_package(Protobuf CONFIG REQUIRED)
  message(STATUS "Using protobuf ${Protobuf_VERSION}")

  set(_PROTOBUF_LIBPROTOBUF protobuf::libprotobuf)
  set(_REFLECTION gRPC::grpc++_reflection)
  if(CMAKE_CROSSCOMPILING)
    find_program(_PROTOBUF_PROTOC protoc)
  else()
    set(_PROTOBUF_PROTOC $<TARGET_FILE:protobuf::protoc>)
  endif()

  # Find gRPC installation
  find_package(gRPC CONFIG REQUIRED)
  message(STATUS "Using gRPC ${gRPC_VERSION}")

  set(_GRPC_GRPCPP gRPC::grpc++)
  if(CMAKE_CROSSCOMPILING)
    find_program(_GRPC_CPP_PLUGIN_EXECUTABLE grpc_cpp_plugin)
  else()
    set(_GRPC_CPP_PLUGIN_EXECUTABLE $<TARGET_FILE:gRPC::grpc_cpp_plugin>)
  endif()
endif()
