cmake_minimum_required(VERSION 3.15)
project(euterpe LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compiler options
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -Wall")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -Wall")

# --- Build options ---

# -DEUTERPE_USE_FLOAT=ON : 信号処理精度を float に切り替え（FFTW も fftwf_* を使用）
option(EUTERPE_USE_FLOAT "Use float (single) precision for signal processing" OFF)

# --- Dependencies ---

# Threads (pthread)
find_package(Threads REQUIRED)

# FFTW3 (double) or FFTW3f (float) based on EUTERPE_USE_FLOAT
if(EUTERPE_USE_FLOAT)
    set(_fftw_config  "FFTW3f")
    set(_fftw_pkg     "fftw3f")
    set(_fftw_lib     "fftw3f")
    set(_fftw_vtarget "FFTW3::fftw3f")
else()
    set(_fftw_config  "FFTW3")
    set(_fftw_pkg     "fftw3")
    set(_fftw_lib     "fftw3")
    set(_fftw_vtarget "FFTW3::fftw3")
endif()

find_package(${_fftw_config} CONFIG QUIET)
if(${_fftw_config}_FOUND)
    message(STATUS "${_fftw_config} found via CONFIG (vcpkg)")
    set(FFTW3_LINK_TARGET ${_fftw_vtarget})
else()
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(FFTW3_PKG QUIET ${_fftw_pkg})
    endif()
    if(FFTW3_PKG_FOUND)
        message(STATUS "${_fftw_config} found via pkg-config")
        include_directories(${FFTW3_PKG_INCLUDE_DIRS})
        link_directories(${FFTW3_PKG_LIBRARY_DIRS})
        set(FFTW3_LINK_TARGET ${FFTW3_PKG_LIBRARIES})
    else()
        message(STATUS "${_fftw_config}: fallback to manual search")
        find_path(FFTW3_INCLUDE_DIR fftw3.h)
        find_library(FFTW3_LIB ${_fftw_lib})
        if(FFTW3_INCLUDE_DIR AND FFTW3_LIB)
            include_directories(${FFTW3_INCLUDE_DIR})
            set(FFTW3_LINK_TARGET ${FFTW3_LIB})
        else()
            message(FATAL_ERROR "${_fftw_config} not found")
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

# libsndfile (required for FileAudioSource / euterpe_imgui)
find_package(SndFile CONFIG QUIET)
if(SndFile_FOUND)
    message(STATUS "libsndfile found via CONFIG (vcpkg)")
    set(SNDFILE_LINK_TARGET SndFile::sndfile)
else()
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(SNDFILE QUIET sndfile)
    endif()
    if(SNDFILE_FOUND)
        message(STATUS "libsndfile found via pkg-config")
        include_directories(${SNDFILE_INCLUDE_DIRS})
        set(SNDFILE_LINK_TARGET ${SNDFILE_LIBRARIES})
    else()
        message(WARNING "libsndfile not found; euterpe_imgui (file input) will not be built")
        set(SNDFILE_FOUND FALSE)
    endif()
endif()

# Math library
find_library(MATH_LIB m)

# --- Source files ---

# Sources shared by all targets (no GUI-specific code)
set(CORE_SOURCES
    src/tempoPitch.cpp
    src/audioPlay.cpp
    src/signalProcessingLibrary.cpp
    src/phaseRecov.cpp
)

# --- Target: euterpe (Tcl/Tk version) ---

add_executable(euterpe
    src/euterpe.cpp
    src/gui.cpp
    ${CORE_SOURCES}
)

target_link_libraries(euterpe
    ${FFTW3_LINK_TARGET}
    ${PORTAUDIO_LINK_TARGET}
    Threads::Threads
    ${MATH_LIB}
)
if(EUTERPE_USE_FLOAT)
    target_compile_definitions(euterpe PRIVATE EUTERPE_USE_FLOAT)
endif()

# Copy Tcl/Tk GUI script to build directory
configure_file(
    ${CMAKE_SOURCE_DIR}/TkTempoConv3.tcl
    ${CMAKE_BINARY_DIR}/TkTempoConv3.tcl
    COPYONLY
)

# --- Target: euterpe_imgui (Dear ImGui + GLFW version with file input) ---

find_package(imgui    CONFIG QUIET)
find_package(glfw3    CONFIG QUIET)
find_package(OpenGL   QUIET)
find_package(nfd CONFIG QUIET)

if(imgui_FOUND AND glfw3_FOUND AND OpenGL_FOUND AND SndFile_FOUND)
    message(STATUS "ImGui, GLFW, OpenGL, libsndfile found: building euterpe_imgui")

    add_executable(euterpe_imgui
        src/euterpe_imgui.cpp
        src/fileAudioSource.cpp
        ${CORE_SOURCES}
    )

    target_link_libraries(euterpe_imgui
        imgui::imgui
        glfw
        OpenGL::GL
        ${SNDFILE_LINK_TARGET}
        ${FFTW3_LINK_TARGET}
        ${PORTAUDIO_LINK_TARGET}
        Threads::Threads
        ${MATH_LIB}
    )

    if(nfd_FOUND)
        target_link_libraries(euterpe_imgui nfd::nfd)
    else()
        message(WARNING "nativefiledialog-extended not found; file picker unavailable")
        target_compile_definitions(euterpe_imgui PRIVATE NO_NFD)
    endif()
    if(EUTERPE_USE_FLOAT)
        target_compile_definitions(euterpe_imgui PRIVATE EUTERPE_USE_FLOAT)
    endif()
else()
    message(WARNING "ImGui/GLFW/OpenGL/libsndfile not all found; skipping euterpe_imgui")
    if(NOT imgui_FOUND)
        message(WARNING "  missing: imgui")
    endif()
    if(NOT glfw3_FOUND)
        message(WARNING "  missing: glfw3")
    endif()
    if(NOT OpenGL_FOUND)
        message(WARNING "  missing: OpenGL")
    endif()
    if(NOT SndFile_FOUND)
        message(WARNING "  missing: libsndfile")
    endif()
endif()
