add_library(common STATIC ${DUMMY_FILE})
target_sources(common PRIVATE
    logging/log.h
    alignment.h
    assert.h
    bit_field.h
    bit_util.h
    cityhash.cpp
    cityhash.h
    common_funcs.h
    common_paths.h
    common_types.h
    hash.h
    math_util.h
    string_util.h
    string_util.cpp
    swap.h
)

target_link_libraries(common PUBLIC fmt::fmt)
