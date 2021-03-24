cmake_minimum_required(VERSION 3.16 FATAL_ERROR)
project(ri VERSION 0.1.0 LANGUAGES CXX)

# Sources
file(GLOB_RECURSE sources src/*.[hc]pp)

# Executable
add_executable(${PROJECT_NAME} ${sources} src/main.manifest)
target_precompile_headers(${PROJECT_NAME} PRIVATE src/main.hpp)
target_compile_definitions(${PROJECT_NAME} PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)
target_include_directories(${PROJECT_NAME} PRIVATE src tmp)

# Dependencies
find_package(Boost REQUIRED COMPONENTS headers regex)
target_link_libraries(${PROJECT_NAME} PRIVATE Boost::headers Boost::regex)

find_package(OpenSSL REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE OpenSSL::Crypto OpenSSL::SSL)

find_package(LibXml2 REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE LibXml2::LibXml2)

find_package(Threads REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE Threads::Threads)

if(NOT WIN32)
  target_link_libraries(${PROJECT_NAME} PRIVATE dl)
endif()
