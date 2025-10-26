add_executable(client client.c)

target_link_libraries(client common)

install(TARGETS client DESTINATION bin)