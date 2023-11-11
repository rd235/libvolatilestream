cmake_minimum_required(VERSION 3.12)
project(volatilestream
    VERSION 1.0.0
    DESCRIPTION "FILE* stream as a temporary allocated memory buffer"
    HOMEPAGE_URL "https://github.com/rd235/libvolatilestream"
    LANGUAGES C)

include(GNUInstallDirs)
add_definitions(-D_GNU_SOURCE)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -pedantic -O2 -D_FORTIFY_SOURCE=2")

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
set(HEADERS_INSTALL_PATH ${CMAKE_INSTALL_PREFIX}/include)
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)

set(VOLATILESTREAM_SOURCES volatilestream.c)
add_library(volatilestream SHARED ${VOLATILESTREAM_SOURCES})

set_target_properties(volatilestream PROPERTIES VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR})

configure_file(volatilestream.pc.in volatilestream.pc @ONLY)

install(TARGETS volatilestream LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(FILES volatilestream.h DESTINATION ${HEADERS_INSTALL_PATH})
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/volatilestream.pc
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)

add_subdirectory(man)

add_custom_target(uninstall
  "${CMAKE_COMMAND}" -P "${PROJECT_SOURCE_DIR}/Uninstall.cmake")
