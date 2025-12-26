#include "NetLib/NetLib.hpp"

//int main()
//{
//	boost::asio::io_context io_context;
//
//	boost::asio::ip::tcp::endpoint endpoints(boost::asio::ip::tcp::v4(), 5000);
//
//	server srv(io_context, endpoints);
//
//	io_context.run();
//}

//void server(boost::asio::io_context& io_context, unsigned short port)
//{
//	boost::asio::ip::udp::socket socket(io_context,
//		boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), port));
//	
//	std::cout << socket.local_endpoint() << std::endl;
//
//	for (;;)
//	{
//		char data[1024];
//		boost::asio::ip::udp::endpoint sender_endpoint;
//		size_t length = socket.receive_from(
//			boost::asio::buffer(data, 1024), sender_endpoint);
//		std::cout << sender_endpoint.address().to_string() << std::endl;
//		std::cout << sender_endpoint.port() << std::endl;
//		socket.send_to(boost::asio::buffer(data, length), sender_endpoint);
//
//	}
//}
//



struct ClientInfo {
    boost::asio::ip::udp::endpoint endpoint;
};

std::vector<ClientInfo> clients;

void server(boost::asio::io_context& io, unsigned short port)
{
    boost::asio::ip::udp::socket socket(
        io,
        boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), port)
    );

    for (;;)
    {
        char data[1024];
        boost::asio::ip::udp::endpoint sender;
        size_t len = socket.receive_from(
            boost::asio::buffer(data), sender);

        std::cout << "Client: "
            << sender.address().to_string()
            << ":" << sender.port() << std::endl;

        // Регистрируем клиента
        bool exists = false;
        for (auto& c : clients)
            if (c.endpoint == sender)
                exists = true;

        if (!exists)
            clients.push_back({ sender });

        // Когда есть 2 клиента — обмениваем их адресами
        if (clients.size() == 2)
        {
            auto a = clients[0].endpoint;
            auto b = clients[1].endpoint;

            std::string msgA =
                b.address().to_string() + ":" + std::to_string(b.port());
            std::string msgB =
                a.address().to_string() + ":" + std::to_string(a.port());

            socket.send_to(boost::asio::buffer(msgA), a);
            socket.send_to(boost::asio::buffer(msgB), b);

            std::cout << "Endpoints exchanged" << std::endl;
        }
    }
}

int main()
{
    boost::asio::io_context io_context;
    server(io_context, 50000);
}