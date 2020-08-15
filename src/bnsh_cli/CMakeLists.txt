add_executable(CLI
    cli.cpp
)

#target_include_directories(CLI PRIVATE ${BNSH_DECOMPILER_SRC_DIR})

target_link_libraries(CLI PUBLIC video_core)

if (${CMAKE_SYSTEM_NAME} MATCHES "Emscripten")
    set_target_properties(CLI PROPERTIES LINK_FLAGS "--bind -o dist/module.js -O3 -s ASSERTIONS=0 -s WASM_ASYNC_COMPILATION=0 -s NODEJS_CATCH_EXIT=0 -s NODEJS_CATCH_REJECTION=0 -s WASM=1 -s MODULARIZE=1 -s ALLOW_MEMORY_GROWTH=1 -s FULL_ES3=1 -s EXTRA_EXPORTED_RUNTIME_METHODS=\"['ccall', 'cwrap']\" -s EXPORTED_FUNCTIONS=\"['_Decode']\" -s EXPORT_NAME=\"'${CMAKE_PROJECT_NAME}'\"")
endif ()
