# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
# 3.24 is the first version where FetchContent supports FIND_PACKAGE_ARGS,
# which the dependency setup below relies on.
cmake_minimum_required (VERSION 3.24)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("sdl3-demo")

# --- Dependencies -----------------------------------------------------------
#
# Each library is declared with FIND_PACKAGE_ARGS, which means:
#   1. CMake first tries find_package() -- so a copy you've installed yourself
#      (pointed at via CMAKE_PREFIX_PATH, or a distro package on Linux) is used
#      as-is.
#   2. If that fails, the source archive below is downloaded into the build
#      directory and built as part of this project.
#
# Pass -DFETCHCONTENT_TRY_FIND_PACKAGE_MODE=NEVER to always download.
include(FetchContent)

# Build SDL as shared libraries (SDL3.dll / libSDL3.so), matching the
# prebuilt packages.
set(SDL_SHARED ON CACHE BOOL "" FORCE)
set(SDL_STATIC OFF CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)

# SDL_image can load many formats, several of which need extra third-party
# libraries. The game only uses PNG, which SDL_image can decode with its
# built-in stb_image backend, so switch off everything that would need an
# external dependency.
set(SDLIMAGE_VENDORED OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_PNG_LIBPNG OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_AVIF OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_JXL OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_TIF OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_WEBP OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_SAMPLES OFF CACHE BOOL "" FORCE)

# glm is header-only; skip building its optional compiled library.
set(GLM_BUILD_LIBRARY OFF CACHE BOOL "" FORCE)
set(GLM_BUILD_INSTALL OFF CACHE BOOL "" FORCE)

FetchContent_Declare(SDL3
  URL https://github.com/libsdl-org/SDL/releases/download/release-3.4.14/SDL3-3.4.14.tar.gz
  FIND_PACKAGE_ARGS
)
FetchContent_Declare(SDL3_image
  URL https://github.com/libsdl-org/SDL_image/releases/download/release-3.4.4/SDL3_image-3.4.4.tar.gz
  FIND_PACKAGE_ARGS
)
FetchContent_Declare(glm
  URL https://github.com/g-truc/glm/archive/refs/tags/1.0.3.tar.gz
  FIND_PACKAGE_ARGS
)
# Order matters: SDL3_image needs SDL3 to already be available.
FetchContent_MakeAvailable(SDL3 SDL3_image glm)

# Include sub-projects.
add_subdirectory ("sdl3-demo")
