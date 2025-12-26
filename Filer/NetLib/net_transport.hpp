#pragma once

#include "net_common.hpp"

enum class Flags : uint8_t
{
	ASK, // 0x01 receipt confirmation
	SYN, // 0x02 connection start
	FIN, // 0x04 Transfer completion 
	RST, // 0x08 emergensy break
	PSH, // 0x10 urgent delivery
	ECE, // 0x20 overload signal (ECN)
	CWR, // 0x40 load reduction confirmation
	RES, // 0x80 reserve
};

struct Packet
{
	const size_t MAX_PAYLOAD = 1400;

	uint32_t SeqNum;
	Flags Flag;
	uint64_t PayloadSize;
	std::vector<uint8_t> Payload;
	std::array<uint8_t, 32> HashSum;

	void GetPacket(std::vector<uint8_t>& data, Flags flag, queue<std::vector<uint8_t>>& msgQueue)
	{
		if (data.size() > MAX_PAYLOAD)
		{
			for (size_t i = 0, j = 0; i < data.size(); i += MAX_PAYLOAD, ++j)
			{
				size_t chunkSize = std::min(MAX_PAYLOAD, data.size() - i);
				Packet pkt;
				pkt.SeqNum = j;
				pkt.Flag = flag;
				pkt.PayloadSize = chunkSize;
				pkt.Payload.assign(data.begin() + i, data.begin() + i + chunkSize);
				pkt.HashSum = CalcHashSum(pkt.Payload);

				
				msgQueue.push_back(Serialize());
				// msgQueue.push_back(pkt);
			}
		}
		else
		{
			Packet pkt;
			pkt.SeqNum = 0;
			pkt.Flag = flag;
			pkt.PayloadSize = data.size();
			pkt.Payload = data;
			pkt.HashSum = CalcHashSum(pkt.Payload);

			msgQueue.push_back(Serialize());
			//msgQueue.push_back(pkt);
		}
	}

	std::array<uint8_t, 32> CalcHashSum(std::vector<uint8_t>& payload)
	{
		uint8_t xor_sum = 0;
		for (uint8_t byte : payload)
			xor_sum ^= byte;

		std::array<uint8_t, 32> hash = {};
		for (size_t i = 0; i < 32; ++i)
			hash[i] = xor_sum;

		return hash;
	}

	std::vector<uint8_t> Serialize() const
	{
		std::vector<uint8_t> buffer;
		
		// SeqNum (4 byte)
		buffer.push_back(static_cast<uint8_t>(SeqNum & 0xFF));
		buffer.push_back(static_cast<uint8_t>((SeqNum >> 8) & 0xFF));
		buffer.push_back(static_cast<uint8_t>((SeqNum >> 16) & 0xFF));
		buffer.push_back(static_cast<uint8_t>((SeqNum >> 24) & 0xFF));

		// Flags
		buffer.push_back(static_cast<uint8_t>(Flag));

		// PayloadSize
		for (int i = 0; i < 8; ++i)
			buffer.push_back(static_cast<uint8_t>((PayloadSize >> (i * 8)) & 0xFF));

		// Payload
		buffer.insert(buffer.end(), Payload.begin(), Payload.end());

		// HashSum
		buffer.insert(buffer.end(), HashSum.begin(), HashSum.end());
		
		return buffer;
	}

	static Packet Deserialize(const std::vector<uint8_t>& buffer)
	{

	}
};

//enum class Packet
//{
//	Handshake,
//	HandshakeAck,
//	Encrypted,
//};

enum class DecryptedPacket
{
	Heartbeat,
	HeartbeatAsk,
	Data,
	Rotate,
	RotateAsk,
};

struct HandshakePacket
{
	uint64_t source_id;
	boost::asio::ip::address peer_address;
};