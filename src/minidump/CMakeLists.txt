set(MINIDUMP_SOURCES
	minidump.cpp
)

add_library(minidump STATIC ${MINIDUMP_SOURCES})
target_include_directories(minidump PUBLIC  ${PROJECT_SOURCE_DIR}/include/ ${PROJECT_SOURCE_DIR}/include/minidump/breakpad)