#pragma once

#include "net_common.hpp"

struct transfer_state
{
	std::ofstream out;
	uint64_t total_size = 0;
	uint64_t received = 0;
	std::string tmp_path;
};

#pragma pack(push, 1)
struct file_chunk_header
{
	uint64_t total_size;
	uint64_t offset;
};
#pragma pack(pop)

struct IFileReceiver
{
	virtual ~IFileReceiver() = default;

	virtual void recv_file(const message_header& header, 
		const uint8_t* body,
		std::size_t body_size,
		std::function<void(uint64_t, uint64_t)> progress_cb,
		std::function<void()> finished_cb) = 0;
};

class AndroidFileReceiver : public IFileReceiver
{
public:
	void recv_file(const message_header& header,
		const uint8_t* body,
		std::size_t body_size,
		std::function<void(uint64_t, uint64_t)> progress_cb,
		std::function<void()> finished_cb) override
	{
		if (body_size < sizeof(file_chunk_header))
			return;

		file_chunk_header chunk_header{};
		std::memcpy(&chunk_header, body, sizeof(file_chunk_header));

		const uint8_t* chunk_data = body + sizeof(file_chunk_header);
		std::size_t chunk_length = body_size - sizeof(file_chunk_header);

		std::vector<uint8_t> chunk_vec(chunk_data, chunk_data + chunk_length);
		std::string key = make_transfer_key(header.sender_guid, header.recipient_guid);

		transfers_received_[key] += chunk_length;
		uint64_t received = transfers_received_[key];
		uint64_t total = chunk_header.total_size;

		if (file_chunk_callback_)
			file_chunk_callback_(chunk_vec, chunk_header.offset, chunk_header.total_size, header.filename);

		if (received >= total)
		{
			if (finished_cb)
				finished_cb();

			transfers_received_.erase(key);
		}

		if (progress_cb)
			progress_cb(received, total);
	}

	
	// callbacks
	using FileChunkCallback = std::function<void(const std::vector<uint8_t>& chunk,
		uint64_t offset,
		uint64_t total_size,
		const std::string& filename)>;
	
	void set_file_chunk_callback(FileChunkCallback cb)
	{
		file_chunk_callback_ = std::move(cb);
	}

private:
	std::string make_transfer_key(const boost::uuids::uuid& sender_guid, const boost::uuids::uuid& recipient_guid)
	{
		return guid_to_string(sender_guid) + "_" + guid_to_string(recipient_guid);
	}

	std::unordered_map<std::string, uint64_t> transfers_received_;

	FileChunkCallback file_chunk_callback_;
};

class DesktopFileReceiver : public IFileReceiver
{
public:
	void recv_file(const message_header& header, 
		const uint8_t* body, 
		std::size_t body_size,
		std::function<void(uint64_t, uint64_t)> progress_cb,
		std::function<void()> finished_cb) override
	{
		if (body_size < sizeof(file_chunk_header))
			return;

		std::string filename = header.filename;

		file_chunk_header chunk_header{};
		std::memcpy(&chunk_header, body, sizeof(file_chunk_header));

		uint8_t* chunk_data = const_cast<uint8_t*>(body) + sizeof(file_chunk_header);
		std::size_t chunk_length = body_size - sizeof(file_chunk_header);

		std::string key = make_transfer_key(header.sender_guid, header.recipient_guid);

		auto it = transfers_.find(key);
		if (it == transfers_.end())
		{
			transfer_state ts;
			ts.total_size = chunk_header.total_size;
			ts.received = 0;
			ts.tmp_path = "recv_" + guid_to_string(header.sender_guid) + "_" + guid_to_string(header.recipient_guid);

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

		if (progress_cb)
			progress_cb(st.received, st.total_size);

		if (st.received >= st.total_size)
		{
			st.out.close();
			std::string final_name = filename;
			std::error_code ercf;
			std::filesystem::rename(st.tmp_path, filename, ercf);

			if (ercf)
				std::cout << "error";

			if (finished_cb)
				finished_cb();

			transfers_.erase(it);
		}
	}

private:
	std::unordered_map<std::string, transfer_state> transfers_;

	std::string make_transfer_key(const boost::uuids::uuid& sender_guid, const boost::uuids::uuid& recipient_guid)
	{
		return guid_to_string(sender_guid) + "_" + guid_to_string(recipient_guid);
	}
};

