set(BREAKPAD_DEMO_SOURCES
	breakpad_demo.cpp
)

# pthread
find_package(Threads REQUIRED)

add_executable(breakpad_demo ${BREAKPAD_DEMO_SOURCES})
if(APPLE) 
	target_link_libraries(breakpad_demo PUBLIC Threads::Threads /usr/local/lib/breakpad.dylib)  # TODO
	target_include_directories(breakpad_demo PUBLIC /usr/local/include /usr/local/include/breakpad)
elseif(UNIX)
	target_link_libraries(breakpad_demo PUBLIC Threads::Threads libbreakpad_client.a)
	target_include_directories(breakpad_demo PUBLIC /usr/local/include /usr/local/include/breakpad)
endif()
