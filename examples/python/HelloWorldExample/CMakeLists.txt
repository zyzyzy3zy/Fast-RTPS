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

###############################################################################
# Dependencies
###############################################################################

FIND_PACKAGE(SWIG REQUIRED)
INCLUDE(${SWIG_USE_FILE})
SET(CMAKE_SWIG_FLAGS "")

FIND_PACKAGE(PythonLibs)
INCLUDE_DIRECTORIES(${PYTHON_INCLUDE_PATH})

if(NOT fastrtps_FOUND)
    find_package(fastrtps REQUIRED)
endif()

###############################################################################
# Compile python wrapper with SWIG
###############################################################################

message(STATUS "Configuring FastDDS wrapper for Python...")
file(GLOB HELLOWORLD_EXAMPLE_SOURCES_CXX "fastdds_wrapper/*.cxx")
file(GLOB HELLOWORLD_EXAMPLE_SOURCES_CPP "fastdds_wrapper/*.cpp")

SET_SOURCE_FILES_PROPERTIES(
    swig/fastdds_wrapper.i
    ${HELLOWORLD_EXAMPLE_SOURCES_CXX}
    ${HELLOWORLD_EXAMPLE_SOURCES_CPP}
    PROPERTIES CPLUSPLUS ON)

INCLUDE_DIRECTORIES(fastdds_wrapper)

SWIG_ADD_LIBRARY(fastdds_wrapper 
    TYPE SHARED
    LANGUAGE python 
    SOURCES swig/fastdds_wrapper.i ${HELLOWORLD_EXAMPLE_SOURCES_CXX} ${HELLOWORLD_EXAMPLE_SOURCES_CPP})
SWIG_LINK_LIBRARIES(fastdds_wrapper ${PYTHON_LIBRARIES} fastrtps)


###############################################################################
# Copy python files
###############################################################################

configure_file(${PROJECT_SOURCE_DIR}/HelloWorldExample.py
    ${CMAKE_CURRENT_BINARY_DIR}/HelloWorldExample.py COPYONLY)

#install(DIRECTORY ${PROJECT_SOURCE_DIR}/
#    DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/
#    USE_SOURCE_PERMISSIONS
#    FILES_MATCHING 
#        PATTERN "*.py"
#        PATTERN "__pycache__" EXCLUDE
#)

