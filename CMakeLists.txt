cmake_minimum_required (VERSION 3.19)
cmake_policy(VERSION 3.19)
project(LearnOpenGL)
find_package(OpenGL REQUIRED)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED true)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(BUILD_SHARED_LIBS OFF)
set(GLM_ENABLE_CXX_20 ON)
set(SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(INCLUDES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/includes")
set(IMGUI_DIR "${INCLUDES_DIR}/imgui")
set(GLAD_DIR "${INCLUDES_DIR}/glad")
set(GLFW_DIR "${INCLUDES_DIR}/glfw")
set(STB_DIR "${INCLUDES_DIR}/stb")
set(GLM_DIR "${INCLUDES_DIR}/glm")
set(FMT_DIR "${INCLUDES_DIR}/fmt")
set(ASSIMP_DIR "${INCLUDES_DIR}/assimp")
add_subdirectory(${GLAD_DIR}/cmake glad_cmake)
add_subdirectory(${GLFW_DIR})
add_subdirectory(${GLM_DIR})
add_subdirectory(${FMT_DIR})
add_subdirectory(${ASSIMP_DIR})
glad_add_library(glad SHARED API gl:core=3.3)
add_library(stb ${SOURCE_DIR}/stb.cpp)
target_include_directories(stb PUBLIC ${STB_DIR})
set(
  EXTERNAL_SOURCES
  ${IMGUI_DIR}/imgui.cpp
  ${IMGUI_DIR}/imgui_draw.cpp
  ${IMGUI_DIR}/imgui_demo.cpp
  ${IMGUI_DIR}/imgui_tables.cpp
  ${IMGUI_DIR}/imgui_widgets.cpp
  ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
  ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
  ${STB_DIR}
)
set(
  EXTERNAL_INCLUDES
  PUBLIC ${IMGUI_DIR}
  PUBLIC ${IMGUI_DIR}/backends
  PUBLIC ${GLFW_DIR}/include
  PUBLIC ${STB_DIR}
)
set(
  EXTERNAL_LIBRARIES
  OpenGL::GL
  glad
  glfw
  stb
  glm::glm
  fmt
  assimp
)

function(create_executable name directory)
  add_executable(
    ${name}
    ${directory}/main.cpp
    ${EXTERNAL_SOURCES}
  )
  target_include_directories(
    ${name}
    ${EXTERNAL_INCLUDES}
  )
  target_link_libraries(
    ${name}
    ${EXTERNAL_LIBRARIES}
  )
  set(
    STATIC_FILE_PATH
    ${CMAKE_BINARY_DIR}/resources/${name}
  )
  target_compile_definitions(
    ${name}
    PRIVATE STATIC_FILE_PATH="${STATIC_FILE_PATH}"
  )
  target_compile_definitions(
    ${name}
    PRIVATE WINDOW_TITLE="${name}"
  )
  file(
    GLOB ASSETS
    LIST_DIRECTORIES true
    "${directory}/*.frag"
    "${directory}/*.vert"
    "${directory}/*.geom"
    "${directory}/*.jpg"
    "${directory}/*.png"
    "${directory}/models"
    "${directory}/resources"
  )
  foreach(ASSET ${ASSETS})
    file(
      COPY
      ${ASSET}
      DESTINATION ${STATIC_FILE_PATH}
    )
  endforeach(ASSET)
endfunction()

create_executable(
  imgui_demo
  ${SOURCE_DIR}/imgui
)
create_executable(
  getting_started__hello_window
  ${SOURCE_DIR}/1.getting_started/1.1.hello_window
)
create_executable(
  getting_started__hello_triangle
  ${SOURCE_DIR}/1.getting_started/2.1.hello_triangle
)
create_executable(
  getting_started__hello_triangle_indexed
  ${SOURCE_DIR}/1.getting_started/2.2.hello_triangle_indexed
)
create_executable(
  getting_started__hello_triangle_exercise2
  ${SOURCE_DIR}/1.getting_started/2.4.hello_triangle_exercise2
)
create_executable(
  getting_started__3.1.shaders_uniform
  ${SOURCE_DIR}/1.getting_started/3.1.shaders_uniform
)
create_executable(
  getting_started__3.2.shaders_interpolation
  ${SOURCE_DIR}/1.getting_started/3.2.shaders_interpolation
)
create_executable(
  getting_started__4.1.textures
  ${SOURCE_DIR}/1.getting_started/4.1.textures
)
create_executable(
  getting_started__4.2.textures_combined
  ${SOURCE_DIR}/1.getting_started/4.2.textures_combined
)
create_executable(
  getting_started__5.1.transformations
  ${SOURCE_DIR}/1.getting_started/5.1.transformations
)
create_executable(
  getting_started__6.1.coordinate_systems
  ${SOURCE_DIR}/1.getting_started/6.1.coordinate_systems
)
create_executable(
  getting_started__6.2.coordinate_systems_depth
  ${SOURCE_DIR}/1.getting_started/6.2.coordinate_systems_depth
)
create_executable(
  getting_started__6.3.coordinate_systems_multiple
  ${SOURCE_DIR}/1.getting_started/6.3.coordinate_systems_multiple
)
create_executable(
  getting_started__7.1.camera_circle
  ${SOURCE_DIR}/1.getting_started/7.1.camera_circle
)
create_executable(
  getting_started__7.2.camera_keyboard_dt
  ${SOURCE_DIR}/1.getting_started/7.2.camera_keyboard_dt
)
create_executable(
  getting_started__7.3.camera_mouse_zoom
  ${SOURCE_DIR}/1.getting_started/7.3.camera_mouse_zoom
)
create_executable(
  lighting__1.colors
  ${SOURCE_DIR}/2.lighting/1.colors
)
create_executable(
  lighting__2.1.basic_lighting_diffuse
  ${SOURCE_DIR}/2.lighting/2.1.basic_lighting_diffuse
)
create_executable(
  lighting__2.2.basic_lighting_specular
  ${SOURCE_DIR}/2.lighting/2.2.basic_lighting_specular
)
create_executable(
  lighting__2.3.basic_lighting_exercise1
  ${SOURCE_DIR}/2.lighting/2.3.basic_lighting_exercise1
)
create_executable(
  lighting__3.1.materials
  ${SOURCE_DIR}/2.lighting/3.1.materials
)
create_executable(
  lighting__3.2.materials
  ${SOURCE_DIR}/2.lighting/3.2.materials
)
create_executable(
  lighting__4.1.lighting_maps_diffuse_map
  ${SOURCE_DIR}/2.lighting/4.1.lighting_maps_diffuse_map
)
create_executable(
  lighting__4.2.lighting_maps_specular_map
  ${SOURCE_DIR}/2.lighting/4.2.lighting_maps_specular_map
)
create_executable(
  lighting__5.1.light_casters_directional
  ${SOURCE_DIR}/2.lighting/5.1.light_casters_directional
)
create_executable(
  lighting__5.2.light_casters_point
  ${SOURCE_DIR}/2.lighting/5.2.light_casters_point
)
create_executable(
  lighting__5.3.light_casters_spot
  ${SOURCE_DIR}/2.lighting/5.3.light_casters_spot
)
create_executable(
  lighting__5.4.light_casters_spot_soft
  ${SOURCE_DIR}/2.lighting/5.4.light_casters_spot_soft
)
create_executable(
  lighting__6.multiple_lights
  ${SOURCE_DIR}/2.lighting/6.multiple_lights
)
create_executable(
  model_loading__1.model_loading
  ${SOURCE_DIR}/3.model_loading/1.model_loading
)
create_executable(
  4.advanced_opengl__1.1depth_testing
  ${SOURCE_DIR}/4.advanced_opengl/1.1depth_testing
)
create_executable(
  4.advanced_opengl__1.2depth_testing_depth_buffer
  ${SOURCE_DIR}/4.advanced_opengl/1.2depth_testing_depth_buffer
)
create_executable(
  4.advanced_opengl__1.3depth_testing_stencil
  ${SOURCE_DIR}/4.advanced_opengl/1.3depth_testing_stencil
)
create_executable(
  4.advanced_opengl__3.1.blending_discard
  ${SOURCE_DIR}/4.advanced_opengl/3.1.blending_discard
)
create_executable(
  4.advanced_opengl__3.2.blending_sort
  ${SOURCE_DIR}/4.advanced_opengl/3.2.blending_sort
)
create_executable(
  4.advanced_opengl__4.1.face_culling
  ${SOURCE_DIR}/4.advanced_opengl/4.1.face_culling
)
create_executable(
  4.advanced_opengl__5.1.framebuffers
  ${SOURCE_DIR}/4.advanced_opengl/5.1.framebuffers
)
create_executable(
  4.advanced_opengl__5.2.framebuffers_kernel
  ${SOURCE_DIR}/4.advanced_opengl/5.2.framebuffers_kernel
)
create_executable(
  4.advanced_opengl__5.3.framebuffers_kernel_blur
  ${SOURCE_DIR}/4.advanced_opengl/5.3.framebuffers_kernel_blur
)
create_executable(
  4.advanced_opengl__5.4.framebuffers_kernel_edge
  ${SOURCE_DIR}/4.advanced_opengl/5.4.framebuffers_kernel_edge
)
create_executable(
  4.advanced_opengl__6.1.cubemaps_skybox
  ${SOURCE_DIR}/4.advanced_opengl/6.1.cubemaps_skybox
)
create_executable(
  4.advanced_opengl__6.2.cubemaps_environment_mapping
  ${SOURCE_DIR}/4.advanced_opengl/6.2.cubemaps_environment_mapping
)
create_executable(
  4.advanced_opengl__6.3.cubemaps_environment_mapping_refraction
  ${SOURCE_DIR}/4.advanced_opengl/6.3.cubemaps_environment_mapping_refraction
)
create_executable(
  4.advanced_opengl__8.advanced_glsl_ubo
  ${SOURCE_DIR}/4.advanced_opengl/8.advanced_glsl_ubo
)
create_executable(
  4.advanced_opengl__9.1.geometry_shader_line
  ${SOURCE_DIR}/4.advanced_opengl/9.1.geometry_shader_line
)
create_executable(
  4.advanced_opengl__9.2.geometry_shader_house
  ${SOURCE_DIR}/4.advanced_opengl/9.2.geometry_shader_house
)
create_executable(
  4.advanced_opengl__9.3.geometry_shader_exploding
  ${SOURCE_DIR}/4.advanced_opengl/9.3.geometry_shader_exploding
)
create_executable(
  4.advanced_opengl__9.4.geometry_shader_normals
  ${SOURCE_DIR}/4.advanced_opengl/9.4.geometry_shader_normals
)
create_executable(
  4.advanced_opengl__10.1.instancing_quads
  ${SOURCE_DIR}/4.advanced_opengl/10.1.instancing_quads
)
create_executable(
  4.advanced_opengl__10.2.asteroids
  ${SOURCE_DIR}/4.advanced_opengl/10.2.asteroids
)
create_executable(
  8.2d_game__breakout
  ${SOURCE_DIR}/8.2d_game/breakout
)
