set(DWARFEXPR_SOURCES
	dwarfexpr.cpp
)

add_executable(dwarfexpr ${DWARFEXPR_SOURCES})
target_link_libraries(dwarfexpr retdec-dwarfparser)
target_include_directories(dwarfexpr PUBLIC ${PROJECT_SOURCE_DIR}/include/)
