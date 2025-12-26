#include "NetLib/NetLib.hpp"
#include "NetLib/net_file.hpp"

//int main()
//{
//	const size_t MAX_PAYLOAD = 14;
//	std::vector<uint8_t> data;
//	std::string text;
//	std::getline(std::cin, text);
//
//	data = static_cast<uint8_t>(text.c_str());
//
//	int x = data.size() / MAX_PAYLOAD;
//	int y = data.size() % MAX_PAYLOAD;
//
//	auto guid = guid_generator();
//	std::cout << guid << std::endl;
//
//	std::filesystem::path file = "C:\\Users\\Havlok\\Downloads\\googletest-1.17.0.tar.gz";
//	std::cout << file.filename();
//
//	logger log("logger.log");
//	auto receiver = std::make_unique<DesktopFileReceiver>();
//
//	boost::asio::io_context io_context;
//	boost::asio::ip::tcp::resolver resolver(io_context);
//
//	auto endpoints = resolver.resolve("192.168.0.31", "5000");
//	std::shared_ptr<client> cli = std::make_shared<client>(io_context, endpoints, guid_to_string(guid), std::move(receiver), log);
//
//	cli->set_send_progress_callback([&log](uint64_t sent, uint64_t total) {
//		std::cout << "[send1] " << sent << "/" << total << " bytes\n";
//		log.log("test");
//		});
//	cli->set_send_finished_callback([]() {
//		std::cout << "[send2] finished\n";
//		});
//	cli->set_send_error_callback([](const std::string& err) {
//		std::cerr << "[send3] error: " << err << "\n";
//		});
//	cli->set_recv_progress_callback([](uint64_t received, uint64_t total) {
//		std::cout << "[recv1] " << received << "/" << total << " bytes\n";
//		});
//
//	cli->set_recv_finished_callback([]() {
//		std::cout << "[recv2] finished\n";
//		});
//
//	std::thread t([&io_context]() { io_context.run(); });
//	
//	cli->start_read();
//
//	while (true)
//	{
//		std::cout << "\n";
//		std::cout << "guid: ";
//		std::string g;
//		std::getline(std::cin, g);
//
//		std::cout << "\n";
//		std::cout << "msgs: ";
//		std::string m;
//		std::getline(std::cin, m);
//
//		cli->send_file(m, guid, string_to_guid(g));
//
//		//file f;
//		//std::vector<uint8_t> x = f.read_file(m);
//		//message msg;
//		//msg.encode_header(x, guid, string_to_guid(g), message_type::file);
//
//		//cli.write(msg);
//
//	}
//
//	cli->close();
//	t.join();
//
//	return 0;
//}


//int main()
//{
//	boost::asio::io_context io_context;
//
//	boost::asio::ip::udp::socket socket(io_context,
//		boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), 0));
//
//	boost::asio::ip::udp::resolver resolver(io_context);
//
//	boost::asio::ip::udp::resolver::results_type endpoints =
//		resolver.resolve(boost::asio::ip::udp::v4(), "127.0.0.1", "50000");
//
//	
//		char request[1024];
//		std::cin.getline(request, 1024);
//		size_t request_length = std::strlen(request);
//		socket.send_to(boost::asio::buffer(request, request_length), *endpoints.begin());
//
//		char reply[1024];
//		boost::asio::ip::udp::endpoint sender_endpoint;
//		size_t reply_length = socket.receive_from(
//			boost::asio::buffer(reply, 1024), sender_endpoint);
//		std::cout << "reply is: ";
//		std::cout.write(reply, reply_length);
//		std::cout << "\n";
//
//
//}

using boost::asio::ip::udp;

int main()
{
    try
    {
        boost::asio::io_context io;

        udp::socket socket(io, udp::endpoint(udp::v4(), 0));

        // Адрес rendezvous-сервера
        udp::endpoint server_endpoint(
            boost::asio::ip::make_address("5.144.176.105"), 50000
        );

        // Регистрация на сервере
        std::string hello = "hello";
        socket.send_to(boost::asio::buffer(hello), server_endpoint);
        std::cout << "Registered on server" << std::endl;

        // Получаем адрес peer'а
        char buffer[1024];
        udp::endpoint from;
        size_t len = socket.receive_from(boost::asio::buffer(buffer), from);

        std::string peer_info(buffer, len);
        std::cout << "Peer info: " << peer_info << std::endl;

        auto pos = peer_info.find(':');
        if (pos == std::string::npos)
        {
            std::cerr << "Invalid peer format" << std::endl;
            return 1;
        }

        std::string peer_ip = peer_info.substr(0, pos);
        unsigned short peer_port = static_cast<unsigned short>(
            std::stoi(peer_info.substr(pos + 1))
            );

        udp::endpoint peer_endpoint(boost::asio::ip::make_address(peer_ip), peer_port);
        std::cout << "Start hole punching to " << peer_ip << ":" << peer_port << std::endl;

        // Поток для получения сообщений
        std::thread receiver([&socket]() {
            for (;;)
            {
                char data[1024];
                udp::endpoint sender;
                try {
                    size_t length = socket.receive_from(boost::asio::buffer(data), sender);
                    std::cout << "\n[From " << sender.address().to_string()
                        << ":" << sender.port() << "] ";
                    std::cout.write(data, length);
                    std::cout << std::endl << "> " << std::flush;
                }
                catch (std::exception& e) {
                    std::cerr << "Receive error: " << e.what() << std::endl;
                }
            }
            });

        // Поток для отправки сообщений
        std::thread sender_thread([&socket, &peer_endpoint]() {
            for (;;)
            {
                std::cout << "> " << std::flush;
                std::string msg;
                std::getline(std::cin, msg);
                if (msg.empty()) continue;
                socket.send_to(boost::asio::buffer(msg), peer_endpoint);
            }
            });

        // Поток для периодического "удержания NAT" (keep-alive)
        std::thread keep_alive([&socket, &peer_endpoint]() {
            while (true)
            {
                std::this_thread::sleep_for(std::chrono::seconds(10));
                std::string ping = "ping";
                socket.send_to(boost::asio::buffer(ping), peer_endpoint);
            }
            });

        receiver.join();
        sender_thread.join();
        keep_alive.join();
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
}