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

if(TARGET SDL3::SDL3)
  set(TD_SDL_TARGET SDL3::SDL3)
elseif(TARGET SDL3::SDL3-static)
  set(TD_SDL_TARGET SDL3::SDL3-static)
elseif(TARGET SDL3-static)
  set(TD_SDL_TARGET SDL3-static)
else()
  message(FATAL_ERROR "Bundled SDL3 target was not created")
endif()

file(GLOB TD_GAME_SOURCES CONFIGURE_DEPENDS
  "${CMAKE_SOURCE_DIR}/CODE/*.CPP"
  "${CMAKE_SOURCE_DIR}/CODE/*.C"
)
list(SORT TD_GAME_SOURCES)

set(TD_GAME_INCLUDE_DIRS
  ${CMAKE_SOURCE_DIR}/CODE
)

add_executable(tiberian-dawn ${TD_GAME_SOURCES})
target_include_directories(tiberian-dawn PRIVATE ${TD_GAME_INCLUDE_DIRS})
target_compile_definitions(tiberian-dawn PRIVATE
  ENGLISH=1
  NOMINMAX
)
target_link_libraries(tiberian-dawn PRIVATE ${TD_SDL_TARGET})

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
    target_compile_options(tiberian-dawn PRIVATE ${TD_SANITIZER_FLAGS_LIST})
    target_link_options(tiberian-dawn PRIVATE ${TD_SANITIZER_FLAGS_LIST})
  endif()
endif()

message(STATUS "Initial TD SDL3/CMake bring-up enabled.")
message(STATUS "This first build intentionally exposes the remaining missing support-layer porting work in-tree.")
