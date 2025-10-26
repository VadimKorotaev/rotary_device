add_executable(server server.c)

target_link_libraries(server common)

install(TARGETS server DESTINATION bin)