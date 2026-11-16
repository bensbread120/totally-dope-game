# CMakeList.txt : CMake project for sdl3-demo, include source and define
# project specific logic here.
#
# SDL3, SDL3_image and glm are made available by the top-level CMakeLists.txt.

# Add source to this project's executable.
add_executable (sdl3-demo "sdl3-demo.cpp" "sdl3-demo.h" "timer.h" "animation.h" "gameobject.h" "mapgen.h" "engine/sdl_app.h" "engine/camera.h" "engine/entity.h" "engine/collision.h" "engine/texture_cache.h")

set_target_properties(sdl3-demo PROPERTIES
  CXX_STANDARD 20
  CXX_STANDARD_REQUIRED ON
)

target_link_libraries(sdl3-demo PRIVATE SDL3::SDL3 SDL3_image::SDL3_image glm::glm)

# Windows has no rpath, so an exe only finds DLLs that sit next to it (or on
# PATH). Copy every DLL the exe links against into its output folder.
if (WIN32)
  add_custom_command(TARGET sdl3-demo POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_RUNTIME_DLLS:sdl3-demo> $<TARGET_FILE_DIR:sdl3-demo>
    COMMAND_EXPAND_LISTS
  )
endif()

# The game loads textures from an assets/ folder next to the exe. This is a
# separate target, rather than a post-build step, so it reruns on every build
# and picks up asset changes even when no code has changed.
add_custom_target(copy-assets
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${PROJECT_SOURCE_DIR}/assets $<TARGET_FILE_DIR:sdl3-demo>/assets
)
add_dependencies(sdl3-demo copy-assets)
