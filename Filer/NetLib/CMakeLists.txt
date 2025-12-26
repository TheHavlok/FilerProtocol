cmake_minimum_required(VERSION 3.30)
set(CMAKE_CXX_STANDARD 20)

project(NetLib LANGUAGES C CXX)

# --- пути к Boost в зависимости от целевой системы ---
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(BOOST_ROOT "C:/boost_1_89_0")
    set(BOOST_INCLUDEDIR "${BOOST_ROOT}/include")
    set(BOOST_LIBRARYDIR "${BOOST_ROOT}/lib")

    # Ищем Boost стандартным образом
    find_package(Boost 1.89 REQUIRED)

    add_library(NetLib INTERFACE)

    target_include_directories(NetLib INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${Boost_INCLUDE_DIRS}
    )

    target_link_libraries(NetLib INTERFACE Boost::headers ws2_32 Mswsock iphlpapi)

elseif(CMAKE_SYSTEM_NAME STREQUAL "Android")
    set(BOOST_ROOT "C:/boost_android/boost_1_89_0")
    set(BOOST_INCLUDEDIR "${BOOST_ROOT}")
    set(BOOST_LIBRARYDIR "C:/boost_android/build/out/arm64-v8a/lib")

    add_library(NetLib INTERFACE)

    target_include_directories(NetLib INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${BOOST_INCLUDEDIR}
    )

elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux" OR CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(BOOST_ROOT "/usr/local/boost_1_89_0")
    set(BOOST_INCLUDEDIR "${BOOST_ROOT}/include")
    set(BOOST_LIBRARYDIR "${BOOST_ROOT}/lib")

    find_package(Boost 1.89 REQUIRED COMPONENTS)

    add_library(NetLib INTERFACE)

    target_include_directories(NetLib INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${Boost_INCLUDE_DIRS}
    )

    target_link_libraries(NetLib INTERFACE Boost::headers)
endif()

# --- добавляем заголовки NetLib ---
file(GLOB NETLIB_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp"
)
target_sources(NetLib INTERFACE ${NETLIB_HEADERS})
