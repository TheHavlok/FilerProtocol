# Добавляем сервер
add_executable(Server server.cpp)
target_link_libraries(Server
    PRIVATE NetLib CryptoLib
)

# Добавляем клиент
add_executable(Client client.cpp)
target_link_libraries(Client
    PRIVATE NetLib CryptoLib
)
