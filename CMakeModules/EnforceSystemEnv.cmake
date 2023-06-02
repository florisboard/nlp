if (CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
    message(FATAL_ERROR "Unsupported host platform: Windows. Consider using a Linux machine or using WSL2.")
elseif (CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
    # Supported
elseif (CMAKE_HOST_SYSTEM_NAME STREQUAL "Darwin")
    # Supported
else ()
    message(FATAL_ERROR "Unknown host platform: ${CMAKE_HOST_SYSTEM_NAME}. Consider using a Linux machine.")
endif()

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    # Supported
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    message(FATAL_ERROR "Unsupported compiler: GCC")
elseif (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    message(FATAL_ERROR "Unsupported compiler: Visual C++")
else ()
    message(FATAL_ERROR "Unsupported compiler: ${CMAKE_CXX_COMPILER_ID}")
endif()
