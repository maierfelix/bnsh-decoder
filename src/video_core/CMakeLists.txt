add_library(video_core STATIC ${DUMMY_FILE})
target_sources(video_core PRIVATE
    engines/const_buffer_engine_interface.h
    engines/const_buffer_info.h
    engines/engine_interface.h
    engines/engine_upload.h
    engines/maxwell_3d.cpp
    engines/maxwell_3d.h
    engines/shader_bytecode.h
    engines/shader_header.h
    engines/shader_type.h

    gpu.h
    guest_driver.cpp
    guest_driver.h

    shader/decode/arithmetic.cpp
    shader/decode/arithmetic_immediate.cpp
    shader/decode/bfe.cpp
    shader/decode/bfi.cpp
    shader/decode/shift.cpp
    shader/decode/arithmetic_integer.cpp
    shader/decode/arithmetic_integer_immediate.cpp
    shader/decode/arithmetic_half.cpp
    shader/decode/arithmetic_half_immediate.cpp
    shader/decode/ffma.cpp
    shader/decode/hfma2.cpp
    shader/decode/conversion.cpp
    shader/decode/memory.cpp
    shader/decode/texture.cpp
    shader/decode/image.cpp
    shader/decode/float_set_predicate.cpp
    shader/decode/integer_set_predicate.cpp
    shader/decode/half_set_predicate.cpp
    shader/decode/predicate_set_register.cpp
    shader/decode/predicate_set_predicate.cpp
    shader/decode/register_set_predicate.cpp
    shader/decode/float_set.cpp
    shader/decode/integer_set.cpp
    shader/decode/half_set.cpp
    shader/decode/video.cpp
    shader/decode/warp.cpp
    shader/decode/xmad.cpp
    shader/decode/other.cpp
    shader/ast.cpp
    shader/ast.h
    shader/compiler_settings.cpp
    shader/compiler_settings.h
    shader/control_flow.cpp
    shader/control_flow.h
    shader/decode.cpp
    shader/expr.cpp
    shader/expr.h
    shader/memory_util.cpp
    shader/memory_util.h
    shader/node.h
    shader/node_helper.cpp
    shader/node_helper.h
    shader/registry.cpp
    shader/registry.h
    shader/shader_ir.cpp
    shader/shader_ir.h
    shader/spirv_decompiler.cpp
    shader/spirv_decompiler.h
    shader/track.cpp
    shader/transform_feedback.cpp
    shader/transform_feedback.h    

    textures/texture.h
)

target_link_libraries(video_core PUBLIC common)

target_include_directories(video_core PRIVATE sirit ../../externals/Vulkan-Headers/include)
target_compile_definitions(video_core PRIVATE HAS_VULKAN)
target_link_libraries(video_core PRIVATE sirit)

if (MSVC)
    target_compile_options(video_core PRIVATE /we4267)
else()
    target_compile_options(video_core PRIVATE -Werror=conversion -Wno-error=sign-conversion)
endif()
