set(MINIDUMP_DUMP_SOURCES
	minidump_dump.cpp
)

add_executable(minidump_dump ${MINIDUMP_DUMP_SOURCES})
target_link_libraries(minidump_dump minidump)