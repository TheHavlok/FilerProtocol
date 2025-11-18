add_library(CryptoLib INTERFACE)

target_include_directories(CryptoLib INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR} 
    ${Boost_INCLUDE_DIRS}   
)

target_link_libraries(CryptoLib INTERFACE Boost::headers)
