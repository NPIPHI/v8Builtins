cmake_minimum_required(VERSION 3.4)

add_library(v8Builtins include/v8Builtins/EmbedBuiltins.h src/EmbedBuiltins.cpp src/log.h src/glue.h)

add_dependencies(v8Builtins v8Core AndriodWebsocket)

target_link_libraries(v8Builtins PUBLIC v8Core AndriodWebsocket)

if(DEFINED V8_INCLUDE_DIR)
    target_include_directories(v8Builtins PUBLIC ${V8_INCLUDE_DIR})
else()
    message(FATAL_ERROR "V8_INCLUDE_DIR not set")
endif()

target_include_directories(v8Builtins INTERFACE ../v8Builtins/include)