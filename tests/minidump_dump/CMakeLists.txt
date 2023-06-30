set(MINIDUMP_DUMP_SOURCES
	minidump_dump.cpp
)

add_executable(minidump_dump ${MINIDUMP_DUMP_SOURCES})
if (${CAPSTONE_ENABLED})
	add_definitions(-DCAPSTONE_ENABLED)
	if(MSVC)
		target_link_libraries(minidump_dump minidump "C:/Users/liudingsan/Downloads/capstone-3/build/Debug/capstone.lib")
		target_include_directories(minidump_dump PUBLIC "C:/Users/liudingsan/Downloads/capstone-3/include")
	else()
		target_link_libraries(minidump_dump minidump libcapstone.a)
	endif()
else()
	target_link_libraries(minidump_dump minidump)
endif()