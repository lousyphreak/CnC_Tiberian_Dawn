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
option(TD_ANDROID_PACKAGE_GAMEDATA "Package the GameData directory into the Android APK assets" ON)
set(TD_ANDROID_SDK_ROOT "" CACHE PATH "Android SDK root used for Android packaging helper discovery")
set(TD_ANDROID_PLATFORM_ROOT "" CACHE PATH "Android SDK platform root containing android.jar for Android packaging")
set(TD_ANDROID_MIN_SDK_VERSION "26" CACHE STRING "Minimum Android SDK level for the native Android build")
set(TD_ANDROID_TARGET_SDK_VERSION "34" CACHE STRING "Target Android SDK level for the native Android build")

set(TD_ICON_DIR "${CMAKE_SOURCE_DIR}/resources/icons")
set(TD_WINDOWS_ICON_FILE "${TD_ICON_DIR}/tiberian-dawn.ico")
set(TD_WINDOW_ICON_FILE "${TD_ICON_DIR}/tiberian-dawn-window-icon.png")
set(TD_ANDROID_ICON_RES_DIR "${TD_ICON_DIR}/android")
set(TD_WEB_ICON_DIR "${CMAKE_SOURCE_DIR}/web/icons")
set(TD_WEB_MANIFEST_FILE "${CMAKE_SOURCE_DIR}/web/site.webmanifest")
set(TD_WINDOWS_RESOURCE_FILE "${CMAKE_SOURCE_DIR}/resources/windows/tiberian-dawn.rc")

if(EMSCRIPTEN)
  set(TD_ENABLE_UDP OFF CACHE BOOL "Enable UDP multiplayer transport" FORCE)
  set(TD_ENABLE_TCP OFF CACHE BOOL "Enable TCP direct multiplayer transport" FORCE)
  set(TD_ENABLE_WOL ON CACHE BOOL "Enable WebSocket (WOL replacement) multiplayer transport" FORCE)
endif()

if(ANDROID)
  set(td_android_sdk_root "")
  if(NOT "${TD_ANDROID_SDK_ROOT}" STREQUAL "")
    set(td_android_sdk_root "${TD_ANDROID_SDK_ROOT}")
  elseif(DEFINED SDL_ANDROID_HOME AND NOT "${SDL_ANDROID_HOME}" STREQUAL "")
    set(td_android_sdk_root "${SDL_ANDROID_HOME}")
  elseif(DEFINED ANDROID_HOME AND NOT "${ANDROID_HOME}" STREQUAL "")
    set(td_android_sdk_root "${ANDROID_HOME}")
  elseif(DEFINED ANDROID_SDK_ROOT AND NOT "${ANDROID_SDK_ROOT}" STREQUAL "")
    set(td_android_sdk_root "${ANDROID_SDK_ROOT}")
  elseif(DEFINED ENV{ANDROID_HOME} AND NOT "$ENV{ANDROID_HOME}" STREQUAL "")
    set(td_android_sdk_root "$ENV{ANDROID_HOME}")
  elseif(DEFINED ENV{ANDROID_SDK_ROOT} AND NOT "$ENV{ANDROID_SDK_ROOT}" STREQUAL "")
    set(td_android_sdk_root "$ENV{ANDROID_SDK_ROOT}")
  endif()

  if(NOT "${td_android_sdk_root}" STREQUAL ""
     AND (NOT DEFINED SDL_ANDROID_HOME OR NOT "${SDL_ANDROID_HOME}" STREQUAL "${td_android_sdk_root}"))
    set(SDL_ANDROID_HOME "${td_android_sdk_root}" CACHE PATH "Path to the Android SDK root" FORCE)
  endif()

  set(td_android_platform_root "")
  if(NOT "${TD_ANDROID_PLATFORM_ROOT}" STREQUAL "")
    set(td_android_platform_root "${TD_ANDROID_PLATFORM_ROOT}")
  elseif(NOT DEFINED SDL_ANDROID_PLATFORM_ROOT OR "${SDL_ANDROID_PLATFORM_ROOT}" STREQUAL "")
    set(td_android_platform_root "")
    set(td_android_platform_version -1)
    if(NOT "${td_android_sdk_root}" STREQUAL "" AND IS_DIRECTORY "${td_android_sdk_root}/platforms")
      file(GLOB td_android_platform_candidates LIST_DIRECTORIES true "${td_android_sdk_root}/platforms/android-*")
      foreach(td_android_platform_candidate IN LISTS td_android_platform_candidates)
        get_filename_component(td_android_platform_name "${td_android_platform_candidate}" NAME)
        if(td_android_platform_name MATCHES "^android-([0-9]+)(\\.[0-9]+)?$"
           AND EXISTS "${td_android_platform_candidate}/android.jar")
          if(CMAKE_MATCH_1 GREATER td_android_platform_version)
            set(td_android_platform_version "${CMAKE_MATCH_1}")
            set(td_android_platform_root "${td_android_platform_candidate}")
          endif()
        endif()
      endforeach()
    endif()
  else()
    set(td_android_platform_root "${SDL_ANDROID_PLATFORM_ROOT}")
  endif()

  if(NOT "${td_android_platform_root}" STREQUAL "")
    get_filename_component(td_android_platform_name "${td_android_platform_root}" NAME)
    set(td_android_sdl_platform_root "${td_android_platform_root}")
    if(td_android_platform_name MATCHES "^android-([0-9]+)\\.[0-9]+$")
      set(td_android_sdl_platform_root "${CMAKE_BINARY_DIR}/android-sdk-platform/android-${CMAKE_MATCH_1}")
      file(MAKE_DIRECTORY "${td_android_sdl_platform_root}")
      file(COPY_FILE
        "${td_android_platform_root}/android.jar"
        "${td_android_sdl_platform_root}/android.jar"
        ONLY_IF_DIFFERENT
      )
    endif()
    set(SDL_ANDROID_PLATFORM_ROOT "${td_android_sdl_platform_root}" CACHE PATH "Path of Android platform" FORCE)
  endif()
