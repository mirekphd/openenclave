# Copyright (c) Open Enclave SDK contributors.
# Licensed under the MIT License.


oeedl_file(../stdc.edl enclave gen)

# TODO: Does this need CXX?
add_enclave(TARGET stdc_enc UUID 9a30506a-87f6-4214-a007-f15b11c98f8b SOURCES enc.cpp ${gen})

target_compile_options(stdc_enc PRIVATE
    -fno-builtin-strcpy
    -fno-builtin-strcat
    -fno-builtin-memcpy
    -fno-builtin-memset
    )

target_include_directories(stdc_enc PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(stdc_enc oelibc)
