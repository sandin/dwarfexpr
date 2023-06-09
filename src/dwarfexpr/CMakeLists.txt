set(DWARFEXPR_SOURCES
	dwarf_searcher.cpp
	dwarf_utils.cpp
	dwarf_attrs.cpp
	dwarf_types.cpp
	dwarf_vars.cpp
)

add_library(dwarfexpr STATIC ${DWARFEXPR_SOURCES})
target_link_libraries(dwarfexpr PUBLIC libdwarf.so.0.7.1)
target_include_directories(dwarfexpr PUBLIC ${PROJECT_SOURCE_DIR}/include/)
