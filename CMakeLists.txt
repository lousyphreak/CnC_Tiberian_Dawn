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
option(TD_ENABLE_UDP "Enable UDP multiplayer transport" ON)
option(TD_ENABLE_TCP "Enable TCP direct multiplayer transport" ON)
option(TD_ENABLE_WOL "Enable WebSocket (WOL replacement) multiplayer transport" ON)
option(TD_EMSCRIPTEN_PACKAGE_GAMEDATA "Package the GameData directory into the emscripten output" OFF)
option(TD_EMSCRIPTEN_LAZY_FETCH_GAMEDATA "Load GameData over HTTP on demand in the emscripten build" ON)
option(TD_EMSCRIPTEN_DIAGNOSTIC "Enable Emscripten diagnostic build (ASSERTIONS, SAFE_HEAP, stack checks, symbol names)" OFF)

set(TD_ICON_DIR "${CMAKE_SOURCE_DIR}/resources/icons")
set(TD_WINDOWS_ICON_FILE "${TD_ICON_DIR}/tiberian-dawn.ico")
set(TD_WINDOW_ICON_FILE "${TD_ICON_DIR}/tiberian-dawn-window-icon.png")
set(TD_WEB_ICON_DIR "${CMAKE_SOURCE_DIR}/web/icons")
set(TD_WEB_MANIFEST_FILE "${CMAKE_SOURCE_DIR}/web/site.webmanifest")
set(TD_WINDOWS_RESOURCE_FILE "${CMAKE_SOURCE_DIR}/resources/windows/tiberian-dawn.rc")

if(EMSCRIPTEN)
  set(TD_ENABLE_UDP OFF CACHE BOOL "Enable UDP multiplayer transport" FORCE)
  set(TD_ENABLE_TCP OFF CACHE BOOL "Enable TCP direct multiplayer transport" FORCE)
  set(TD_ENABLE_WOL ON CACHE BOOL "Enable WebSocket (WOL replacement) multiplayer transport" FORCE)
endif()

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

if(TD_ENABLE_UDP)
  list(APPEND TD_COMMON_DEFINITIONS TD_ENABLE_UDP=1)
else()
  list(APPEND TD_COMMON_DEFINITIONS TD_ENABLE_UDP=0)
endif()

if(TD_ENABLE_TCP)
  list(APPEND TD_COMMON_DEFINITIONS TD_ENABLE_TCP=1)
else()
  list(APPEND TD_COMMON_DEFINITIONS TD_ENABLE_TCP=0)
endif()

if(TD_ENABLE_WOL)
  list(APPEND TD_COMMON_DEFINITIONS TD_ENABLE_WOL=1)
else()
  list(APPEND TD_COMMON_DEFINITIONS TD_ENABLE_WOL=0)
endif()

if(EMSCRIPTEN)
  list(APPEND TD_COMMON_DEFINITIONS INTERNET_OFF)
endif()

if(EMSCRIPTEN AND TD_EMSCRIPTEN_LAZY_FETCH_GAMEDATA)
  list(APPEND TD_COMMON_DEFINITIONS TD_EMSCRIPTEN_LAZY_FETCH_GAMEDATA=1)
else()
  list(APPEND TD_COMMON_DEFINITIONS TD_EMSCRIPTEN_LAZY_FETCH_GAMEDATA=0)
endif()

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
list(FILTER TD_GAME_SOURCES EXCLUDE REGEX "/(CCDDE|DDE|NULLCONN|NULLDLG|NULLMGR|TEMP)\\.CPP$")
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
list(FILTER TD_WIN32LIB_SOURCES EXCLUDE REGEX "/KEYBOARD/KEYBOARD\\.CPP$")
list(FILTER TD_WIN32LIB_SOURCES EXCLUDE REGEX "/RAWFILE/.*\\.CPP$")
# LOADPCX.CPP defines Read_PCX_File using raw POSIX I/O; CODE/WINSTUB.CPP provides
# the same symbol using CCFileClass (Mix archive access), which is the correct
# implementation for this port. Excluding LOADPCX.CPP eliminates the ODR violation.
list(FILTER TD_WIN32LIB_SOURCES EXCLUDE REGEX "/IFF/LOADPCX\\.CPP$")

if(EMSCRIPTEN)
  file(GLOB_RECURSE TD_FILTERED_GAMEDATA_ENTRIES
    RELATIVE "${CMAKE_SOURCE_DIR}/GameData"
    "${CMAKE_SOURCE_DIR}/GameData/*"
  )
  set(TD_FILTERED_GAMEDATA_FILES "")
  foreach(td_gamedata_entry IN LISTS TD_FILTERED_GAMEDATA_ENTRIES)
    if(IS_DIRECTORY "${CMAKE_SOURCE_DIR}/GameData/${td_gamedata_entry}")
      continue()
    endif()
    if(td_gamedata_entry MATCHES "^(tiberian-dawn|tiberian-dawn-asan)(\\.exe)?$")
      continue()
    endif()
    string(REPLACE "\\" "/" td_gamedata_entry_normalized "${td_gamedata_entry}")
    list(APPEND TD_FILTERED_GAMEDATA_FILES "${td_gamedata_entry_normalized}")
  endforeach()
  list(SORT TD_FILTERED_GAMEDATA_FILES)

  set(TD_EMSCRIPTEN_EFFECTIVE_PACKAGE_GAMEDATA ${TD_EMSCRIPTEN_PACKAGE_GAMEDATA})
  if(TD_EMSCRIPTEN_LAZY_FETCH_GAMEDATA AND TD_EMSCRIPTEN_PACKAGE_GAMEDATA)
    message(STATUS "TD_EMSCRIPTEN_LAZY_FETCH_GAMEDATA is ON; skipping monolithic GameData preloading")
    set(TD_EMSCRIPTEN_EFFECTIVE_PACKAGE_GAMEDATA OFF)
  endif()

  if(TD_EMSCRIPTEN_LAZY_FETCH_GAMEDATA)
    list(JOIN TD_FILTERED_GAMEDATA_FILES "\n" TD_EMSCRIPTEN_GAMEDATA_MANIFEST_CONTENT)
    set(TD_EMSCRIPTEN_GAMEDATA_MANIFEST "${CMAKE_BINARY_DIR}/tiberian-dawn-assets-manifest.txt")
    if(TD_EMSCRIPTEN_GAMEDATA_MANIFEST_CONTENT)
      file(WRITE "${TD_EMSCRIPTEN_GAMEDATA_MANIFEST}" "${TD_EMSCRIPTEN_GAMEDATA_MANIFEST_CONTENT}\n")
    else()
      file(WRITE "${TD_EMSCRIPTEN_GAMEDATA_MANIFEST}" "")
    endif()
  endif()
