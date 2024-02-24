cmake_minimum_required(VERSION 3.8)

project(OMS C CXX)

########################################################################
# Macros and additional flags
########################################################################

INCLUDE( CheckCXXCompilerFlag )

FILE( GLOB filenames "../cmake/*.cmake" )
FOREACH( filename ${filenames} )
  INCLUDE( ${filename} )
ENDFOREACH()
########################################################################

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE debug)
ENABLE_IF_SUPPORTED(CMAKE_CXX_FLAGS "-std=c++20")


find_package(nlohmann_json REQUIRED)
find_package(yaml-cpp REQUIRED)
find_library(YAML_LIB NAMES yaml-cpp PATHS /opt/homebrew/lib/ /usr/local/lib)
find_library(KAFKA_LIB NAMES rdkafka++ PATHS /usr/local/lib/)
find_package(spdlog REQUIRED)

include_directories(/opt/homebrew/include/)
include_directories(/usr/local/include/librdkafka/)
include_directories(${YAML_CPP_INCLUDE_DIR}) # See yaml-cpp-config.cmake file created in the build or install dir
include_directories(${Protobuf_INCLUDE_DIR})

# Proto file
get_filename_component(graph_proto "../protos/graph.proto" ABSOLUTE)
get_filename_component(graph_proto_path "${graph_proto}" PATH)

# Generated sources
set(graph_proto_srcs "${CMAKE_CURRENT_BINARY_DIR}/graph.pb.cc")
set(graph_proto_hdrs "${CMAKE_CURRENT_BINARY_DIR}/graph.pb.h")
set(graph_grpc_srcs "${CMAKE_CURRENT_BINARY_DIR}/graph.grpc.pb.cc")
set(graph_grpc_hdrs "${CMAKE_CURRENT_BINARY_DIR}/graph.grpc.pb.h")
add_custom_command(
      OUTPUT "${graph_proto_srcs}" "${graph_proto_hdrs}" "${graph_grpc_srcs}" "${graph_grpc_hdrs}" 
      COMMAND ${_PROTOBUF_PROTOC}
      ARGS --grpc_out "${CMAKE_CURRENT_BINARY_DIR}"
        --cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
        -I "${graph_proto_path}"
        --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
        "${graph_proto}"
      DEPENDS "${graph_proto}")

        message(STATUS "CMAKE_CURRENT_BINARY_DIR= ${CMAKE_CURRENT_BINARY_DIR}")
# Include generated *.pb.h files
include_directories("${CMAKE_CURRENT_BINARY_DIR}")

# graph_grpc_proto
add_library(graph_grpc_proto
  ${graph_grpc_srcs}
  ${graph_grpc_hdrs}
  ${graph_proto_srcs}
  ${graph_proto_hdrs})
target_link_libraries(graph_grpc_proto
  ${_REFLECTION}
  ${_GRPC_GRPCPP}
  ${_PROTOBUF_LIBPROTOBUF})

########################################################################################
# Build gRPC Service for orchestrator Node (User facing API)
get_filename_component(orchestrator_proto "../protos/orchestrator.proto" ABSOLUTE)
get_filename_component(orchestrator_proto_path "${orchestrator_proto}" PATH)

set(orchestrator_proto_srcs "${CMAKE_CURRENT_BINARY_DIR}/orchestrator.pb.cc")
set(orchestrator_proto_hdrs "${CMAKE_CURRENT_BINARY_DIR}/orchestrator.pb.h")
set(orchestrator_grpc_srcs "${CMAKE_CURRENT_BINARY_DIR}/orchestrator.grpc.pb.cc")
set(orchestrator_grpc_hdrs "${CMAKE_CURRENT_BINARY_DIR}/orchestrator.grpc.pb.h")
add_custom_command(
      OUTPUT "${orchestrator_proto_srcs}" "${orchestrator_proto_hdrs}" "${orchestrator_grpc_srcs}" "${orchestrator_grpc_hdrs}" 
      COMMAND ${_PROTOBUF_PROTOC}
      ARGS --grpc_out "${CMAKE_CURRENT_BINARY_DIR}"
        --cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
        -I "${orchestrator_proto_path}"
        --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
        "${orchestrator_proto}"
      DEPENDS "${orchestrator_proto}")

        message(STATUS "CMAKE_CURRENT_BINARY_DIR= ${CMAKE_CURRENT_BINARY_DIR}")
# Include generated *.pb.h files
include_directories("${CMAKE_CURRENT_BINARY_DIR}")

