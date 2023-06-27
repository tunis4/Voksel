project('voksel', ['cpp', 'c'], default_options: ['cpp_std=gnu++20', 'cpp_rtti=false'])

voksel_cpp_arguments = [
  '-Ofast', '-g',
  '-DDEBUG',
  '-DGLM_FORCE_LEFT_HANDED',
  '-DGLM_FORCE_DEPTH_ZERO_TO_ONE',
  '-DGLM_EXT_INCLUDED',
]
add_project_arguments(voksel_cpp_arguments, language: 'cpp')

voksel_source_files = [
  'deps/imgui/imgui.cpp',
  'deps/imgui/imgui_demo.cpp',
  'deps/imgui/imgui_draw.cpp',
  'deps/imgui/imgui_freetype.cpp',
  'deps/imgui/imgui_impl_glfw.cpp',
  'deps/imgui/imgui_tables.cpp',
  'deps/imgui/imgui_widgets.cpp',

  'deps/stb/stb_image.c',

  'deps/vma/vk_mem_alloc.cpp',

# 'deps/PackedArray/PackedArray.c',
  'deps/PackedArray/PackedArraySIMD.c',


  'src/main.cpp',

  'src/block/block.cpp',

  'src/client/client.cpp',
  'src/client/window.cpp',
  'src/client/camera.cpp',

  'src/client/renderer/renderer.cpp',
  'src/client/renderer/vulkan_procs.cpp',
  'src/client/renderer/pipeline.cpp',
  'src/client/renderer/queue_manager.cpp',
  'src/client/renderer/swapchain.cpp',
  'src/client/renderer/texture.cpp',
  'src/client/renderer/persistent_buffer.cpp',
  'src/client/renderer/imgui_impl_voksel.cpp',
  'src/client/renderer/context.cpp',
  'src/client/renderer/box_renderer.cpp',
  'src/client/renderer/chunk_renderer.cpp',
  'src/client/renderer/chunk_mesh_builder.cpp',

  'src/world/world.cpp',
  'src/world/chunk.cpp',
  'src/world/block_storage.cpp',
]

deps_include_dir = include_directories('deps')

glslc = find_program('glslc')

vert_shaders = [
  'res/shaders/src/chunk.vert.glsl',
  'res/shaders/src/box.vert.glsl',
  'res/shaders/src/imgui.vert.glsl',
]

frag_shaders = [
  'res/shaders/src/chunk.frag.glsl',
  'res/shaders/src/box.frag.glsl',
  'res/shaders/src/imgui.frag.glsl',
]

shader_targets = []
foreach s : vert_shaders
  shader_targets += custom_target(
    'shader @0@'.format(s.split('/').get(-1)),
    command: [glslc, '-O', '--target-env=vulkan1.2', '-fshader-stage=vert', '@INPUT@', '-o', '@SOURCE_ROOT@/res/shaders/build/@OUTPUT@'],
    input: s, 
    output: '@BASENAME@.spv'
  )
endforeach
foreach s : frag_shaders
  shader_targets += custom_target(
    'shader @0@'.format(s.split('/').get(-1)),
    command: [glslc, '-O', '--target-env=vulkan1.2', '-fshader-stage=frag', '@INPUT@', '-o', '@SOURCE_ROOT@/res/shaders/build/@OUTPUT@'],
    input: s,
    output: '@BASENAME@.spv'
  )
endforeach

glfw3_dep = dependency('glfw3')
vulkan_dep = dependency('vulkan')
glm_dep = dependency('glm')
freetype2_dep = dependency('freetype2')

cmake = import('cmake')
fastnoise2_opt = cmake.subproject_options()
fastnoise2_opt.add_cmake_defines({'FASTNOISE2_NOISETOOL': 'OFF', 'BUILD_SHARED_LIBS': 'ON'})
fastnoise2_opt.set_override_option('cpp_rtti', 'true')
fastnoise2_proj = cmake.subproject('FastNoise2', options: fastnoise2_opt)
fastnoise2_dep = fastnoise2_proj.dependency('FastNoise')

voksel_dependencies = [
  glfw3_dep,
  vulkan_dep,
  glm_dep,
  freetype2_dep,
  fastnoise2_dep,
]

executable('voksel', [voksel_source_files, shader_targets], include_directories: deps_include_dir, dependencies: voksel_dependencies)
