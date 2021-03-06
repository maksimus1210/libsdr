cmake_minimum_required(VERSION 2.8.8)
project(libsdr)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/cmake)
include(InstallHeadersWithDirectory)
include(LinkResources)

OPTION(BUILD_EXAMPLES "Build examples" OFF)
OPTION(BUILD_UNIT_TESTS "Build unit tests" OFF)
OPTION(BUILD_COMMANDLINETOOL "Build command line tool" ON)

SET(libsdr_VERSION_MAJOR "0")
SET(libsdr_VERSION_MINOR "1")
SET(libsdr_VERSION_PATCH "0")

find_package(FFTW)
find_package(FFTWSingle)
find_package(PortAudio)
find_package(RTLSDR)

INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/src)
INCLUDE_DIRECTORIES(${PROJECT_BINARY_DIR}/src)
INCLUDE_DIRECTORIES(${PORTAUDIO_INCLUDE_DIRS})
INCLUDE_DIRECTORIES(${GETOPT_INCLUDE_DIRS})

# Set some variables for the configuration file
IF(FFTW_FOUND)
  set(SDR_WITH_FFTW ON)
ELSE(FFTW_FOUND)
  set(FFTW_LIBRARIES)
  set(FFTWSingle_LIBRARIES)
ENDIF(FFTW_FOUND)

IF(PORTAUDIO_FOUND)
  set(SDR_WITH_PORTAUDIO ON)
ELSE(PORTAUDIO_FOUND)
  set(PORTAUDIO_LIBRARIES)
ENDIF(PORTAUDIO_FOUND)

IF(RTLSDR_FOUND)
  set(SDR_WITH_RTLSDR ON)
ELSE(TRLSDR_FOUND)
  set(RTLSDR_LIBRARIES)
ENDIF(RTLSDR_FOUND)


set(LIBS ${FFTW_LIBRARIES} ${FFTWSingle_LIBRARIES} ${PORTAUDIO_LIBRARIES} ${RTLSDR_LIBRARIES}
         "pthread")

# Set compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -fPIC")
set(CMAKE_CXX_FLAGS_DEBUG   "${CMAKE_CXX_FLAGS_DEBUG} -O0 -ggdb")
set(CMAKE_CXX_FLAGS_DEBUG   "${CMAKE_CXX_FLAGS_DEBUG} -O0 -ggdb")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -ggdb")

LINK_DIRECTORIES(${PROJECT_BINARY_DIR}/src)

IF(CMAKE_BUILD_TYPE MATCHES DEBUG)
  SET(SDR_DEBUG ON)
ENDIF(CMAKE_BUILD_TYPE MATCHES DEBUG)


# Create config.hh
CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/src/config.hh.in
               ${CMAKE_CURRENT_BINARY_DIR}/src/config.hh)


#
# Get default install directories under Linux
#
IF(UNIX AND NOT APPLE)
 INCLUDE(GNUInstallDirs)
ENDIF(UNIX AND NOT APPLE)
IF(UNIX AND APPLE)
 SET(CMAKE_INSTALL_LIBDIR "lib")
 SET(CMAKE_INSTALL_FULL_LIBDIR "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}")
 SET(CMAKE_INSTALL_INCLUDEDIR "include")
 SET(CMAKE_INSTALL_FULL_INCLUDEDIR "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_INCLUDEDIR}")
 # Set RPATH under MacOS
 SET(CMAKE_SKIP_RPATH FALSE)
 SET(CMAKE_SKIP_BUILD_RPATH FALSE)
 SET(CMAKE_INSTALL_RPATH ${CMAKE_INSTALL_FULL_LIBDIR})
 SET(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
 SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
ENDIF(UNIX AND APPLE)

# Add core library, and unit tests
add_subdirectory(src)

IF(BUILD_UNIT_TESTS)
 add_subdirectory(test)
ENDIF(BUILD_UNIT_TESTS)

IF(BUILD_EXAMPLES)
 add_subdirectory(examples)
ENDIF(BUILD_EXAMPLES)

IF(BUILD_COMMANDLINETOOL)
 add_subdirectory(cmd)
endif(BUILD_COMMANDLINETOOL)

# Source distribution packages:
set(CPACK_PACKAGE_VERSION_MAJOR ${libsdr_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${libsdr_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${libsdr_VERSION_PATCH})
set(CPACK_SOURCE_GENERATOR "TGZ")
set(CPACK_SOURCE_PACKAGE_FILE_NAME
  "${CMAKE_PROJECT_NAME}-${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}")
set(CPACK_SOURCE_IGNORE_FILES
  "/build/;/doc/;/dist/;/.git/;.dat$;.wav$;~$;.qm$;${CPACK_SOURCE_IGNORE_FILES}")

include(CPack)
