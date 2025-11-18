#include "NetLib/NetLib.hpp"

int main()
{
	boost::asio::io_context io_context;

	boost::asio::ip::tcp::endpoint endpoints(boost::asio::ip::tcp::v4(), 5000);

	server srv(io_context, endpoints);

	io_context.run();
}
