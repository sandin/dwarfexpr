set(DWARF2LINE_SOURCES
	dwarf2line.cpp
)

add_executable(dwarf2line ${DWARF2LINE_SOURCES})
target_link_libraries(dwarf2line dwarfexpr)
target_include_directories(dwarf2line PUBLIC ${PROJECT_SOURCE_DIR}/include/)
