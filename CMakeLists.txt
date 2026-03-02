cmake_minimum_required(VERSION 3.15)
project(euterpe LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compiler options
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -Wall")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -Wall")

# --- Dependencies ---

# Threads (pthread)
find_package(Threads REQUIRED)

# FFTW3: vcpkg (CONFIG) -> pkg-config -> manual search
find_package(FFTW3 CONFIG QUIET)
if(FFTW3_FOUND)
    message(STATUS "FFTW3 found via CONFIG (vcpkg)")
    set(FFTW3_LINK_TARGET FFTW3::fftw3)
else()
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(FFTW3 QUIET fftw3)
    endif()
    if(FFTW3_FOUND)
        message(STATUS "FFTW3 found via pkg-config")
        include_directories(${FFTW3_INCLUDE_DIRS})
        link_directories(${FFTW3_LIBRARY_DIRS})
        set(FFTW3_LINK_TARGET ${FFTW3_LIBRARIES})
    else()
        message(STATUS "FFTW3: fallback to manual search")
        find_path(FFTW3_INCLUDE_DIR fftw3.h)
        find_library(FFTW3_LIB fftw3)
        if(FFTW3_INCLUDE_DIR AND FFTW3_LIB)
            include_directories(${FFTW3_INCLUDE_DIR})
            set(FFTW3_LINK_TARGET ${FFTW3_LIB})
        else()
            message(FATAL_ERROR "FFTW3 not found")
        endif()
    endif()
endif()

# PortAudio: vcpkg (CONFIG) -> pkg-config -> manual search
find_package(portaudio CONFIG QUIET)
if(portaudio_FOUND)
    message(STATUS "PortAudio found via CONFIG (vcpkg)")
    set(PORTAUDIO_LINK_TARGET portaudio_static)
else()
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(PORTAUDIO QUIET portaudio-2.0)
    endif()
    if(PORTAUDIO_FOUND)
        message(STATUS "PortAudio found via pkg-config")
        include_directories(${PORTAUDIO_INCLUDE_DIRS})
        link_directories(${PORTAUDIO_LIBRARY_DIRS})
        set(PORTAUDIO_LINK_TARGET ${PORTAUDIO_LIBRARIES})
    else()
        message(STATUS "PortAudio: fallback to manual search")
        find_path(PORTAUDIO_INCLUDE_DIR portaudio.h)
        find_library(PORTAUDIO_LIB portaudio)
        if(PORTAUDIO_INCLUDE_DIR AND PORTAUDIO_LIB)
            include_directories(${PORTAUDIO_INCLUDE_DIR})
            set(PORTAUDIO_LINK_TARGET ${PORTAUDIO_LIB})
        else()
            message(FATAL_ERROR "PortAudio not found")
        endif()
    endif()
endif()

# Math library
find_library(MATH_LIB m)

# --- Source files ---

set(COMMON_SOURCES
    src/tempoPitch.cpp
    src/gui.cpp
    src/audioPlay.cpp
    src/signalProcessingLibrary.cpp
    src/phaseRecov.cpp
)

# --- Build target ---

add_executable(euterpe
    src/euterpe.cpp
    ${COMMON_SOURCES}
)

target_link_libraries(euterpe
    ${FFTW3_LINK_TARGET}
    ${PORTAUDIO_LINK_TARGET}
    Threads::Threads
    ${MATH_LIB}
)

# Copy Tcl/Tk GUI script to build directory
configure_file(
    ${CMAKE_SOURCE_DIR}/TkTempoConv3.tcl
    ${CMAKE_BINARY_DIR}/TkTempoConv3.tcl
    COPYONLY
)
