cmake_minimum_required(VERSION 2.8.12)

if(NOT CMAKE_VERSION VERSION_LESS 3.0)
    cmake_policy(SET CMP0048 NEW)
endif()

# SWIG: use standard target name.
if(POLICY CMP0078)
  cmake_policy(SET CMP0078 NEW)
endif()

# SWIG: use SWIG_MODULE_NAME property.
if(POLICY CMP0086)
  cmake_policy(SET CMP0086 NEW)
endif()

project(PythonHelloWorldExample)

FIND_PACKAGE(SWIG REQUIRED)
INCLUDE(${SWIG_USE_FILE})
SET(CMAKE_SWIG_FLAGS "")

FIND_PACKAGE(PythonLibs)
INCLUDE_DIRECTORIES(${PYTHON_INCLUDE_PATH})

if(NOT fastrtps_FOUND)
    find_package(fastrtps REQUIRED)
endif()

message(STATUS "Configuring FastDDS wrapper for Python...")
file(GLOB HELLOWORLD_EXAMPLE_SOURCES_CXX "fastdds_wrapper/*.cxx")
file(GLOB HELLOWORLD_EXAMPLE_SOURCES_CPP "fastdds_wrapper/*.cpp")

SET_SOURCE_FILES_PROPERTIES(
    swig/fastdds_wrapper.i
    ${HELLOWORLD_EXAMPLE_SOURCES_CXX}
    ${HELLOWORLD_EXAMPLE_SOURCES_CPP}
    PROPERTIES CPLUSPLUS ON)

#swig_add_library(fastdds_wrapper
#  TYPE SHARED
#  LANGUAGE python
#  OUTPUT_DIR ${CMAKE_BINARY_DIR}/python/${PYTHON_PROJECT}/Foo
#  SOURCES fastdds_wrapper.i ${HELLOWORLD_EXAMPLE_SOURCES_CXX} ${HELLOWORLD_EXAMPLE_SOURCES_CPP})

#target_include_directories(fastdds_wrapper
#  PRIVATE
#  fastdds_wrapper ${Python_INCLUDE_DIRS}
#  )

#target_link_libraries(fastdds_wrapper fastrtps)

#set_property(TARGET fastdds_wrapper PROPERTY SWIG_USE_TARGET_INCLUDE_DIRECTORIES ON)




INCLUDE_DIRECTORIES(fastdds_wrapper)

SWIG_ADD_LIBRARY(fastdds_wrapper TYPE SHARED LANGUAGE python 
    SOURCES swig/fastdds_wrapper.i ${HELLOWORLD_EXAMPLE_SOURCES_CXX} ${HELLOWORLD_EXAMPLE_SOURCES_CPP})
SWIG_LINK_LIBRARIES(fastdds_wrapper ${PYTHON_LIBRARIES} fastrtps)