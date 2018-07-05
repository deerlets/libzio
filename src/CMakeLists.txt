cmake_minimum_required(VERSION 2.8)

file(GLOB SRC serial/*.c socket/*.c)
file(GLOB INC serial/serial_public.h socket/socket_public.h)

add_library(zio SHARED ${SRC})
set_target_properties(zio PROPERTIES VERSION 0.1.0 SOVERSION 0.1)

if (WIN32)
	target_link_libraries(zio Ws2_32)
endif ()

install(TARGETS zio
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib)

install(FILES ${INC} DESTINATION include/zio)
