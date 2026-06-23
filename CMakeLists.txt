cmake_minimum_required(VERSION 4.3)

# project definition and language
project(TruthByte 
    VERSION 1.0.0 
    DESCRIPTION "TruthByte - Bytes can't hide from you!"
    LANGUAGES C
)

# language standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# source files
set(SOURCES 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hash.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/commands.c
)

# header files
set(HEADERS 
    ${CMAKE_CURRENT_SOURCE_DIR}/include/truthbyte/commands.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/truthbyte/hash.h
)

# external packages
find_package(OpenSSL REQUIRED)

# sources / executables
add_executable(truthbyte ${SOURCES} ${HEADERS})

# include header files
target_include_directories(truthbyte PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# link libraries
target_link_libraries(truthbyte PRIVATE OpenSSL::Crypto)

## compiler warnings
if(MSVC)
    target_compile_options(truthbyte PRIVATE /W4 /WX)
else()
    target_compile_options(truthbyte PRIVATE 
        -Wall -Wextra  -Wpedantic -Werror
    )
endif()