# orchestrator_grpc_proto
add_library(orchestrator_grpc_proto
  ${orchestrator_grpc_srcs}
  ${orchestrator_grpc_hdrs}
  ${orchestrator_proto_srcs}
  ${orchestrator_proto_hdrs})
target_link_libraries(orchestrator_grpc_proto
  ${_REFLECTION}
  ${_GRPC_GRPCPP}
  ${_PROTOBUF_LIBPROTOBUF})
########################################################################################


# logging
add_library(logging
  "safe_queue.h"
  "thread_guard.h"
  "logging/logging.h"
  "logging/log_signal.h"
  "logging/factory.cc"
  "logging/file_logger.cc"
  "logging/log_processor.cc"
  "logging/spd_logger.cc"
  "logging/stdout_logger.cc"
  )
target_link_libraries(logging
  spdlog::spdlog)

# config parser
add_library(config_parser
  "config/config_parser.h"
  "config/config_parser.cc")
target_link_libraries(config_parser
  ${YAML_CPP_LIBRARIES}
  ${YAML_LIB})

# worker rpc clients
add_library(worker_graph
  "worker/worker_graph_client.h"
  "worker/worker_graph_client.cc"
  )
target_include_directories(worker_graph PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(worker_graph
  ${_GRPC_GRPCPP})

# kafka_poller
add_library(kafka_poller
  "kafka/kafka_strategy_factory.h"
  "kafka/kafka_strategy_factory.cc"
  "kafka/kafka_message_strategy.h"
  "kafka/kafka_message_strategy.cc"
  "kafka/kafka_print_message_strategy.h"
  "kafka/kafka_print_message_strategy.cc"
  "kafka/kafka_delivery_report_cb.h"
  "kafka/kafka_delivery_report_cb.cc"
  "data_source.h"
  "kafka/kafka_data_source.h"
  "kafka/kafka_data_source.cc"
  "kafka/kafka_builder.h"
  "kafka/kafka_builder.cc"
  "thread_dispatcher.h"
  )
target_link_libraries(kafka_poller
   ${KAFKA_LIB})

# graph_helper
add_library(graph_helper
  "graph/helper.h"
  "graph/helper.cc"
  )
target_link_libraries(graph_helper
  worker_graph  
  graph_grpc_proto
  ${_REFLECTION}
  ${_GRPC_GRPCPP}
  ${_PROTOBUF_LIBPROTOBUF}
  nlohmann_json::nlohmann_json)

# graph_client
add_library(graph_client
  "orchestrator/graph_client.h"
  "orchestrator/graph_client.cc"
  )
target_link_libraries(graph_client
  worker_graph  
  graph_grpc_proto
  ${_REFLECTION}
  ${_GRPC_GRPCPP}
  ${_PROTOBUF_LIBPROTOBUF}
  nlohmann_json::nlohmann_json)


# graph_orchestrator
add_library(graph_orchestrator
  "orchestrator/graph_orchestrator.h"
  "orchestrator/graph_orchestrator.cc"
  "orchestrator/orchestrator_builder.h"
  "orchestrator/orchestrator_builder.cc"
  "orchestrator/health_checker.h"
  "orchestrator/health_checker.cc"
  "orchestrator/orchestrator_api.h"
  "orchestrator/orchestrator_api.cc"
  "orchestrator/api_runner.h"
  "orchestrator/api_runner.cc"
  )
target_link_libraries(graph_orchestrator
  graph_client
  orchestrator_grpc_proto
  ${_GRPC_GRPCPP}
  ${_PROTOBUF_LIBPROTOBUF})


# Targets graph_(orchestrator|worker)
add_executable(main
  "main.cc")
target_link_libraries(main
  graph_grpc_proto
  graph_helper
  graph_orchestrator
  graph_client
  kafka_poller
  logging
  ${KAFKA_LIB}
  ${_REFLECTION}
  ${_GRPC_GRPCPP}
  ${_PROTOBUF_LIBPROTOBUF}
  nlohmann_json::nlohmann_json
  config_parser
  )

  # Targets graph_(orchestrator|worker)
add_executable(graph_worker
  "worker/graph_worker.cc")
target_link_libraries(graph_worker
  graph_grpc_proto
  graph_helper
  graph_orchestrator
  graph_client
  kafka_poller
  logging
  ${KAFKA_LIB}
  ${_REFLECTION}
  ${_GRPC_GRPCPP}
  ${_PROTOBUF_LIBPROTOBUF}
  nlohmann_json::nlohmann_json
  config_parser
  )