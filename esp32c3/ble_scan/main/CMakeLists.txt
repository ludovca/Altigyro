idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    REQUIRES bt nvs_flash
)
target_link_libraries(${COMPONENT_LIB} m)
