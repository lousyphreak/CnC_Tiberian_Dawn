cmake_minimum_required(VERSION 3.24)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Choose the type of build." FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

project(TiberianDawn VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(TD_ENABLE_ASAN "Enable AddressSanitizer" OFF)
option(TD_ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer" OFF)

if(NOT EXISTS "${CMAKE_SOURCE_DIR}/extern/SDL3/CMakeLists.txt")
  message(FATAL_ERROR "SDL3 submodule is missing. Run: git submodule update --init --recursive")
endif()

if(ANDROID)
  set(SDL_SHARED ON CACHE BOOL "" FORCE)
  set(SDL_STATIC OFF CACHE BOOL "" FORCE)
else()
  set(SDL_SHARED OFF CACHE BOOL "" FORCE)
  set(SDL_STATIC ON CACHE BOOL "" FORCE)
endif()
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
set(SDL_TESTS OFF CACHE BOOL "" FORCE)
set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SDL_INSTALL OFF CACHE BOOL "" FORCE)
set(SDL_DISABLE_INSTALL OFF CACHE BOOL "" FORCE)

add_subdirectory(extern/SDL3 EXCLUDE_FROM_ALL)

find_package(Threads REQUIRED)

if(TARGET SDL3::SDL3)
  set(TD_SDL_TARGET SDL3::SDL3)
elseif(TARGET SDL3::SDL3-static)
  set(TD_SDL_TARGET SDL3::SDL3-static)
elseif(TARGET SDL3-static)
  set(TD_SDL_TARGET SDL3-static)
else()
  message(FATAL_ERROR "Bundled SDL3 target was not created")
endif()

set(TD_COMMON_DEFINITIONS
  ENGLISH=1
  NOMINMAX
)

set(TD_COMMON_INCLUDE_DIRS
  ${CMAKE_SOURCE_DIR}/SDL3_COMPAT/wrappers
  ${CMAKE_SOURCE_DIR}/CODE
  ${CMAKE_SOURCE_DIR}/WIN32LIB/INCLUDE
)

file(GLOB TD_GAME_SOURCES CONFIGURE_DEPENDS
  "${CMAKE_SOURCE_DIR}/CODE/*.CPP"
  "${CMAKE_SOURCE_DIR}/CODE/*.C"
)
list(SORT TD_GAME_SOURCES)
list(FILTER TD_GAME_SOURCES EXCLUDE REGEX "/ALLOC\\.CPP$")
list(FILTER TD_GAME_SOURCES EXCLUDE REGEX "/CWSTUB\\.C$")
list(FILTER TD_GAME_SOURCES EXCLUDE REGEX "/(CCDDE|DDE|DPMI|IPX|IPXMGR|NULLCONN|NULLDLG|NULLMGR|TEMP)\\.CPP$")
if(WIN32)
  list(FILTER TD_GAME_SOURCES EXCLUDE REGEX "/SOCKETS_LINUX\\.CPP$")
else()
  list(FILTER TD_GAME_SOURCES EXCLUDE REGEX "/SOCKETS_WINDOWS\\.CPP$")
endif()

set(TD_WIN32LIB_GLOBS
  "${CMAKE_SOURCE_DIR}/WIN32LIB/AUDIO/*.CPP"
  "${CMAKE_SOURCE_DIR}/WIN32LIB/DIPTHONG/*.CPP"
  "${CMAKE_SOURCE_DIR}/WIN32LIB/DRAWBUFF/*.CPP"
  "${CMAKE_SOURCE_DIR}/WIN32LIB/FONT/*.CPP"
  "${CMAKE_SOURCE_DIR}/WIN32LIB/IFF/*.CPP"
  "${CMAKE_SOURCE_DIR}/WIN32LIB/KEYBOARD/*.CPP"
  "${CMAKE_SOURCE_DIR}/WIN32LIB/MEM/*.CPP"
  "${CMAKE_SOURCE_DIR}/WIN32LIB/MISC/*.CPP"
  "${CMAKE_SOURCE_DIR}/WIN32LIB/MONO/*.CPP"
  "${CMAKE_SOURCE_DIR}/WIN32LIB/PALETTE/*.CPP"
  "${CMAKE_SOURCE_DIR}/WIN32LIB/SHAPE/*.CPP"
  "${CMAKE_SOURCE_DIR}/WIN32LIB/TILE/*.CPP"
  "${CMAKE_SOURCE_DIR}/WIN32LIB/TIMER/*.CPP"
  "${CMAKE_SOURCE_DIR}/WIN32LIB/VQA32/*.CPP"
  "${CMAKE_SOURCE_DIR}/WIN32LIB/WSA/*.CPP"
  "${CMAKE_SOURCE_DIR}/WIN32LIB/WW_WIN/*.CPP"
)
file(GLOB TD_WIN32LIB_SOURCES CONFIGURE_DEPENDS ${TD_WIN32LIB_GLOBS})
list(FILTER TD_WIN32LIB_SOURCES EXCLUDE REGEX "/(TEST|TST|REDBOOK)\\.CPP$")
list(FILTER TD_WIN32LIB_SOURCES EXCLUDE REGEX "/(PROFILE|WPROFILE)\\.CPP$")
list(FILTER TD_WIN32LIB_SOURCES EXCLUDE REGEX "/VQCOMPAT\\.CPP$")
list(FILTER TD_WIN32LIB_SOURCES EXCLUDE REGEX "/RAWFILE/.*\\.CPP$")

add_library(sdl3_compat STATIC
  SDL3_COMPAT/wrappers/win32_compat.cpp
  SDL3_COMPAT/wrappers/sdl_config.cpp
  SDL3_COMPAT/wrappers/sdl_fs.cpp
  SDL3_COMPAT/wrappers/sdl_draw.cpp
)
target_include_directories(sdl3_compat PUBLIC ${TD_COMMON_INCLUDE_DIRS})
target_compile_definitions(sdl3_compat PUBLIC ${TD_COMMON_DEFINITIONS})

add_library(win32lib STATIC ${TD_WIN32LIB_SOURCES})
target_include_directories(win32lib PUBLIC ${TD_COMMON_INCLUDE_DIRS})
target_compile_definitions(win32lib PUBLIC ${TD_COMMON_DEFINITIONS})

add_executable(tiberian-dawn ${TD_GAME_SOURCES})
target_include_directories(tiberian-dawn PRIVATE ${TD_COMMON_INCLUDE_DIRS})
target_compile_definitions(tiberian-dawn PRIVATE ${TD_COMMON_DEFINITIONS})

set(TD_BUILD_TARGETS sdl3_compat win32lib tiberian-dawn)

foreach(target ${TD_BUILD_TARGETS})
  if(MSVC)
    target_compile_definitions(${target} PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_compile_options(${target} PRIVATE /w /MP)
  else()
    target_compile_options(${target} PRIVATE -Wall -fpermissive)
  endif()
endforeach()

target_link_libraries(sdl3_compat PUBLIC ${TD_SDL_TARGET})
target_link_libraries(win32lib PUBLIC ${TD_SDL_TARGET})
target_link_libraries(tiberian-dawn PRIVATE win32lib sdl3_compat ${TD_SDL_TARGET} Threads::Threads)

if(WIN32)
  target_link_libraries(tiberian-dawn PRIVATE ws2_32)
else()
  target_link_libraries(tiberian-dawn PRIVATE m)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
  set(TD_SANITIZER_FLAGS "")
  if(TD_ENABLE_ASAN)
    string(APPEND TD_SANITIZER_FLAGS " -fsanitize=address")
  endif()
  if(TD_ENABLE_UBSAN)
    string(APPEND TD_SANITIZER_FLAGS " -fsanitize=undefined")
  endif()
  if(NOT TD_SANITIZER_FLAGS STREQUAL "")
    separate_arguments(TD_SANITIZER_FLAGS_LIST NATIVE_COMMAND "${TD_SANITIZER_FLAGS}")
    foreach(target ${TD_BUILD_TARGETS})
      target_compile_options(${target} PRIVATE ${TD_SANITIZER_FLAGS_LIST})
      target_link_options(${target} PRIVATE ${TD_SANITIZER_FLAGS_LIST})
    endforeach()
  endif()
endif()

message(STATUS "TD SDL3/CMake support-layer bring-up enabled.")
message(STATUS "This build now compiles the imported support libraries in-tree and exposes the next TD-side porting blockers.")
