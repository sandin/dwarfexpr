set(MINIDUMP_DUMP_SOURCES
	minidump_dump.cpp
)

add_executable(minidump_dump ${MINIDUMP_DUMP_SOURCES})
if (${CAPSTONE_ENABLED})
	add_definitions(-DCAPSTONE_ENABLED)
	target_link_libraries(minidump_dump minidump libcapstone.a)
else()
	target_link_libraries(minidump_dump minidump)
endif()