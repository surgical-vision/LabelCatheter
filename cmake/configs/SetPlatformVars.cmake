## Compiler configuration
if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_COMPILER_IS_GNUCC)
  set(_GCC_ 1)
endif()

if(MSVC)
  set(_MSVC_ 1)
endif()

## Platform configuration
if(WIN32 OR WIN64)
  set(_WIN_ 1)
endif()

if(UNIX)
  set(_UNIX_ 1)
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
   set(_OSX_ 1)
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
   set(_LINUX_ 1)
endif()

if(ANDROID)
   set(_ANDROID_ 1)
endif()
