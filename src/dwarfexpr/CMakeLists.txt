set(DWARFEXPR_SOURCES
	dwarf_searcher.cpp
	dwarf_utils.cpp
	dwarf_attrs.cpp
	dwarf_tag.cpp
	dwarf_types.cpp
	dwarf_vars.cpp
	dwarf_location.cpp
	dwarf_expression.cpp
)

add_library(dwarfexpr STATIC ${DWARFEXPR_SOURCES})
if(APPLE) 
	target_link_libraries(dwarfexpr PUBLIC /usr/local/lib/libdwarf.dylib)  # TODO
	target_include_directories(dwarfexpr PUBLIC /usr/local/include ${PROJECT_SOURCE_DIR}/include/)
elseif(UNIX)
	target_link_libraries(dwarfexpr PUBLIC libdwarf.so.0.7.1)
	target_include_directories(dwarfexpr PUBLIC ${PROJECT_SOURCE_DIR}/include/)
endif()
