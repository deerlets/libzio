cmake_minimum_required(VERSION 2.8)

file(GLOB SRC serial/*.c socket/*.c)
file(GLOB INC serial/*.h socket/*.h)

add_library(zio SHARED ${SRC})
set_target_properties(zio PROPERTIES VERSION 0.1.0 SOVERSION 0.1)

if (WIN32)
	target_link_libraries(socket Ws2_32)
endif ()

install(TARGETS zio
		RUNTIME DESTINATION bin
		LIBRARY DESTINATION lib
		ARCHIVE DESTINATION lib)