endif()

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
if(WIN32)
  target_sources(tiberian-dawn PRIVATE "${TD_WINDOWS_RESOURCE_FILE}")
endif()

set(TD_BUILD_TARGETS sdl3_compat win32lib tiberian-dawn)

foreach(target ${TD_BUILD_TARGETS})
  if(MSVC)
    target_compile_definitions(${target} PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_compile_options(${target} PRIVATE /w /MP)
  else()
    target_compile_options(${target} PRIVATE -Wall -fpermissive)
    if(NOT EMSCRIPTEN)
      target_compile_options(${target} PRIVATE -Werror)
    endif()
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

if(EMSCRIPTEN)
  set_target_properties(tiberian-dawn PROPERTIES SUFFIX ".html")
  set(TD_EMSCRIPTEN_SHELL_FILE "${CMAKE_SOURCE_DIR}/web/shell.html")
  target_link_options(tiberian-dawn PRIVATE
    "-sASYNCIFY"
    "-sASYNCIFY_IMPORTS=ra_set_web_soft_keyboard_active"
    "-sASYNCIFY_STACK_SIZE=1048576"
    "-Wl,--export=emscripten_stack_get_end"
    "-sALLOW_MEMORY_GROWTH=1"
    "-sINITIAL_MEMORY=268435456"
    "-sSTACK_SIZE=8388608"
    "-sMAX_WEBGL_VERSION=2"
    "-sMIN_WEBGL_VERSION=1"
    "-sENVIRONMENT=web"
    "--shell-file" "${TD_EMSCRIPTEN_SHELL_FILE}"
  )
  if(TD_EMSCRIPTEN_DIAGNOSTIC)
    message(STATUS "Emscripten diagnostic build enabled (ASSERTIONS=2, SAFE_HEAP=1, symbol names)")
    target_compile_options(tiberian-dawn PRIVATE
      "-gsource-map"
      "-fno-omit-frame-pointer"
    )
    target_link_options(tiberian-dawn PRIVATE
      "-gsource-map"
      "--profiling-funcs"
      "-sASSERTIONS=2"
      "-sSAFE_HEAP=1"
      "-sSTACK_OVERFLOW_CHECK=2"
      "-sASYNCIFY_ADVISE=1"
    )
  endif()
  if(TD_EMSCRIPTEN_LAZY_FETCH_GAMEDATA)
    target_link_options(tiberian-dawn PRIVATE
      "-sFORCE_FILESYSTEM"
      "-lidbfs.js"
      "--embed-file"
      "${TD_EMSCRIPTEN_GAMEDATA_MANIFEST}@/tiberian-dawn-assets-manifest.txt"
    )
  endif()
  if(TD_EMSCRIPTEN_EFFECTIVE_PACKAGE_GAMEDATA)
    target_link_options(tiberian-dawn PRIVATE
      "--preload-file"
      "${CMAKE_SOURCE_DIR}/GameData@/"
    )
  else()
    add_custom_command(TARGET tiberian-dawn POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E rm -f "$<TARGET_FILE_DIR:tiberian-dawn>/tiberian-dawn.data"
      VERBATIM
    )
  endif()
  set_property(TARGET tiberian-dawn APPEND PROPERTY LINK_DEPENDS "${TD_EMSCRIPTEN_SHELL_FILE}")
  add_custom_command(TARGET tiberian-dawn POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:tiberian-dawn>/icons"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      "${TD_WEB_ICON_DIR}/favicon-16.png"
      "${TD_WEB_ICON_DIR}/favicon-32.png"
      "${TD_WEB_ICON_DIR}/favicon-256.png"
      "${TD_WEB_ICON_DIR}/icon-192.png"
      "${TD_WEB_ICON_DIR}/apple-touch-icon.png"
      "$<TARGET_FILE_DIR:tiberian-dawn>/icons"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      "${TD_WEB_MANIFEST_FILE}"
      "$<TARGET_FILE_DIR:tiberian-dawn>"
    VERBATIM
  )
else()
  add_custom_command(TARGET tiberian-dawn POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:tiberian-dawn>/resources/icons"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      "${TD_WINDOW_ICON_FILE}"
      "$<TARGET_FILE_DIR:tiberian-dawn>/resources/icons"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_SOURCE_DIR}/GameData/resources/icons"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      "${TD_WINDOW_ICON_FILE}"
      "${CMAKE_SOURCE_DIR}/GameData/resources/icons"
    VERBATIM
  )
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
