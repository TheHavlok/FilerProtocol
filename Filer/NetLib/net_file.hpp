#pragma once

#include "net_common.hpp"

class file
{
public:
	std::vector<uint8_t> read_file(const std::string& file_path)
	{
		std::vector<uint8_t> data;
		std::ifstream read(file_path, std::ios::binary);
		read.unsetf(std::ios::skipws);
		std::streampos fileSize = read.tellg();
		data.reserve(fileSize);
		std::copy(std::istream_iterator<uint8_t>(read), std::istream_iterator<uint8_t>(), std::back_inserter(data));
		return data;
	}

	void write_file(const std::string& file_path, const std::vector<uint8_t> data)
	{
		std::ofstream out(file_path, std::ios::out | std::ios::binary);

		out.write(reinterpret_cast<const char*>(data.data()), data.size());
		out.close();
	}

private:

};
