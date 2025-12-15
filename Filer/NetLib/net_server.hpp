#pragma once

#include "net_common.hpp"
#include "net_message.hpp"
#include "net_queue.hpp"
#include "net_utils.hpp"

class session;

class connections
{
public:
	inline void connect(const boost::uuids::uuid& guid, std::shared_ptr<session> s);

	inline void is_connected();

	inline void send(const message& msg);

	inline std::vector<std::shared_ptr<session>> get_session(const boost::uuids::uuid& guid) const;

	inline void close(const boost::uuids::uuid& guid, std::shared_ptr<session> s);

private:
	std::unordered_map<boost::uuids::uuid, std::vector<std::shared_ptr<session>>> map_;
};


class session : public std::enable_shared_from_this<session>
{
public:
	inline session(boost::asio::ip::tcp::socket socket, connections& connection);

	inline void start();

	inline void deliver(const message& msg);

private:
	inline void auth();

	inline void do_read_header();

	inline void do_read_body();

	inline void do_write();

private:
	boost::asio::ip::tcp::socket socket_;
	message read_msg_;
	queue<message> write_msgs_;
	boost::uuids::uuid user_uuid_;

	connections& connection_;

	std::array<char, 36> auth_;
};

inline void connections::connect(const boost::uuids::uuid& guid, std::shared_ptr<session> s)
{
	map_[guid].push_back(s);
}

inline void connections::send(const message& msg)
{
	auto recipient = msg.header_.recipient_guid;
	auto dests = connections::get_session(recipient);

	for (auto& dest : dests)
		dest->deliver(msg);
}

inline std::vector<std::shared_ptr<session>> connections::get_session(const boost::uuids::uuid& guid) const
{
	auto item = map_.find(guid);
	if (item != map_.end())
		return item->second;
	else
		return {};
}

inline void connections::close(const boost::uuids::uuid& guid, std::shared_ptr<session> s)
{
	auto item = map_.find(guid);
	if (item != map_.end())
	{
		auto& vec = item->second;
		vec.erase(std::remove(vec.begin(), vec.end(), s), vec.end());
		if (vec.empty())
		{
			map_.erase(item);
		}
	}
}

inline session::session(boost::asio::ip::tcp::socket socket, connections& connection)
	: socket_(std::move(socket)), connection_(connection)
{
}

inline void session::start()
{
	//user_connection_.connect();
	//do_read_header();
	auth();
}

inline void session::auth()
{
	auto self(shared_from_this());
	boost::asio::async_read(socket_, boost::asio::buffer(auth_),
		[this, self](boost::system::error_code ec, std::size_t)
		{
			if (!ec)
			{
				std::string auth_str(auth_.data(), auth_.size());
				auto guid = string_to_guid(auth_str);
				user_uuid_ = guid;
				connection_.connect(guid, shared_from_this());
				do_read_header();
			}
		});
}

inline void session::deliver(const message& msg)
{
	bool write_in_progress = !write_msgs_.empty();
	write_msgs_.push_back(msg);
	if (!write_in_progress)
		do_write();
}

inline void session::do_read_header()
{
	auto self(shared_from_this());
	boost::asio::async_read(socket_, boost::asio::buffer(read_msg_.data(), sizeof(message_header)),
		[this, self](boost::system::error_code ec, std::size_t length)
		{
			std::cout << "header cb: ec=" << ec.message() << ", len=" << length << "\n";

			if (!ec && read_msg_.decode_header())
			{
				std::cout << "Got header: body_size=" << read_msg_.header_.body_size
					<< " msg_type=" << static_cast<int>(read_msg_.header_.msg_type) << "\n";
				do_read_body();
			}
			else
			{
				connection_.close(user_uuid_, shared_from_this());
			}
		});
}

inline void session::do_read_body()
{
	auto self(shared_from_this());
	boost::asio::async_read(socket_, boost::asio::buffer(read_msg_.body(), read_msg_.body_size()),
		[this, self](boost::system::error_code ec, std::size_t length)
		{
			std::cout << "body cb: ec=" << ec.message() << ", len=" << length << "\n";
			if (!ec)
			{
				connection_.send(read_msg_);
				do_read_header();
			}
			else
			{
				connection_.close(user_uuid_, shared_from_this());
			}
		});
}

inline void session::do_write()
{
	auto self(shared_from_this());
	boost::asio::async_write(socket_, boost::asio::buffer(write_msgs_.front().data(), write_msgs_.front().size()),
		[this, self](boost::system::error_code ec, std::size_t)
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
				connection_.close(user_uuid_, shared_from_this());
			}
		});
}

class server
{
public:
	server(boost::asio::io_context& io_context, const boost::asio::ip::tcp::endpoint& endpoint)
		: acceptor_(io_context, endpoint)
	{
		do_accept();
	}

private:
	void do_accept()
	{
		acceptor_.async_accept([this](boost::system::error_code ec, boost::asio::ip::tcp::socket socket)
			{
				if (!ec)
				{
					std::make_shared<session>(std::move(socket), connection_)->start();
				}

				do_accept();
			});
	}

private:
	boost::asio::ip::tcp::acceptor acceptor_;
	connections connection_;
};
