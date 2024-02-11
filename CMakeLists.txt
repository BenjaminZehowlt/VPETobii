cmake_minimum_required(VERSION 3.26)
project(libeyetrack)

set(CMAKE_CXX_STANDARD 17)

# ---- CPM Package Manager ----
set(CPM_DOWNLOAD_VERSION 0.34.0)

if(CPM_SOURCE_CACHE)
    set(CPM_DOWNLOAD_LOCATION "${CPM_SOURCE_CACHE}/cpm/CPM_${CPM_DOWNLOAD_VERSION}.cmake")
elseif(DEFINED ENV{CPM_SOURCE_CACHE})
    set(CPM_DOWNLOAD_LOCATION "$ENV{CPM_SOURCE_CACHE}/cpm/CPM_${CPM_DOWNLOAD_VERSION}.cmake")
else()
    set(CPM_DOWNLOAD_LOCATION "${CMAKE_BINARY_DIR}/cmake/CPM_${CPM_DOWNLOAD_VERSION}.cmake")
endif()

if(NOT (EXISTS ${CPM_DOWNLOAD_LOCATION}))
    message(STATUS "Downloading CPM.cmake to ${CPM_DOWNLOAD_LOCATION}")
    file(DOWNLOAD
            https://github.com/TheLartians/CPM.cmake/releases/download/v${CPM_DOWNLOAD_VERSION}/CPM.cmake
            ${CPM_DOWNLOAD_LOCATION}
    )
endif()

include(${CPM_DOWNLOAD_LOCATION})

# ---- Dependencies ----
CPMAddPackage(
        NAME
        libusb
        GIT_REPOSITORY
        https://github.com/libusb/libusb-cmake.git
        GIT_TAG
        main
)

add_library(libeyetrack SHARED libeyetrack.cpp
        vive_control_t.h
        tobii.h)

target_include_directories(libeyetrack PRIVATE "${LIBUSB_GEN_INCLUDES}")
target_link_libraries(libeyetrack PRIVATE usb-1.0)

target_link_options(libeyetrack PRIVATE "$<$<CONFIG:Release>:/DEBUG>")
target_link_options(libeyetrack PRIVATE "$<$<CONFIG:Release>:/OPT:REF>")
target_link_options(libeyetrack PRIVATE "$<$<CONFIG:Release>:/OPT:ICF>")