#pragma once 

#include "net_common.hpp"

enum class message_type : uint16_t
{
	text,
	file,
	auth,
};

#pragma pack(push, 1)
struct file_chunk_header
{
	uint64_t total_size;
	uint64_t offset;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct message_header					// 48 bytes // 42 bytes
{
	uint64_t body_size;					// 8 bytes
	boost::uuids::uuid sender_guid;		// 16 bytes
	boost::uuids::uuid recipient_guid;	// 16 bytes
	message_type msg_type;				// 2 bytes
	char filename[255];
};
#pragma pack(pop)

class message
{
public:
	message()
	{
		data_.resize(sizeof(message_header));
	}

	const uint8_t* data() const
	{
		return data_.data();
	}

	uint8_t* data()
	{
		return data_.data();
	}

	const uint8_t* body() const
	{
		return data_.data() + sizeof(message_header);
	}

	uint8_t* body()
	{
		return data_.data() + sizeof(message_header);
	}

	std::vector<uint8_t> body_to_file(uint8_t* data, std::size_t size)
	{
		std::vector<uint8_t> tmp;
		tmp.insert(tmp.end(), data, data + size);

		return tmp;
	}

	std::size_t body_size() const
	{
		return header_.body_size;
	}

	std::size_t size() const
	{
		return sizeof(header_) + header_.body_size;
	}

	void prepare_for_header()
	{
		data_.resize(sizeof(message_header));
		header_ = message_header{};
	}

	void encode_header(std::string& msg, 
		boost::uuids::uuid sender_guid, 
		boost::uuids::uuid recipient_guid, 
		message_type msg_type)
	{
		header_.body_size = msg.size();
		header_.sender_guid = sender_guid;
		header_.recipient_guid = recipient_guid;
		header_.msg_type = msg_type;

		data_.resize(sizeof(message_header) + header_.body_size);
		std::memcpy(data_.data(), &header_, sizeof(header_));
		std::memcpy(data_.data() + sizeof(header_), msg.data(), header_.body_size);
	}

	void encode_header(std::vector<uint8_t>& msg, 
		boost::uuids::uuid sender_guid, 
		boost::uuids::uuid recipient_guid, 
		message_type msg_type,
		const std::string& filename = "")
	{
		header_.body_size = msg.size();
		header_.sender_guid = sender_guid;
		header_.recipient_guid = recipient_guid;
		header_.msg_type = msg_type;

		if (msg_type == message_type::text)
		{

		}
		else if (msg_type == message_type::file)
		{
			std::strncpy(header_.filename, filename.c_str(), sizeof(header_.filename) - 1);
			header_.filename[sizeof(header_.filename) - 1] = '\0';
		}

		data_.resize(sizeof(message_header) + header_.body_size);
		std::memcpy(data_.data(), &header_, sizeof(header_));
		std::memcpy(data_.data() + sizeof(header_), msg.data(), header_.body_size);
	}

	bool decode_header()
	{
		//std::cout << data_.size() << " " << sizeof(header_) << std::endl;
		if (data_.size() < sizeof(header_))
			return false;


		std::memcpy(&header_, data_.data(), sizeof(message_header));

		//const uint64_t MAX_BODY = 1024 * 1024; // 1 MB
		//if (header_.body_size > MAX_BODY)
		//{
		//	std::cerr << "decode_header: unreasonable body_size = " << header_.body_size << "\n";
		//	return false;
		//}

		data_.resize(sizeof(message_header) + header_.body_size);
		return true;
	}

public:
	message_header header_{};
	std::vector<uint8_t> data_;
};
