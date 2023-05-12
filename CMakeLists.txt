cmake_minimum_required(VERSION 3.12 FATAL_ERROR)

project(
  bzip3
  VERSION 1.3.0
  DESCRIPTION "A better and stronger spiritual successor to BZip2"
  HOMEPAGE_URL "https://github.com/kspalaiologos/bzip3"
  LANGUAGES C)

set(CMAKE_C_STANDARD 99)

option(BUILD_SHARED_LIBS "Build libbz3 as a shared library" ON)
option(BZIP3_BUILD_APPS "Build bzip3 applications" ON)
option(BZIP3_ENABLE_PTHREAD "Enable use of pthread library" ON)

include(GNUInstallDirs)

set(CMAKE_INSTALL_RPATH ${CMAKE_INSTALL_FULL_LIBDIR})
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

set(prefix ${CMAKE_INSTALL_PREFIX})
set(exec_prefix ${CMAKE_INSTALL_PREFIX})
set(bindir ${CMAKE_INSTALL_FULL_BINDIR})
set(libdir ${CMAKE_INSTALL_FULL_LIBDIR})
set(includedir ${CMAKE_INSTALL_FULL_INCLUDEDIR})
set(PACKAGE ${CMAKE_PROJECT_NAME})
set(PACKAGE_VERSION ${PROJECT_VERSION})
configure_file(bzip3.pc.in ${CMAKE_CURRENT_BINARY_DIR}/bzip3.pc @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/bzip3.pc
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)

if(BZIP3_ENABLE_PTHREAD)
  set(THREADS_PREFER_PTHREAD_FLAG TRUE)
  find_package(Threads REQUIRED)
endif()

if(BUILD_SHARED_LIBS)
  add_library(bz3 SHARED)
else()
  add_library(bz3 STATIC)
endif()
target_sources(bz3 PRIVATE src/libbz3.c)
target_compile_definitions(bz3 PUBLIC VERSION="${PROJECT_VERSION}")
target_include_directories(
  bz3
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  INTERFACE $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
if(BZIP3_ENABLE_PTHREAD)
  target_compile_definitions(bz3 PRIVATE PTHREAD)
endif()
set_target_properties(
  bz3
  PROPERTIES OUTPUT_NAME bzip3
             SOVERSION "0.0.0"
             PUBLIC_HEADER include/libbz3.h
             VERSION "0")
install(
  TARGETS bz3
  EXPORT ${CMAKE_PROJECT_NAME}-config
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(
  EXPORT ${CMAKE_PROJECT_NAME}-config
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${CMAKE_PROJECT_NAME}
  NAMESPACE ${CMAKE_PROJECT_NAME}::)

if(BZIP3_BUILD_APPS)
  add_executable(bzip3)
  target_sources(bzip3 PRIVATE src/main.c)
  include(CheckIncludeFile)
  check_include_file(getopt.h HAVE_GETOPT_H)
  include(CheckFunctionExists)
  check_function_exists(getopt_long HAVE_GETOPT_LONG)
  if(HAVE_GETOPT_LONG)
    target_compile_definitions(bzip3 PRIVATE HAVE_GETOPT_LONG)
  endif()
  if(BZIP3_ENABLE_PTHREAD)
    target_compile_definitions(bzip3 PRIVATE PTHREAD)
  endif()
  target_link_libraries(bzip3 PRIVATE bz3)
  install(TARGETS bzip3 RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

  set(BZIP3_APP_SCRIPTS bunzip3 bz3cat bz3grep bz3less bz3more bz3most)
  install(PROGRAMS ${BZIP3_APP_SCRIPTS} DESTINATION ${CMAKE_INSTALL_BINDIR})

  set(BZIP3_MANS
      bunzip3.1
      bz3cat.1
      bz3grep.1
      bz3less.1
      bz3more.1
      bz3most.1
      bzip3.1)
  foreach(BZIP3_MAN ${BZIP3_MANS})
    if(EXISTS ${BZIP3_MAN}.in)
      string(TIMESTAMP MAN_DATE "%d %B %Y" UTC)
      set(TRANSFORMED_PACKAGE_NAME ${CMAKE_PROJECT_NAME})
      set(MAN_DATE ${MAN_DATE})
      set(VERSION ${PROJECT_VERSION})
      configure_file(${BZIP3_MAN}.in ${CMAKE_CURRENT_BINARY_DIR}/${BZIP3_MAN}
                     @ONLY)
    else()
      configure_file(${BZIP3_MAN} ${CMAKE_CURRENT_BINARY_DIR}/${BZIP3_MAN}
                     COPYONLY)
    endif()
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${BZIP3_MAN}
            DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)
  endforeach()
endif()
