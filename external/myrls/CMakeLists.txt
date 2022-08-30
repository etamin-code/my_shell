cmake_minimum_required(VERSION 3.15)


set(PROJECT_NAME myrls)
project(${PROJECT_NAME} C CXX)

set(CMAKE_CXX_STANDARD 17)

set(WARNINGS_AS_ERRORS ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../bin)

#! Build release version if not specified otherwise.
if (NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release)
endif ()

#! Include headers
include_directories(include)

#! Project source compilation
add_executable(${PROJECT_NAME} src/main.cpp
								src/options_parser.cpp
								src/recursive_ls.cpp)

#! Add external packages
find_package(Boost 1.71.0 COMPONENTS program_options REQUIRED)
include_directories(${Boost_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME} Boost::program_options)

#! set output directory (bin)
set_target_properties(${PROJECT_NAME}
						PROPERTIES
						RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)


 
