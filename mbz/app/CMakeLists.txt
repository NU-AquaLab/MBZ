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

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.

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

                       # Links the target library to the log library
                       # included in the NDK.
                       ${log-lib} )