endif()

if(NOT EXISTS "${CMAKE_SOURCE_DIR}/extern/SDL3/CMakeLists.txt")
  message(FATAL_ERROR "SDL3 submodule is missing. Run: git submodule update --init --recursive")
endif()

if(ANDROID)
  set(SDL_SHARED ON CACHE BOOL "" FORCE)
  set(SDL_STATIC OFF CACHE BOOL "" FORCE)
  set(SDL_ANDROID_JAR ON CACHE BOOL "" FORCE)
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

if(ANDROID)
  if(TARGET SDL3::SDL3-shared)
    set(TD_ANDROID_SDL_SHARED_TARGET SDL3::SDL3-shared)
  elseif(TARGET SDL3-shared)
    set(TD_ANDROID_SDL_SHARED_TARGET SDL3-shared)
  else()
    message(FATAL_ERROR "Android builds require the bundled SDL3 shared target")
  endif()
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

if(EMSCRIPTEN OR (ANDROID AND TD_ANDROID_PACKAGE_GAMEDATA))
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
    if(td_gamedata_entry_normalized MATCHES "^resources/icons/")
      continue()
    endif()
    list(APPEND TD_FILTERED_GAMEDATA_FILES "${td_gamedata_entry_normalized}")
  endforeach()
  list(SORT TD_FILTERED_GAMEDATA_FILES)
endif()

if(EMSCRIPTEN)
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

if(ANDROID)
  set(TD_ANDROID_ASSET_STAGE_DIR "${CMAKE_BINARY_DIR}/android-assets")
  set(TD_ANDROID_STAGED_ASSETS "")
  if(TD_ANDROID_PACKAGE_GAMEDATA)
    foreach(td_android_asset IN LISTS TD_FILTERED_GAMEDATA_FILES)
      set(td_android_asset_source "${CMAKE_SOURCE_DIR}/GameData/${td_android_asset}")
      set(td_android_asset_destination "${TD_ANDROID_ASSET_STAGE_DIR}/${td_android_asset}")
      get_filename_component(td_android_asset_destination_directory "${td_android_asset_destination}" DIRECTORY)
      add_custom_command(
        OUTPUT "${td_android_asset_destination}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${td_android_asset_destination_directory}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${td_android_asset_source}" "${td_android_asset_destination}"
        DEPENDS "${td_android_asset_source}"
        VERBATIM
      )
      list(APPEND TD_ANDROID_STAGED_ASSETS "${td_android_asset_destination}")
    endforeach()
  endif()
  add_custom_target(tiberian-dawn-android-assets DEPENDS ${TD_ANDROID_STAGED_ASSETS})
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

if(ANDROID)
  add_library(tiberian-dawn SHARED ${TD_GAME_SOURCES})
else()
  add_executable(tiberian-dawn ${TD_GAME_SOURCES})
