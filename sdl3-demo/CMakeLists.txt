find_package(SDL3 REQUIRED)
find_package(SDL3_image REQUIRED)
find_package(glm REQUIRED)

# CMakeList.txt : CMake project for sdl3-demo, include source and define
# project specific logic here.
#

# Add source to this project's executable.
add_executable (sdl3-demo "sdl3-demo.cpp" "sdl3-demo.h" "timer.h" "animation.h")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET sdl3-demo PROPERTY CXX_STANDARD 20)
endif()

# TODO: Add tests and install targets if needed.
target_link_libraries(sdl3-demo PRIVATE SDL3::SDL3 SDL3_image::SDL3_image glm::glm)