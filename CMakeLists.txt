cmake_minimum_required(VERSION 3.30)
set(CMAKE_CXX_STANDARD 20)

project(Filer LANGUAGES C CXX)

set(BOOST_ROOT "C:/boost_1_89_0")
set(BOOST_INCLUDEDIR "${BOOST_ROOT}/include")
set(BOOST_LIBRARYDIR "${BOOST_ROOT}/lib")
find_package(Boost 1.89 REQUIRED)

add_subdirectory(Filer/NetLib)
add_subdirectory(Filer/CryptoLib)
add_subdirectory(Filer)

#add_executable(Filer Filer/main.cpp)