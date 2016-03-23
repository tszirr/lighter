project(lighter)

option(LIGHTER_USE_OPENGL "Enable OpenGL" ON)
option(LIGHTER_USE_FREEIMAGE "Enable image loading using freeimage" ON)
option(LIGHTER_USE_FREETYPE "Enable font rendering using freetype" ON)
option(LIGHTER_USE_SCENE "Enable scene-related functionality" ON)
option(LIGHTER_INSTALL_DATA_DIRECTORY "Install data files that lighter functionality depends on" ON)
option(LIGHTER_INSTALL_LIB "Install generated library" OFF)

set(LIGHTER_SHADER_SRC
  text.glsl
  ui.glsl
)
set(LIGHTER_DATA_FILES
  ${LIGHTER_SHADER_SRC}
)
set(LIGHTER_SRC
  stdx
  debug
  debug.cpp
  file
  filex
  file.cpp
  mathx
  appx
  input
  oglbase
  ogl
  oclbase
  ocl
  cudabase
  cuda
  cudax
  pool
  pool.cpp
  img
  text
  textx
  text.glsl
  uii
  uiix
  ui
  uix
  ui.glsl
  uii.cpp
  scene
  scenex
)
if (LIGHTER_USE_OPENGL AND TARGET glew AND TARGET glfw)
  list(APPEND LIGHTER_SRC
	ogl.cpp
  )
  list(APPEND LIGHTER_DEPENDENCIES glew glfw)
endif()
if (LIGHTER_USE_FREETYPE AND TARGET freetype)
  list(APPEND LIGHTER_SRC
	ui.cpp
  )
  list(APPEND LIGHTER_DEPENDENCIES freetype)
endif()
if (LIGHTER_USE_FREEIMAGE AND TARGET freeimage)
  list(APPEND LIGHTER_SRC img.cpp)
  list(APPEND LIGHTER_DEPENDENCIES freeimage)
endif()
if (LIGHTER_USE_SCENE)
  list(APPEND LIGHTER_SRC scene.cpp)
endif()

#--------------------------------------------------------------------
# Source dependencies
#--------------------------------------------------------------------
set(LIGHTER_DATA_STAMP ${PROJECT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/lighter_data.stamp)
add_custom_command(OUTPUT ${LIGHTER_DATA_STAMP}
    COMMAND ${CMAKE_COMMAND} -E touch \"${LIGHTER_DATA_STAMP}\"
    DEPENDS ${LIGHTER_DATA_FILES})

#--------------------------------------------------------------------
# Create target
#--------------------------------------------------------------------
source_group("Source Files" FILES ${LIGHTER_SRC})
add_library(lighter ${LIGHTER_SRC} ${LIGHTER_DATA_STAMP})
target_include_directories(lighter INTERFACE ${PROJECT_SOURCE_DIR})
target_link_libraries(lighter PUBLIC ${LIGHTER_DEPENDENCIES})

#--------------------------------------------------------------------
# Install files?
#--------------------------------------------------------------------
if (LIGHTER_INSTALL_DATA_DIRECTORY)
	install (FILES ${LIGHTER_DATA_FILES} DESTINATION ${LIGHTER_INSTALL_DATA_DIRECTORY})
endif()
if (LIGHTER_INSTALL_LIB)
    install (TARGETS lighter DESTINATION lib)
endif()
