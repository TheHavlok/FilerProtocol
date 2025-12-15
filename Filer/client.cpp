#include "NetLib/NetLib.hpp"
#include "NetLib/net_file.hpp"

int main()
{
	auto guid = guid_generator();
	std::cout << guid << std::endl;

	std::filesystem::path file = "C:\\Users\\Havlok\\Downloads\\googletest-1.17.0.tar.gz";
	std::cout << file.filename();

	boost::asio::io_context io_context;
	boost::asio::ip::tcp::resolver resolver(io_context);

	auto endpoints = resolver.resolve("192.168.0.31", "5000");
	std::shared_ptr<client> cli = std::make_shared<client>(io_context, endpoints, guid_to_string(guid));

	cli->set_send_progress_callback([](uint64_t sent, uint64_t total) {
		std::cout << "[send1] " << sent << "/" << total << " bytes\n";
		});
	cli->set_send_finished_callback([]() {
		std::cout << "[send2] finished\n";
		});
	cli->set_send_error_callback([](const std::string& err) {
		std::cerr << "[send3] error: " << err << "\n";
		});
	cli->set_recv_progress_callback([](uint64_t received, uint64_t total) {
		std::cout << "[recv1] " << received << "/" << total << " bytes\n";
		});

	cli->set_recv_finished_callback([]() {
		std::cout << "[recv2] finished\n";
		});

	std::thread t([&io_context]() { io_context.run(); });
	
	cli->start_read();

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

		cli->send_file(m, guid, string_to_guid(g));

		//file f;
		//std::vector<uint8_t> x = f.read_file(m);
		//message msg;
		//msg.encode_header(x, guid, string_to_guid(g), message_type::file);

		//cli.write(msg);

	}

	cli->close();
	t.join();

	return 0;
}
