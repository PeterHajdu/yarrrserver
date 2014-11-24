cmake_minimum_required(VERSION 2.8)
project(yarrrserver)

option(sanitize "Select sanitizer (address,memory,thread,...)" OFF)

if(UNIX)
    option(enable_libc++ "Enable looking for LLVM's libc++ and using it as standard library if found")
endif(UNIX)


set(LIBS ${LIBS})

if(UNIX)
    if(enable_libc++)
        message("--> Looking for libc++ and libc++abi ...")
        find_library(libcpp_abi c++abi)
        if(libcpp_abi)
            message("libc++ found: ${libcpp_abi}")
            set(LIBS ${LIBS} ${libcpp_abi} -stdlib=libc++)
            add_definitions(-stdlib=libc++)
        else(libcpp_abi)
          message("libc++ not found, falling back to libstdc++")
        endif(libcpp_abi)
    else(enable_libc++)
        message("--> Using libstdc++ as standard library")
    endif(enable_libc++)
endif(UNIX)

if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  set(LIBS dl ${LIBS})
endif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")

include_directories(include)
include_directories(thirdparty/sol)
include_directories(thirdparty/igloo)
include_directories(/usr/local/include/lua52)

add_definitions(-std=c++14
                -Wall
                -Werror
                -g
                -fPIC
                )


if(sanitize)
    message("--> Sanitize: ${sanitize}")
    list(APPEND LIBS -fsanitize=${sanitize})
    add_definitions(-fsanitize=${sanitize})
else(sanitize)
    message("--> Sanitizing is disabled.")
endif(sanitize)

add_subdirectory(src)
add_subdirectory(test)

