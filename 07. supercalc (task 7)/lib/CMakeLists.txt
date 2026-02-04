add_library(lib STATIC
        add.c
        sub.c
        mul.c
        div.c
)

target_include_directories(lib PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)