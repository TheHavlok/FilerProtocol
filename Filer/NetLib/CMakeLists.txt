add_library(NetLib INTERFACE)

target_include_directories(NetLib INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR} 
    ${Boost_INCLUDE_DIRS}      
)

target_link_libraries(NetLib INTERFACE Boost::headers)
