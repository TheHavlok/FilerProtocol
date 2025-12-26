#pragma once

#include "net_common.hpp"
#include "net_message.hpp"
#include "net_queue.hpp"
#include "net_file.hpp"
#include "net_utils.hpp"
#include "net_logger.hpp"

using ProgressCallBack = std::function<void(uint64_t sent, uint64_t total)>;
using FinishedCallBack = std::function<void()>;
using ErrorCallBack = std::function<void(const std::string&)>;
using RecvFinishedCallBack = std::function<void()>;

class client : public std::enable_shared_from_this<client>
{
public:
	client(boost::asio::io_context& io_context,
		const boost::asio::ip::tcp::resolver::results_type& endpoints,
		std::string guid,
		std::unique_ptr<IFileReceiver> receiver,
		logger& log)
		: io_context_(io_context), socket_(io_context), guid_(guid), receiver_(std::move(receiver)), log_(log)
	{
		do_connect(endpoints);
	}

	void write(const message& msg)
	{
		if (!socket_.is_open())
		{
			log_.log("Client: error open socket");
			std::cout << "client: error\n";
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

	/*using FileChunkCallback = std::function<void(const std::vector<uint8_t>& chunk,
		uint64_t offset,
		uint64_t total_size,
		const std::string& filename)>;

	FileChunkCallback file_chunk_callback_;
	std::unordered_map<std::string, uint64_t> transfers_received_;

	void recv_file_data(const message_header& header, const uint8_t* body, size_t body_size)
	{
		if (body_size < sizeof(file_chunk_header))
		{
			return;
		}

		file_chunk_header chunk_header{};
		std::memcpy(&chunk_header, body, sizeof(file_chunk_header));
		
		const uint8_t* chunk_data = body + sizeof(file_chunk_header);
		size_t chunk_length = body_size - sizeof(file_chunk_header);

		std::vector<uint8_t> chunk_vec(chunk_data, chunk_data + chunk_length);

		std::string key = make_transfer_key(header.sender_guid, header.recipient_guid);

		transfers_received_[key] += chunk_length;
		uint64_t received = transfers_received_[key];
		uint64_t total = chunk_header.total_size;

		if (file_chunk_callback_)
			file_chunk_callback_(chunk_vec, chunk_header.offset, chunk_header.total_size, header.filename);

		if (received >= total)
		{
			if (recv_finished_callback_)
				recv_finished_callback_();

			transfers_received_.erase(key);
		}

		if (recv_progress_callback_)
			recv_progress_callback_(received, total);
	}*/

	void send_file_data(const std::vector<uint8_t> chunk, 
		std::shared_ptr<uint64_t> sent_ptr, 
		const uint64_t total,
		const boost::uuids::uuid& sender_guid, 
		const boost::uuids::uuid& recipient_guid,
		const std::string& filename,
		std::function<void()> next_chunk_callback = nullptr)
	{
		if (*sent_ptr >= total)
			if (send_finished_callback_)
				send_finished_callback_();

		auto self(shared_from_this());

		size_t bytes_read = chunk.size();

		file_chunk_header chunk_header;
		chunk_header.total_size = total;
		chunk_header.offset = static_cast<uint64_t>(*sent_ptr);


		std::vector<uint8_t> payload;
		payload.reserve(sizeof(file_chunk_header) + static_cast<size_t>(bytes_read));

		const uint8_t* ph = reinterpret_cast<const uint8_t*>(&chunk_header);
		payload.insert(payload.end(), ph, ph + sizeof(file_chunk_header));
		payload.insert(payload.end(), chunk.begin(), chunk.begin() + bytes_read);

		message msg;
		msg.encode_header(payload, sender_guid, recipient_guid, message_type::file, filename);

		boost::asio::post(io_context_,
			[this, self, msg = std::move(msg), sent_ptr, bytes_read, total, next_chunk_callback]() mutable
			{
				bool write_in_progress = !write_msgs_.empty();
				write_msgs_.push_back(msg);
					
				*sent_ptr += bytes_read;
				if (send_progress_callback_)
					send_progress_callback_(*sent_ptr, total);

				if (!write_in_progress)
				{
					do_write(next_chunk_callback);
				}

			});
	}

	void send_file(const std::string& file_path,
		const boost::uuids::uuid& sender_guid,
		const boost::uuids::uuid& recipient_guid)
	{

		auto file = std::make_shared<std::ifstream>(file_path, std::ios::binary);
		if (!file || !file->is_open())
		{
			if (send_error_callback_)
				send_error_callback_("error");
			return;
		}

		size_t chunk_size = 1024; // *1024 * 10; // 1024 = 1 KB 1024 * 1024 = 1MB
		uint64_t total_size = std::filesystem::file_size(file_path);
		auto sent_bytes = std::make_shared<uint64_t>(0ull);

		std::filesystem::path filename = file_path;

		auto self(shared_from_this());
		auto push_chunk = std::make_shared<std::function<void()>>();
		*push_chunk = [this, self, file, sender_guid, recipient_guid, total_size, sent_bytes, chunk_size, push_chunk, filename]() mutable
			{
				if (!file->good())
				{
					if (send_finished_callback_)
						send_finished_callback_();
					return;
				}

				std::vector<uint8_t> buffer(chunk_size);
				file->read(reinterpret_cast<char*>(buffer.data()), buffer.size());
				std::streamsize bytes_read = file->gcount();

				if (bytes_read <= 0)
				{
					if (send_finished_callback_)
						send_finished_callback_();
					return;
				}

				file_chunk_header chunk_header;
				chunk_header.total_size = total_size;
				chunk_header.offset = *sent_bytes;

				std::vector<uint8_t> payload;
				payload.reserve(sizeof(file_chunk_header) + static_cast<size_t>(bytes_read));

				const uint8_t* ph = reinterpret_cast<const uint8_t*>(&chunk_header);
				payload.insert(payload.end(), ph, ph + sizeof(file_chunk_header));
				payload.insert(payload.end(), buffer.begin(), buffer.begin() + bytes_read);

				
				message msg;
				msg.encode_header(payload, sender_guid, recipient_guid, message_type::file, filename.filename().string());

				boost::asio::post(io_context_,
					[this, self, msg = std::move(msg), push_chunk, sent_bytes, bytes_read, total_size]()
					{
						bool write_in_progress = !write_msgs_.empty();
						write_msgs_.push_back(msg);

						*sent_bytes += static_cast<uint64_t>(bytes_read);

						if (send_progress_callback_)
							send_progress_callback_(*sent_bytes, total_size);

						if (!write_in_progress)
						{
							do_write([push_chunk]() { (*push_chunk)(); });
						}
					});
			};
		boost::asio::post(io_context_, [push_chunk]() { (*push_chunk)(); });
	}

	void auth(std::string auth)
	{
		auto auth_ptr = std::make_shared<std::string>(std::move(auth));
		boost::asio::async_write(socket_, boost::asio::buffer(auth_ptr->data(), auth_ptr->size()),
			[this](boost::system::error_code ec, std::size_t)
			{
				if (ec)
				{
					log_.log("Client: connect error");
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
				socket_connected_ = false;
			});
	}

	void start_read()
	{
		auto self = shared_from_this();
		
		if (socket_connected_.load())
		{
			boost::asio::post(io_context_,
				[this, self]()
				{
					do_read_header();
				});
			return;
		}
		
		std::lock_guard<std::mutex> lg(on_connect_mutex_);
		on_connect_callback_.push_back([this, self]()
			{
				do_read_header();
			});
	}

	std::vector<std::function<void()>> on_connect_callback_;
	std::mutex on_connect_mutex_;
	std::atomic<bool> socket_connected_{ false };

	bool setOsAndroid = false;

	void set_send_progress_callback(ProgressCallBack cb) { send_progress_callback_ = std::move(cb); }
	void set_send_finished_callback(FinishedCallBack cb) { send_finished_callback_ = std::move(cb); }
	void set_send_error_callback(ErrorCallBack cb) { send_error_callback_ = std::move(cb); }
	void set_recv_progress_callback(ProgressCallBack cb) { recv_progress_callback_ = std::move(cb); }
	void set_recv_finished_callback(RecvFinishedCallBack cb) { recv_finished_callback_ = std::move(cb); }

private:
	void do_connect(const boost::asio::ip::tcp::resolver::results_type& endpoints)
	{
		log_.log("Client: starting async_connect...");
		std::cout << "client: starting async_connect...\n";

		boost::asio::async_connect(socket_, endpoints,
			[this](boost::system::error_code ec, boost::asio::ip::tcp::endpoint endpoints)
			{
				if (!ec)
				{
					auth(guid_);
					//do_read_header();

					socket_connected_.store(true);
					std::vector<std::function<void()>> callbacks;
					{
						std::lock_guard<std::mutex> lg(on_connect_mutex_);
						callbacks.swap(on_connect_callback_);
					}

					for (auto& cb : callbacks)
					{
						boost::asio::post(io_context_, std::move(cb));
					}
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

	/*void recv_file(const message_header& header, const uint8_t* body, size_t body_size)
	{
		if (body_size < sizeof(file_chunk_header))
		{
			return;
		}

		std::string filename = header.filename;

		file_chunk_header chunk_header{};
		std::memcpy(&chunk_header, body, sizeof(file_chunk_header));

		uint8_t* chunk_data = const_cast<uint8_t*>(body) + sizeof(file_chunk_header);
		size_t chunk_length = body_size - sizeof(file_chunk_header);
		
		std::string key = make_transfer_key(header.sender_guid, header.recipient_guid);

		auto it = transfers_.find(key);
		if (it == transfers_.end())
		{
			transfer_state ts;
			ts.total_size = chunk_header.total_size;
			ts.received = 0;
			ts.tmp_path = "recv_" + guid_to_string(read_msg_.header_.sender_guid) + "_" + guid_to_string(read_msg_.header_.recipient_guid);

			ts.out.open(ts.tmp_path, std::ios::binary | std::ios::out | std::ios::trunc);
			if (!ts.out.is_open())
			{
				ts.out.clear();
				ts.out.open(ts.tmp_path, std::ios::binary | std::ios::trunc);
			}
			it = transfers_.emplace(key, std::move(ts)).first;
		}

		transfer_state& st = it->second;
		st.out.seekp(static_cast<std::streamoff>(chunk_header.offset), std::ios::beg);
		st.out.write(reinterpret_cast<const char*>(chunk_data), static_cast<std::streamsize>(chunk_length));
		st.out.flush();

		st.received += static_cast<uint64_t>(chunk_length);

		if (recv_progress_callback_)
			recv_progress_callback_(st.received, st.total_size);

		if (st.received >= st.total_size)
		{
			st.out.close();
			std::string final_name = filename;
			std::error_code ercf;
			std::filesystem::rename(st.tmp_path, final_name, ercf);

			if (ercf)
				std::cout << "error rename";

			if (recv_finished_callback_)
			{
				recv_finished_callback_();
			}

			transfers_.erase(it);
		}
	}*/

	void do_read_body()
	{
		boost::asio::async_read(socket_, boost::asio::buffer(read_msg_.body(), read_msg_.body_size()),
			[this](boost::system::error_code ec, std::size_t)
			{
				if (!ec)
				{
					if (read_msg_.header_.msg_type == message_type::file)
					{
						//file f;
						//f.write_file("2.exe", read_msg_.body_to_file(read_msg_.body(), read_msg_.body_size()));
						/*if (setOsAndroid)
							recv_file_data(read_msg_.header_, read_msg_.body(), read_msg_.body_size());
						else
							recv_file(read_msg_.header_, read_msg_.body(), read_msg_.body_size());*/

						receiver_->recv_file(read_msg_.header_,
							read_msg_.body(),
							read_msg_.body_size(),
							recv_progress_callback_,
							recv_finished_callback_);
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

	void do_write(std::function<void()> next_chunk_callback = nullptr)
	{
		auto self(shared_from_this());
		boost::asio::async_write(socket_, boost::asio::buffer(write_msgs_.front().data(), write_msgs_.front().size()),
			[this, self, next_chunk_callback](boost::system::error_code ec, std::size_t length)
			{
				if (!ec)
				{
					write_msgs_.pop_front();
					if (!write_msgs_.empty())
					{
						do_write(next_chunk_callback);
					}
					else if (next_chunk_callback)
					{
						next_chunk_callback();
					}
				}
				else
				{
					socket_.close();
				}
			});
	}

	/*void do_write()
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
	}*/

	std::string make_transfer_key (const boost::uuids::uuid& sender_guid, const boost::uuids::uuid& recipient_guid)
	{
		return guid_to_string(sender_guid) + "_" + guid_to_string(recipient_guid);
	}

private:
	boost::asio::io_context& io_context_;
	boost::asio::ip::tcp::socket socket_;
	message read_msg_;
	queue<message> write_msgs_;

	logger& log_;

	std::string guid_;

	std::unordered_map<std::string, transfer_state> transfers_;

	ProgressCallBack send_progress_callback_;
	FinishedCallBack send_finished_callback_;
	ErrorCallBack send_error_callback_;
	ProgressCallBack recv_progress_callback_;
	RecvFinishedCallBack recv_finished_callback_;


	std::unique_ptr<IFileReceiver> receiver_;
};
