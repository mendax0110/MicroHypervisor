#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <fstream>
#include <iostream>
#include <mutex>
#include <sstream>
#include <chrono>
#include <ctime>
#include <stack>
#include <windows.h>
#include <dbghelp.h>
#include <sstream>
#include <iomanip> 
#include <queue>
#include <functional>

/// @brief Logger class for the HyperVisor \class Logger
class Logger
{
public:
	using LogCallback = std::function<void(const std::string&)>;

	Logger(const std::string& logFile);
	~Logger();

	/**
	* @brief Enum with the possible log levels
	* 
	*/
	enum class LogLevel
	{
		Info,
		Warning,
		Error,
		State
	};

	/**
	 * @brief Logs a message with a specified log level
	 * 
	 * @param level -> The log level
	 * @param message -> The message to log
	 */
	void Log(LogLevel level, const std::string& message);

	/**
	 * @brief Sets the log File
	 * 
	 * @param logFile -> The log file to write to
	 */
	void SetLogFile(const std::string& logFile);

	/**
	 * @brief Pushes a log message to the stack
	 * 
	 * @param message -> The message to push
	 */
	void PushLogMessage(const std::string& message);

	/**
	 * @brief Flushes the deferred logs
	 * 
	 */
	void FlushDeferredLogs();

	/**
	 * @brief Pushes a log level to the stack
	 * 
	 * @param level -> The log level to push
	 */
	void PushLogLevel(LogLevel level);

	/**
	 * @brief Pops a log level from the stack
	 * 
	 */
	void PopLogLevel();

	/**
	 * @brief Logs the stack trace
	 * 
	 */
	void LogStackTrace();

	/**
	 * @brief Processes the output buffer
	 * 
	 */
	void ProcessOutputBuffer();

	/**
	 * @brief Sets the log callback
	 * 
	 * @param callback -> The callback to set
	 */
	void SetLogCallback(LogCallback callback);

private:
	std::ofstream logStream_;
	std::mutex mutex_;
	std::stack<std::string> logStack_;
	std::stack<LogLevel> logLevelStack_;
	bool errorOccurred_ = false;
	bool stopLogging_ = false;
	std::thread logThread_;
	std::condition_variable cv_;
	std::queue<std::string> outputBuffer_;
	LogCallback logCallback_;
	static bool dbg_;
	static std::mutex dbgMutex_;

	/**
	 * @brief Gets the current time
	 * 
	 * @return std::string -> The current time
	 */
	std::string GetCurrentTimeLog();

	/**
	 * @brief Converts a log level to a string
	 * 
	 * @param level -> The log level
	 * @return std::string -> The log level as a string
	 */
	std::string LogLevelToString(LogLevel level);

	/**
	 * @brief Initializes the DbgHelp library
	 * 
	 */
	void InitializeDbgHelp();
};

#endif // LOGGER_H