#pragma once

#include <boost/filesystem.hpp>
#include <atomic>
#include <stdarg.h>
#include <windows.h>

#if defined(min)
#undef min
#endif

#if defined(max)
#undef max
#endif

namespace fs = boost::filesystem;

class Spinlock
{
	// std::mutex will throw an exception when busy, so create a spinlock
	// class based on atomic_flag::test_and_reset
	std::atomic_flag& flag;
public:
	Spinlock(std::atomic_flag& lock) : flag(lock) { lock.test_and_set(); }
	~Spinlock() { flag.clear(); }
};

void OpenLogFile(const boost::filesystem::path& file);

fs::path GetMailMarshalInstallDirectory();

void DLog(const char* fmt, ...);


