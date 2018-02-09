set_property(GLOBAL PROPERTY USE_FOLDERS On)
cmake_minimum_required(VERSION 2.8.3) 

project(gvdbviewer)

message("CMAKE_SYSTEM_PROCESSOR: ${CMAKE_SYSTEM_PROCESSOR}")

# FIND OPENGL
find_package(OpenGL REQUIRED)
include_directories(${OpenGL_INCLUDE_DIRS})
link_directories(${OpenGL_LIBRARY_DIRS})
add_definitions(${OpenGL_DEFINITIONS})
if(NOT OPENGL_FOUND)
    message(ERROR " OPENGL not found!")
endif(NOT OPENGL_FOUND)


# Set output paths
# set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
# set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

include_directories(
	external/glfw-3.0.3/include/GLFW/
	external/glm-0.9.4.0/
	external/glew-1.9.0/include/
	../
	.
)

# GVDB
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/../cmake")
message(${CMAKE_MODULE_PATH})
find_package(GVDB REQUIRED)
if ( GVDB_FOUND )
    message( STATUS "--> Using package GVDB (inc: ${GVDB_INCLUDE_DIR}) ")    
    include_directories( ${GVDB_INCLUDE_DIR} )
    link_directories( ${GVDB_LIB_DIR})
    add_definitions(-DUSE_GVDB)
    if(WIN32)
      LIST(APPEND LIBRARIES_OPTIMIZED ${GVDB_LIB_DIR}/${GVDB_LIB} )
      LIST(APPEND LIBRARIES_DEBUG ${GVDB_LIB_DIR}/${GVDB_LIB} )
    endif(WIN32)
    LIST(APPEND PACKAGE_SOURCE_FILES ${GVDB_INCLUDE_DIR}/${GVDB_HEADERS} )  	
    source_group(GVDB FILES ${GVDB_INCLUDE_DIR}/${GVDB_HEADERS} ) 
else()
	message( FATAL_ERROR "--> Unable to find GVDB") 
endif()


set(ALL_LIBS
	${OPENGL_LIBRARY}
	GLFW_303
	GLEW_190
	${GVDB_LIB}
)

add_definitions(
	-DTW_STATIC
	-DTW_NO_LIB_PRAGMA
	-DTW_NO_DIRECT3D
	-DGLEW_STATIC
	-D_CRT_SECURE_NO_WARNINGS
)

# Compile external dependencies 
add_subdirectory (external)

add_definitions(-DSTANDALONE_APP)

# Main app
SET( srcs 
		../SparseVol.cpp
		GLUtils.cpp
		Camera.cpp
		main.cpp 
		)

# Headers
SET( headers
		../SparseVol.h
		GLUtils.h
		Camera.h
		)

#add_definitions(-DCMAKE_BUILD_TYPE=Debug -DPRINT_DEBUG)
#add_definitions(-DSHADER_DIR=\"${CMAKE_SOURCE_DIR}/../gigapoint_resource/shaders/\")

add_executable(gvdbviewer ${srcs} ${headers})

# create the program
target_link_libraries(gvdbviewer ${ALL_LIBS} )

source_group("app" FILES Camera.h Camera.cpp GLInlcude.h nuklear.h nuklear_glfw_gl2.h GLUtils.h GLUtils.cpp Mesh.h Mesh.cpp main.cpp)
