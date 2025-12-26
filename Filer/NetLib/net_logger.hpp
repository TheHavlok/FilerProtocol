#pragma once

#include "net_common.hpp"
#include "net_file.hpp"

enum LogLevel
{
	Debug,
	Info,
	Warning,
	Error,
	Critical
};

class logger
{
public:
	logger(const std::string logPath, bool dev = true)
	{
		logFile.open(logPath, std::ios::app);
		if (!logFile.is_open())
			successCreateLogFile_ = false;
		else
			successCreateLogFile_ = true;
	}

	~logger()
	{
		logFile.close();
	}

	void log(const std::string& msg, LogLevel level = Debug)
	{
		std::scoped_lock lock(muxLog);
		time_t now = time(0);
		tm* timeinfo = localtime(&now);
		char timestamp[20];
		strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", timeinfo);

		std::ostringstream logEntry;
		logEntry << "[" << timestamp << "] " << levelToString(level) << ": " << msg << std::endl;

		std::cout << logEntry.str();

		if (logFile.is_open())
		{
			logFile << logEntry.str();
			logFile.flush();
		}
	}

private:
	std::string levelToString(LogLevel level)
	{
		switch (level)
		{
		case Debug:
			return "[DEBUG]";
		case Info:
			return "[INFO]";
		case Warning:
			return "[WARNING]";
		case Error:
			return "[ERROR]";
		case Critical:
			return "[CRITICAL]";
		default:
			return "[UNKNOWN]";
		}
	}

private:
	std::ofstream logFile;
	bool successCreateLogFile_;
	std::mutex muxLog;
};
