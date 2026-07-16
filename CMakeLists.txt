# ============================================================================
#  INKBOUND: THE LAST REEL
#  Master build file.
#
#  Build flavours (set CMAKE_BUILD_TYPE to one of):
#    Debug        -O0 -g3   asserts + dev tools
#    Development  -O2 -g    asserts + dev tools (default)
#    Release      -O2       no asserts, no dev tools
#    Shipping     -O3       no asserts, no dev tools, shipping flags
#
#  Dependencies: SDL2 (>= 2.26). Point CMAKE_PREFIX_PATH / INK_SDL2_PREFIX at it.
# ============================================================================
cmake_minimum_required(VERSION 3.16)
project(inkbound VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "Development" CACHE STRING "Build flavour" FORCE)
endif()
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Debug Development Release Shipping)

# Per-flavour flags (single-config generators use CMAKE_BUILD_TYPE;
# multi-config generators such as Visual Studio use the standard configs).
set(CMAKE_CXX_FLAGS_DEBUG        "-O0 -g3")
set(CMAKE_CXX_FLAGS_DEVELOPMENT  "-O2 -g")
set(CMAKE_CXX_FLAGS_RELEASE      "-O2")
set(CMAKE_CXX_FLAGS_SHIPPING     "-O3")

if(MSVC)
    add_compile_options(/W4 /permissive- /Zc:__cplusplus)
else()
    add_compile_options(-Wall -Wextra)
endif()

# --- SDL2 -------------------------------------------------------------------
set(INK_SDL2_PREFIX "" CACHE PATH "Prefix where SDL2 is installed (lib/include/cmake)")
if(INK_SDL2_PREFIX)
    list(PREPEND CMAKE_PREFIX_PATH "${INK_SDL2_PREFIX}")
endif()
find_package(SDL2 CONFIG QUIET)
if(NOT SDL2_FOUND)
    message(FATAL_ERROR
        "SDL2 not found. Install SDL2 (>= 2.26) or pass -DINK_SDL2_PREFIX=/path/to/sdl2.")
endif()

# --- Shared compile definitions (flavour based) -----------------------------
set(INK_FLAVOUR_DEFS
    $<$<CONFIG:Debug>:INK_DEBUG=1;INK_ASSERTS=1;INK_DEVTOOLS=1>
    $<$<CONFIG:Development>:INK_ASSERTS=1;INK_DEVTOOLS=1>
    $<$<CONFIG:Release>:NDEBUG>
    $<$<CONFIG:Shipping>:NDEBUG;INK_SHIPPING=1>
)

# --- inkcore: pure C++20 core, no SDL --------------------------------------
file(GLOB_RECURSE INKCORE_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_SOURCE_DIR}/Source/Core/*.cpp
    ${CMAKE_SOURCE_DIR}/Source/Physics/*.cpp
    ${CMAKE_SOURCE_DIR}/Source/AI/*.cpp
    ${CMAKE_SOURCE_DIR}/Source/Animation/*.cpp
    ${CMAKE_SOURCE_DIR}/Source/VFX/*.cpp
)
add_library(inkcore STATIC ${INKCORE_SOURCES})
target_include_directories(inkcore PUBLIC ${CMAKE_SOURCE_DIR}/Source)
target_compile_definitions(inkcore PRIVATE ${INK_FLAVOUR_DEFS})

# --- inkgame: the gameplay simulation, no SDL (unit-testable) ---------------
file(GLOB_RECURSE INKGAME_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_SOURCE_DIR}/Source/Gameplay/*.cpp
    ${CMAKE_SOURCE_DIR}/Source/Player/*.cpp
    ${CMAKE_SOURCE_DIR}/Source/Enemies/*.cpp
    ${CMAKE_SOURCE_DIR}/Source/Bosses/*.cpp
    ${CMAKE_SOURCE_DIR}/Source/Weapons/*.cpp
    ${CMAKE_SOURCE_DIR}/Source/Charms/*.cpp
    ${CMAKE_SOURCE_DIR}/Source/Items/*.cpp
    ${CMAKE_SOURCE_DIR}/Source/Levels/*.cpp
    ${CMAKE_SOURCE_DIR}/Source/World/*.cpp
    ${CMAKE_SOURCE_DIR}/Source/Save/*.cpp
    ${CMAKE_SOURCE_DIR}/Source/Cinematics/*.cpp
    ${CMAKE_SOURCE_DIR}/Source/Progression/*.cpp
    ${CMAKE_SOURCE_DIR}/Source/Quests/*.cpp
    ${CMAKE_SOURCE_DIR}/Source/Dialogue/*.cpp
)
add_library(inkgame STATIC ${INKGAME_SOURCES})
target_link_libraries(inkgame PUBLIC inkcore)
target_compile_definitions(inkgame PRIVATE ${INK_FLAVOUR_DEFS})

# --- inkbound-tools: designer/validator CLI (SDL-free) ----------------------
add_executable(inkbound-tools ${CMAKE_SOURCE_DIR}/Source/Editor/main.cpp)
target_link_libraries(inkbound-tools PRIVATE inkgame)
target_compile_definitions(inkbound-tools PRIVATE ${INK_FLAVOUR_DEFS})

# --- inkbound: the game (SDL glue + rendering + audio + UI) ------------------
file(GLOB_RECURSE INKAPP_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_SOURCE_DIR}/Source/Engine/*.cpp
    ${CMAKE_SOURCE_DIR}/Source/Rendering/*.cpp
    ${CMAKE_SOURCE_DIR}/Source/Input/*.cpp
    ${CMAKE_SOURCE_DIR}/Source/Audio/*.cpp
    ${CMAKE_SOURCE_DIR}/Source/UI/*.cpp
    ${CMAKE_SOURCE_DIR}/Source/Art/*.cpp
    ${CMAKE_SOURCE_DIR}/Source/Tools/*.cpp
)
add_executable(inkbound
    ${INKAPP_SOURCES}
    ${CMAKE_SOURCE_DIR}/Source/Game/Game.h
    ${CMAKE_SOURCE_DIR}/Source/Game/Game.cpp
    ${CMAKE_SOURCE_DIR}/Source/Game/main.cpp
)
target_link_libraries(inkbound PRIVATE inkgame SDL2::SDL2)
target_compile_definitions(inkbound PRIVATE ${INK_FLAVOUR_DEFS})

# --- Tests -------------------------------------------------------------------
option(INK_WITH_TESTS "Build unit tests" ON)
if(INK_WITH_TESTS)
    enable_testing()
    file(GLOB INKTEST_SOURCES CONFIGURE_DEPENDS
        ${CMAKE_SOURCE_DIR}/Source/Tests/*.cpp
    )
    add_executable(inkbound-tests ${INKTEST_SOURCES})
    target_link_libraries(inkbound-tests PRIVATE inkgame)
    target_compile_definitions(inkbound-tests PRIVATE ${INK_FLAVOUR_DEFS})
    add_test(NAME unit COMMAND inkbound-tests)
endif()

message(STATUS "INKBOUND configured: SDL2 ${SDL2_VERSION}, flavour ${CMAKE_BUILD_TYPE}")
