#include "NetLib/NetLib.hpp"
#include "NetLib/net_file.hpp"

int main()
{
	auto guid = guid_generator();
	std::cout << guid << std::endl;

	boost::asio::io_context io_context;
	boost::asio::ip::tcp::resolver resolver(io_context);

	auto endpoints = resolver.resolve("127.0.0.1", "5000");
	client cli(io_context, endpoints, guid_to_string(guid));

	std::thread t([&io_context]() { io_context.run(); });

	while (true)
	{
		std::cout << "\n";
		std::cout << "guid: ";
		std::string g;
		std::getline(std::cin, g);

		std::cout << "\n";
		std::cout << "msgs: ";
		std::string m;
		std::getline(std::cin, m);

		file f;
		std::vector<uint8_t> x = f.read_file(m);
		message msg;
		msg.encode_header(x, guid, string_to_guid(g), message_type::file);

		cli.write(msg);

	}

	cli.close();
	t.join();

	return 0;
}
