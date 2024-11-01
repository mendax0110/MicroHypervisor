#include "Logger.h"

Logger::Logger(const std::string& logFile) : stopLogging_(false)
{
	InitializeDbgHelp();
	SetLogFile(logFile);
	logThread_ = std::thread([this]()
	{
		while (!stopLogging_)
		{
			FlushDeferredLogs();
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
	});
}

Logger::~Logger()
{
	stopLogging_ = true;
	cv_.notify_all();
	if (logThread_.joinable())
	{
		logThread_.join();
	}
	if (logStream_.is_open())
	{
		logStream_.close();
	}
}

void Logger::Log(LogLevel level, const std::string& message)
{
	std::lock_guard<std::mutex> lock(mutex_);

	if (!logLevelStack_.empty() && level < logLevelStack_.top())
	{
		return;
	}

	if (logStream_.is_open())
	{
		std::string logMessage = GetCurrentTimeLog() + " [" + LogLevelToString(level) + "] " + message + "\n";
		logStream_ << logMessage << std::flush;
		outputBuffer_.push(logMessage);
		cv_.notify_all();

		if (logCallback_)
		{
			logCallback_(logMessage);
		}

		switch (level)
		{
			case LogLevel::Info:
				std::cout << "\033[1;32m" << logMessage << "\033[0m";
				break;
			case LogLevel::Warning:
				std::cout << "\033[1;33m" << logMessage << "\033[0m";
				break;
			case LogLevel::Error:
				std::cerr << "\033[1;31m" << logMessage << "\033[0m";
				break;
			case LogLevel::State:
				std::cout << "\033[1;34m" << logMessage << "\033[0m";
				break;
			default:
				break;
		}
	}
}

void Logger::LogStackTrace()
{
	void* stack[100] = {};
	unsigned short frames;
	SYMBOL_INFO* symbol;
	HANDLE process = GetCurrentProcess();

	frames = CaptureStackBackTrace(0, 100, stack, nullptr);
	symbol = (SYMBOL_INFO*)calloc(sizeof(SYMBOL_INFO) + 256 * sizeof(char), 1);
	symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
	symbol->MaxNameLen = 255;

	std::ostringstream stackTrace;
	stackTrace << GetCurrentTimeLog() << " [STACK TRACE]\n";

	for (unsigned short i = 0; i < frames; i++)
	{
		if (SymFromAddr(process, (DWORD64)(stack[i]), 0, symbol))
		{
			stackTrace << "  " << i << ": " << symbol->Name
				<< " - Address: 0x" << std::hex << symbol->Address
				<< " (Module Base: 0x" << std::hex << symbol->ModBase << ")\n"
				<< "    Type Index: " << symbol->TypeIndex << "\n"
				<< "    Size: " << symbol->Size << " bytes\n"
				<< "    Flags: 0x" << std::hex << symbol->Flags << "\n"
				<< "    Value: 0x" << std::hex << symbol->Value << "\n"
				<< "    Register: " << symbol->Register << "\n"
				<< "    Scope: " << symbol->Scope << "\n"
				<< "    Tag: " << symbol->Tag << "\n";
		}
		else
		{
			stackTrace << "  " << i << ": Unknown function - Address: 0x" << std::hex << (DWORD64)(stack[i]) << "\n";
		}
	}

	free(symbol);
	Log(LogLevel::Info, stackTrace.str());
}

bool Logger::dbg_ = false;
std::mutex Logger::dbgMutex_;

void Logger::InitializeDbgHelp()
{
	std::lock_guard<std::mutex> lock(dbgMutex_);
	if (dbg_)
		return;

	SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES);
	auto process = GetCurrentProcess();
	auto currentDirectory = GetCurrentDirectoryA(0, nullptr);
	auto buffer = std::make_unique<char[]>(currentDirectory);

	GetCurrentDirectoryA(currentDirectory, buffer.get());
	if (!SymInitialize(process, buffer.get(), TRUE))
	{
		std::cerr << "[Error]: Failed to initialize DbgHelp\n";
	}
	else
	{
		dbg_ = true;
		std::cout << "[Info]: DbgHelp initialized\n";
	}
}

void Logger::SetLogFile(const std::string& logFile)
{
	if (logStream_.is_open())
	{
		logStream_.close();
	}
	logStream_.open(logFile, std::ios::out | std::ios::app);
	if (!logStream_.is_open())
	{
		std::cerr << "[Error]: Failed to open log file: " << logFile << "\n";
	}
}

void Logger::PushLogMessage(const std::string& message)
{
	std::lock_guard<std::mutex> lock(mutex_);
	logStack_.push(message);
}

void Logger::FlushDeferredLogs()
{
	std::lock_guard<std::mutex> lock(mutex_);
	while (!logStack_.empty())
	{
		logStream_ << logStack_.top() << std::endl;
		logStack_.pop();
	}
}

void Logger::PushLogLevel(LogLevel level)
{
	std::lock_guard<std::mutex> lock(mutex_);
	logLevelStack_.push(level);
}

void Logger::PopLogLevel()
{
	std::lock_guard<std::mutex> lock(mutex_);
	if (!logLevelStack_.empty())
	{
		logLevelStack_.pop();
	}
}

std::string Logger::GetCurrentTimeLog()
{
	auto now = std::chrono::system_clock::now();
	std::time_t nT = std::chrono::system_clock::to_time_t(now);
	std::tm now_tm;
	localtime_s(&now_tm, &nT);
	char buffer[100]{};
	std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &now_tm);
	return std::string(buffer);
}

std::string Logger::LogLevelToString(LogLevel level)
{
	switch (level)
	{
		case LogLevel::Info:
			return "INFO";
		case LogLevel::Warning:
			return "WARNING";
		case LogLevel::Error:
			return "ERROR";
		case LogLevel::State:
			return "STATE";
		default:
			return "UNKNOWN";
	}
}

void Logger::ProcessOutputBuffer()
{
	while (!stopLogging_)
	{
		std::unique_lock<std::mutex> lock(mutex_);
		cv_.wait(lock, [this] { return !logStack_.empty() || stopLogging_; });

		while (!outputBuffer_.empty())
		{
			std::string logMessage = outputBuffer_.front();
			outputBuffer_.pop();
		}
	}
}

void Logger::SetLogCallback(LogCallback callback)
{
	std::lock_guard<std::mutex> lock(mutex_);
	logCallback_ = callback;
}