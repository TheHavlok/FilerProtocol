#pragma once

#include "net_common.hpp"
#include "net_message.hpp"
#include "net_queue.hpp"
#include "net_file.hpp"

class client
{
public:
	client(boost::asio::io_context& io_context, const boost::asio::ip::tcp::resolver::results_type& endpoints, std::string guid)
		:io_context_(io_context), socket_(io_context), guid_(guid)
	{
		do_connect(endpoints);
	}

	void write(const message& msg)
	{
		if (!socket_.is_open())
		{
			std::cout << "client: попытка записи Ч сокет не открыт, сообщение отброшено\n";
			return;
		}
		boost::asio::post(io_context_,
			[this, msg]()
			{
				bool write_in_progress = !write_msgs_.empty();
				write_msgs_.push_back(msg);
				if (!write_in_progress)
				{
					do_write();
				}
			});
	}

	void auth(std::string auth)
	{
		auto auth_ptr = std::make_shared<std::string>(std::move(auth));
		boost::asio::async_write(socket_, boost::asio::buffer(auth_ptr->data(), auth_ptr->size()),
			[this](boost::system::error_code ec, std::size_t)
			{
				if (ec)
				{
					std::cout << "connect error\n";
				}
			});
	}

	void close()
	{
		boost::asio::post(io_context_, 
			[this]() 
			{ 
				socket_.close(); 
			});
	}

private:
	void do_connect(const boost::asio::ip::tcp::resolver::results_type& endpoints)
	{
		std::cout << "client: starting async_connect...\n";

		boost::asio::async_connect(socket_, endpoints,
			[this](boost::system::error_code ec, boost::asio::ip::tcp::endpoint endpoints)
			{
				if (!ec)
				{
					auth(guid_);
					do_read_header();
				}

				else
				{
					std::cout << "client: connect failed: " << ec.message() << " (" << ec.value() << ")\n";
				}
			});
	}

	void do_read_header()
	{
		read_msg_.prepare_for_header();
		boost::asio::async_read(socket_, boost::asio::buffer(read_msg_.data(), sizeof(message_header)),
			[this](boost::system::error_code ec, std::size_t)
			{
				if (!ec && read_msg_.decode_header())
				{
					//std::cout << "Got header: body_size=" << read_msg_.header_.body_size
					//	<< " msg_type=" << static_cast<int>(read_msg_.header_.msg_type) << "\n";
					do_read_body();
				}
				else
				{
					if (ec) std::cout << "client: read_header error: " << ec.message() << " (" << ec.value() << ")\n";
					else std::cout << "client: decode_header returned false\n";
					socket_.close();
				}
			});
	}

	void do_read_body()
	{
		boost::asio::async_read(socket_, boost::asio::buffer(read_msg_.body(), read_msg_.body_size()),
			[this](boost::system::error_code ec, std::size_t)
			{
				if (!ec)
				{
					if (read_msg_.header_.msg_type == message_type::file)
					{
						file f;
						f.write_file("2.exe", read_msg_.body_to_file(read_msg_.body(), read_msg_.body_size()));
					}
					else if (read_msg_.header_.msg_type == message_type::text)
					{
						std::cout << "incoming: ";
						std::cout.write((char*)read_msg_.body(), read_msg_.body_size());
						std::cout << "\n";
					}
					
					do_read_header();
				}
				else
				{
					if (ec) std::cout << "client: read_header error: " << ec.message() << " (" << ec.value() << ")\n";
					else std::cout << "client: decode_header returned false\n";
					socket_.close();
				}
			});
	}

	void do_write()
	{
		boost::asio::async_write(socket_, boost::asio::buffer(write_msgs_.front().data(), write_msgs_.front().size()),
			[this](boost::system::error_code ec, std::size_t)
			{
				if (!ec)
				{
					write_msgs_.pop_front();
					if (!write_msgs_.empty())
					{
						do_write();
					}
				}
				else
				{
					socket_.close();
				}
			});
	}

private:
	boost::asio::io_context& io_context_;
	boost::asio::ip::tcp::socket socket_;
	message read_msg_;
	queue<message> write_msgs_;

	std::string guid_;
};
