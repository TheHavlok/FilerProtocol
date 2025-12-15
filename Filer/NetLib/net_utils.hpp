#pragma once

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/lexical_cast.hpp>

inline auto guid_generator()
{
	boost::uuids::uuid guid = boost::uuids::random_generator()();
	return guid;
}

inline auto string_to_guid(std::string guid)
{
	return boost::lexical_cast<boost::uuids::uuid>(guid);
}

inline std::string guid_to_string(boost::uuids::uuid guid)
{
	return boost::uuids::to_string(guid);
}

inline const char* guid_to_char(boost::uuids::uuid guid)
{
	const std::string tmp = boost::uuids::to_string(guid);
	return tmp.c_str();
}