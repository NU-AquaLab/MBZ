# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.4.1)

# Sets the C++ standard.

set (CMAKE_CXX_STANDARD 11)


# Defines preprocessor symbols.

add_definitions(-DLINUX=1)
#add_definitions(-DDEBUG=1)

# Defines include directories.

include_directories("src/main/cpp")

# Collects all pcpp sources by directory.

file(GLOB PCPP_SOURCES "src/main/cpp/pcpp/*.cpp"
                       "src/main/cpp/pcpp/*.h")

# Collects all the net sources by directory
file(GLOB NET_SOURCES "src/main/cpp/net/*.cc"
                        "src/main/cpp/net/*.h"
                      )
#
#file(GLOB GOOGLE_SOURCES "src/main/cpp/google/*.cc"
#                         "src/main/cpp/google/*.h")
#
#file(GLOB TESTING_SOURCES "src/main/cpp/testing/*.cc"
#                          "src/main/cpp/testing/*.h")
#
#file(GLOB CHROMIUM_SOURCES "src/main/cpp/chromium/*.cc"
#                           "src/main/cpp/chromium/*.h")
#
#file(GLOB QUIC_SOURCES "src/main/cpp/quic/src/*.cc"
#                       "src/main/cpp/quic/src/*.h")
#
#file(GLOB BASE_SOURCES "src/main/cpp/base/*.cc"
#                       "src/main/cpp/base/*.h")

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.

#add_library(#Sets the name
#        quic-lib
#
#        #Set the library as a shared library
#        SHARED
#
#        "src/main/cpp/net/temp.cpp"
##        "src/main/cpp/net/quic/test_tools/quic_test_utils.cc"
#
#
#        #Provides a relative path to the source files
#        ${NET_SOURCES}
#        )

#add_library(
#        google-lib
#
#        SHARED
#
#        "src/main/cpp/google/temp.cpp"
#
#        ${GOOGLE_SOURCES}
#)
#
#add_library(
#        testing_lib
#
#        SHARED
#
#        "src/main/cpp/testing/temp.cpp"
#
#        ${TESTING_SOURCES}
#)
#
#add_library(
#        chromium-lib
#
#        SHARED
#
#        "src/main/cpp/chromium/temp.cpp"
#
#        ${CHROMIUM_SOURCES}
#)
#
#add_library(
#        base-lib
#
#        SHARED
#
#        "src/main/cpp/base/temp.cpp"
#
#        ${BASE_SOURCES}
#)
#
#add_library(
#        quic-lib
#
#        SHARED
#
#        "src/main/cpp/quic/temp.cpp"
#
#        ${QUIC_SOURCES}
#)

add_library( # Sets the name of the library.
             mbz-lib

             # Sets the library as a shared library.
             SHARED

             # Provides a relative path to your source file(s).
             src/main/cpp/mbz-lib.cpp

             src/main/cpp/common/utils.cpp

             src/main/cpp/routing/Flow.cpp
             src/main/cpp/routing/FlowId.cpp
             src/main/cpp/routing/FlowTable.cpp
             src/main/cpp/routing/InPacket.cpp
             src/main/cpp/routing/Jni.cpp
             src/main/cpp/routing/OutPacket.cpp
             src/main/cpp/routing/Plugin.cpp
             src/main/cpp/routing/PluginTable.cpp
             src/main/cpp/routing/Router.cpp
             src/main/cpp/routing/ServiceTable.cpp
             src/main/cpp/routing/TcpFlow.cpp
             src/main/cpp/routing/UdpFlow.cpp

             src/main/cpp/services/FlowStatsService.cpp
             src/main/cpp/services/Service.cpp
             src/main/cpp/services/UiService.cpp
             src/main/cpp/services/WifiService.cpp

             ${PCPP_SOURCES})

# Searches for a specified prebuilt library and stores the path as a
# variable. Because CMake includes system libraries in the search path by
# default, you only need to specify the name of the public NDK library
# you want to add. CMake verifies that the library exists before
# completing its build.

find_library( # Sets the name of the path variable.
              log-lib

              # Specifies the name of the NDK library that
              # you want CMake to locate.
              log )

# Specifies libraries CMake should link to your target library. You
# can link multiple libraries, such as libraries you define in this
# build script, prebuilt third-party libraries, or system libraries.

target_link_libraries( # Specifies the target library.
                       mbz-lib
#                       base-lib
#                       quic-lib
                       #chromium-lib
#                       quic-lib
#                       google-lib
#                       testing-lib
                       # Links the target library to the log library
                       # included in the NDK.
                       ${log-lib} )