endif()
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
    if(ANDROID)
      target_compile_options(${target} PRIVATE -Wno-c++11-narrowing)
    endif()
    if(NOT EMSCRIPTEN AND NOT ANDROID)
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
    "-lwebsocket.js"
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
elseif(ANDROID)
  find_package(Java REQUIRED)
  if(NOT TARGET SDL3::Jar OR NOT TARGET SDL3-jar)
    message(FATAL_ERROR "Bundled SDL3 Android Java support was not created; check Java and Android SDK setup")
  endif()
  set(TD_ANDROID_SDL_JAR "$<TARGET_PROPERTY:SDL3-jar,OUTPUT>")

  list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/extern/SDL3/cmake/android")
  find_package(SdlAndroidPlatform MODULE REQUIRED)
  find_package(SdlAndroid MODULE REQUIRED)
  include(SdlAndroidFunctions)

  set(TD_ANDROID_PACKAGE "com.ea.tiberiandawn")
  set(TD_ANDROID_ACTIVITY "TiberianDawnActivity")
  set(TD_ANDROID_DATA_SPLIT_NAME "td_data")
  set(TD_ANDROID_GENERATED_SRC_FOLDER "${CMAKE_CURRENT_BINARY_DIR}/android/tiberian-dawn-src")
  set(TD_ANDROID_GENERATED_MANIFEST "${TD_ANDROID_GENERATED_SRC_FOLDER}/AndroidManifest.xml")
  set(TD_ANDROID_GENERATED_DATA_MANIFEST "${TD_ANDROID_GENERATED_SRC_FOLDER}/AndroidManifest-data.xml")
  string(REPLACE "." "/" TD_ANDROID_JAVA_PACKAGE_PATH "${TD_ANDROID_PACKAGE}")
  set(TD_ANDROID_JAVA_OUTPUT_DIR "${TD_ANDROID_GENERATED_SRC_FOLDER}/${TD_ANDROID_JAVA_PACKAGE_PATH}")
  file(MAKE_DIRECTORY "${TD_ANDROID_JAVA_OUTPUT_DIR}")

  configure_file(
    "${CMAKE_SOURCE_DIR}/android/cmake/AndroidManifest.xml.cmake"
    "${TD_ANDROID_GENERATED_MANIFEST}"
    @ONLY
  )
  configure_file(
    "${CMAKE_SOURCE_DIR}/android/cmake/AndroidDataManifest.xml.cmake"
    "${TD_ANDROID_GENERATED_DATA_MANIFEST}"
    @ONLY
  )
  configure_file(
    "${CMAKE_SOURCE_DIR}/android/cmake/TiberianDawnActivity.java.cmake"
    "${TD_ANDROID_JAVA_OUTPUT_DIR}/${TD_ANDROID_ACTIVITY}.java"
    @ONLY
  )

  file(GLOB_RECURSE TD_ANDROID_SDL_RESOURCE_FILES LIST_DIRECTORIES false
    "${CMAKE_SOURCE_DIR}/extern/SDL3/android-project/app/src/main/res/*"
  )
  list(FILTER TD_ANDROID_SDL_RESOURCE_FILES EXCLUDE REGEX "/mipmap-[^/]+/ic_launcher\\.png$")
  list(SORT TD_ANDROID_SDL_RESOURCE_FILES)

  sdl_android_compile_resources(tiberian-dawn-android-resources
    RESFOLDER "${TD_ANDROID_ICON_RES_DIR}"
    RESOURCES
      ${TD_ANDROID_SDL_RESOURCE_FILES}
  )

  sdl_android_link_resources(tiberian-dawn-apk-linked
    MANIFEST "${TD_ANDROID_GENERATED_MANIFEST}"
    PACKAGE "${TD_ANDROID_PACKAGE}"
    RES_TARGETS tiberian-dawn-android-resources
    MIN_SDK_VERSION "${TD_ANDROID_MIN_SDK_VERSION}"
    TARGET_SDK_VERSION "${TD_ANDROID_TARGET_SDK_VERSION}"
  )

  set(CMAKE_JAVA_COMPILE_FLAGS "-encoding;utf-8")
  set(TD_ANDROID_CLASSES_DIR "${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/tiberian-dawn-java.dir/classes")
  if(NOT IS_DIRECTORY "${TD_ANDROID_CLASSES_DIR}")
    execute_process(COMMAND ${CMAKE_COMMAND} -E make_directory "${TD_ANDROID_CLASSES_DIR}")
  endif()
  set(TD_ANDROID_OUT_JAR "${CMAKE_CURRENT_BINARY_DIR}/tiberian-dawn.jar")
  add_custom_command(
    OUTPUT "${TD_ANDROID_OUT_JAR}"
    COMMAND ${CMAKE_COMMAND} -E rm -rf "${TD_ANDROID_CLASSES_DIR}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${TD_ANDROID_CLASSES_DIR}"
    COMMAND ${Java_JAVAC_EXECUTABLE}
      -source 1.8 -target 1.8
      -bootclasspath "${TD_ANDROID_SDL_JAR}"
      "${TD_ANDROID_JAVA_OUTPUT_DIR}/${TD_ANDROID_ACTIVITY}.java"
      $<TARGET_PROPERTY:tiberian-dawn-apk-linked,JAVA_R>
      -cp "${TD_ANDROID_SDL_JAR}:${SDL_ANDROID_PLATFORM_ANDROID_JAR}"
      -d "${TD_ANDROID_CLASSES_DIR}"
    COMMAND ${Java_JAR_EXECUTABLE} cf "${TD_ANDROID_OUT_JAR}" -C "${TD_ANDROID_CLASSES_DIR}" .
    DEPENDS
      $<TARGET_PROPERTY:tiberian-dawn-apk-linked,OUTPUTS>
      SDL3-jar
      "${TD_ANDROID_JAVA_OUTPUT_DIR}/${TD_ANDROID_ACTIVITY}.java"
    VERBATIM
  )
  add_custom_target(tiberian-dawn-jar DEPENDS "${TD_ANDROID_OUT_JAR}")
  set_property(TARGET tiberian-dawn-jar PROPERTY OUTPUT "${TD_ANDROID_OUT_JAR}")

  set(TD_ANDROID_DEX_WORKDIR "${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/tiberian-dawn-dex.dir")
  if(NOT IS_DIRECTORY "${TD_ANDROID_DEX_WORKDIR}")
    execute_process(COMMAND "${CMAKE_COMMAND}" -E make_directory "${TD_ANDROID_DEX_WORKDIR}")
  endif()
  set(TD_ANDROID_CLASSES_DEX "${TD_ANDROID_DEX_WORKDIR}/classes.dex")
  add_custom_command(
    OUTPUT "${TD_ANDROID_CLASSES_DEX}"
    COMMAND SdlAndroid::d8
      $<TARGET_PROPERTY:tiberian-dawn-jar,OUTPUT>
      "${TD_ANDROID_SDL_JAR}"
      --lib "${SDL_ANDROID_PLATFORM_ANDROID_JAR}"
      --output "${TD_ANDROID_DEX_WORKDIR}"
    DEPENDS $<TARGET_PROPERTY:tiberian-dawn-jar,OUTPUT> SDL3-jar
    VERBATIM
  )
  add_custom_target(tiberian-dawn-dex DEPENDS "${TD_ANDROID_CLASSES_DEX}")
  set_property(TARGET tiberian-dawn-dex PROPERTY OUTPUT "${TD_ANDROID_CLASSES_DEX}")

  set(TD_ANDROID_CODE_APK_WORKDIR "${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/tiberian-dawn-code-apk-contents.dir")
  set(TD_ANDROID_CODE_UNALIGNED_APK "${CMAKE_CURRENT_BINARY_DIR}/intermediates/tiberian-dawn-code-unaligned.apk")
  file(MAKE_DIRECTORY "${TD_ANDROID_CODE_APK_WORKDIR}")
  add_custom_command(
    OUTPUT "${TD_ANDROID_CODE_UNALIGNED_APK}"
    COMMAND ${CMAKE_COMMAND} -E remove_directory "${TD_ANDROID_CODE_APK_WORKDIR}/lib"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${TD_ANDROID_CODE_APK_WORKDIR}/lib/${ANDROID_ABI}"
    COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_PROPERTY:tiberian-dawn-apk-linked,OUTPUT>" "${TD_ANDROID_CODE_UNALIGNED_APK}"
    COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_PROPERTY:tiberian-dawn-dex,OUTPUT>" "${TD_ANDROID_CODE_APK_WORKDIR}/classes.dex"
    COMMAND SdlAndroid::zip -u -q -j "${TD_ANDROID_CODE_UNALIGNED_APK}" classes.dex
    COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:${TD_ANDROID_SDL_SHARED_TARGET}>" "${TD_ANDROID_CODE_APK_WORKDIR}/lib/${ANDROID_ABI}/$<TARGET_FILE_NAME:${TD_ANDROID_SDL_SHARED_TARGET}>"
    COMMAND SdlAndroid::zip -u -q "${TD_ANDROID_CODE_UNALIGNED_APK}" "lib/${ANDROID_ABI}/$<TARGET_FILE_NAME:${TD_ANDROID_SDL_SHARED_TARGET}>"
    COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:tiberian-dawn>" "${TD_ANDROID_CODE_APK_WORKDIR}/lib/${ANDROID_ABI}/$<TARGET_FILE_NAME:tiberian-dawn>"
    COMMAND SdlAndroid::zip -u -q "${TD_ANDROID_CODE_UNALIGNED_APK}" "lib/${ANDROID_ABI}/$<TARGET_FILE_NAME:tiberian-dawn>"
    DEPENDS
      tiberian-dawn
      tiberian-dawn-apk-linked
      tiberian-dawn-dex
      ${TD_ANDROID_SDL_SHARED_TARGET}
    WORKING_DIRECTORY "${TD_ANDROID_CODE_APK_WORKDIR}"
    COMMAND_EXPAND_LISTS
    VERBATIM
  )
  add_custom_target(tiberian-dawn-code-unaligned-apk DEPENDS "${TD_ANDROID_CODE_UNALIGNED_APK}")
  set_property(TARGET tiberian-dawn-code-unaligned-apk PROPERTY OUTPUT "${TD_ANDROID_CODE_UNALIGNED_APK}")

  if(TD_ANDROID_PACKAGE_GAMEDATA)
    sdl_android_link_resources(tiberian-dawn-data-apk-linked
      MANIFEST "${TD_ANDROID_GENERATED_DATA_MANIFEST}"
      PACKAGE "${TD_ANDROID_PACKAGE}"
      MIN_SDK_VERSION "${TD_ANDROID_MIN_SDK_VERSION}"
      TARGET_SDK_VERSION "${TD_ANDROID_TARGET_SDK_VERSION}"
    )

    set(TD_ANDROID_TOPLEVEL_MIX_ASSETS "")
    foreach(TD_ANDROID_FILTERED_GAMEDATA_FILE IN LISTS TD_FILTERED_GAMEDATA_FILES)
      if(TD_ANDROID_FILTERED_GAMEDATA_FILE MATCHES "^[^/]+\\.MIX$")
        list(APPEND TD_ANDROID_TOPLEVEL_MIX_ASSETS "${TD_ANDROID_FILTERED_GAMEDATA_FILE}")
      endif()
    endforeach()
    set(TD_ANDROID_TOPLEVEL_MIX_ARCHIVE_PATHS "")
    foreach(TD_ANDROID_TOPLEVEL_MIX_ASSET IN LISTS TD_ANDROID_TOPLEVEL_MIX_ASSETS)
      list(APPEND TD_ANDROID_TOPLEVEL_MIX_ARCHIVE_PATHS "assets/${TD_ANDROID_TOPLEVEL_MIX_ASSET}")
    endforeach()

    set(TD_ANDROID_DATA_APK_WORKDIR "${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/tiberian-dawn-data-apk-contents.dir")
    set(TD_ANDROID_DATA_UNALIGNED_APK "${CMAKE_CURRENT_BINARY_DIR}/intermediates/tiberian-dawn-data-unaligned.apk")
    file(MAKE_DIRECTORY "${TD_ANDROID_DATA_APK_WORKDIR}")
    add_custom_command(
      OUTPUT "${TD_ANDROID_DATA_UNALIGNED_APK}"
      COMMAND ${CMAKE_COMMAND} -E remove_directory "${TD_ANDROID_DATA_APK_WORKDIR}/assets"
      COMMAND ${CMAKE_COMMAND} -E make_directory "${TD_ANDROID_DATA_APK_WORKDIR}/assets"
      COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_PROPERTY:tiberian-dawn-data-apk-linked,OUTPUT>" "${TD_ANDROID_DATA_UNALIGNED_APK}"
      COMMAND ${CMAKE_COMMAND} -E copy_directory "${TD_ANDROID_ASSET_STAGE_DIR}" "${TD_ANDROID_DATA_APK_WORKDIR}/assets"
      COMMAND SdlAndroid::zip -u -r -q "${TD_ANDROID_DATA_UNALIGNED_APK}" assets
      COMMAND SdlAndroid::zip -d -q "${TD_ANDROID_DATA_UNALIGNED_APK}" ${TD_ANDROID_TOPLEVEL_MIX_ARCHIVE_PATHS}
      COMMAND SdlAndroid::zip -0 -q "${TD_ANDROID_DATA_UNALIGNED_APK}" ${TD_ANDROID_TOPLEVEL_MIX_ARCHIVE_PATHS}
      DEPENDS
        tiberian-dawn-data-apk-linked
        tiberian-dawn-android-assets
      WORKING_DIRECTORY "${TD_ANDROID_DATA_APK_WORKDIR}"
      COMMAND_EXPAND_LISTS
      VERBATIM
    )
    add_custom_target(tiberian-dawn-data-unaligned-apk DEPENDS "${TD_ANDROID_DATA_UNALIGNED_APK}")
    set_property(TARGET tiberian-dawn-data-unaligned-apk PROPERTY OUTPUT "${TD_ANDROID_DATA_UNALIGNED_APK}")
  endif()

  sdl_create_android_debug_keystore(tiberian-dawn-debug-keystore)
  sdl_apk_align(tiberian-dawn-code-aligned-apk tiberian-dawn-code-unaligned-apk
    OUTDIR "${CMAKE_CURRENT_BINARY_DIR}/intermediates"
    NAME "tiberian-dawn-code.aligned.apk"
  )
  sdl_apk_sign(tiberian-dawn-code-apk tiberian-dawn-code-aligned-apk
    KEYSTORE tiberian-dawn-debug-keystore
    OUTPUT "tiberian-dawn-code.apk"
  )
  if(TD_ANDROID_PACKAGE_GAMEDATA)
    sdl_apk_align(tiberian-dawn-data-aligned-apk tiberian-dawn-data-unaligned-apk
      OUTDIR "${CMAKE_CURRENT_BINARY_DIR}/intermediates"
      NAME "tiberian-dawn-data.aligned.apk"
    )
    sdl_apk_sign(tiberian-dawn-data-apk tiberian-dawn-data-aligned-apk
      KEYSTORE tiberian-dawn-debug-keystore
      OUTPUT "tiberian-dawn-data.apk"
    )
    add_custom_target(tiberian-dawn-apk DEPENDS tiberian-dawn-code-apk tiberian-dawn-data-apk)
  else()
    add_custom_target(tiberian-dawn-apk DEPENDS tiberian-dawn-code-apk)
  endif()

  if(TARGET SdlAndroid::adb)
    add_custom_target(install-tiberian-dawn-apk
      COMMAND "${ADB_BIN}" install-multiple -r
        "$<TARGET_PROPERTY:tiberian-dawn-code-apk,OUTPUT>"
        "$<$<BOOL:${TD_ANDROID_PACKAGE_GAMEDATA}>:$<TARGET_PROPERTY:tiberian-dawn-data-apk,OUTPUT>>"
      DEPENDS tiberian-dawn-apk
      VERBATIM
    )
    add_custom_target(install-tiberian-dawn-code-apk
      COMMAND "${ADB_BIN}" install-multiple -r -p "${TD_ANDROID_PACKAGE}"
        "$<TARGET_PROPERTY:tiberian-dawn-code-apk,OUTPUT>"
      DEPENDS tiberian-dawn-code-apk
      VERBATIM
    )
    if(TD_ANDROID_PACKAGE_GAMEDATA)
      add_custom_target(install-tiberian-dawn-data-apk
        COMMAND "${ADB_BIN}" install-multiple -r -p "${TD_ANDROID_PACKAGE}"
          "$<TARGET_PROPERTY:tiberian-dawn-data-apk,OUTPUT>"
        DEPENDS tiberian-dawn-data-apk
        VERBATIM
      )
    endif()
    add_custom_target(start-tiberian-dawn
      COMMAND "${ADB_BIN}" shell am start-activity -S "${TD_ANDROID_PACKAGE}/.${TD_ANDROID_ACTIVITY}"
      VERBATIM
    )
    add_custom_target(build-install-start-tiberian-dawn
      COMMAND "${ADB_BIN}" install-multiple -r
        "$<TARGET_PROPERTY:tiberian-dawn-code-apk,OUTPUT>"
        "$<$<BOOL:${TD_ANDROID_PACKAGE_GAMEDATA}>:$<TARGET_PROPERTY:tiberian-dawn-data-apk,OUTPUT>>"
      COMMAND "${ADB_BIN}" shell am start-activity -S "${TD_ANDROID_PACKAGE}/.${TD_ANDROID_ACTIVITY}"
      DEPENDS tiberian-dawn-apk
      VERBATIM
    )
  endif()
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
