#include "utils.h"
#include <shlwapi.h>
#include <boost/filesystem.hpp>
#include "json.hpp"
#include <iostream>

void OpenLogFile(const boost::filesystem::path& file)
{
	auto res = ShellExecute(nullptr, "open", file.string().c_str(), nullptr, nullptr, SW_SHOWDEFAULT);
}

void OpenBareTail(const std::vector<std::string>& logfiles)
{
	std::string params;
	for (auto file : logfiles) 
		params += "\"" + file + "\" ";

	STARTUPINFO startupInfo = { 0 };
	PROCESS_INFORMATION procInfo = { 0 };
	if (!CreateProcess("baretail.exe", const_cast<char*>(params.c_str()), nullptr, nullptr, FALSE,
		0, nullptr, nullptr, &startupInfo, &procInfo)) {
		printf("Error running baretail.exe, err=%d\n", GetLastError());
		return;
	}

	CloseHandle(procInfo.hThread);
	CloseHandle(procInfo.hProcess);
}

fs::path GetMailMarshalInstallDirectory()
{
	try {
		RegKey mmKey(HKEY_LOCAL_MACHINE, "SOFTWARE\\Trustwave\\Secure Email Gateway", 0, KEY_WOW64_64KEY | GENERIC_READ);
		return fs::path(mmKey.GetStringValue("InstallPath"));
	}
	catch (std::exception& e) {
		printf("Error: %s\n", e.what());
	}

	const char* defaultDir = "c:\\Program Files\\Trustwave\\Secure Email Gateway";
	printf("Cannot get MailMarshal install directory, defaulting to %s", defaultDir);
	return fs::path(defaultDir);
}

fs::path GetManagerLoggingDirectory()
{
	using namespace nlohmann;

	fs::path mmdir = GetMailMarshalInstallDirectory();
	fs::path amcfg = mmdir / fs::path("arraymanager.config.json");

	try {
		if (fs::exists(amcfg)) {
			std::ifstream cfgfile(amcfg.string().c_str());
			json cfg;
			cfgfile >> cfg;

			auto dir = cfg["Directories"];
			if (dir.is_null())
				throw std::runtime_error("no directories key");

			auto logging = dir["Logging"];
			if (logging.is_null())
				throw std::runtime_error("no logging key");

			auto logdir = fs::path(logging.get<std::string>());
			return logdir.is_absolute() ? logdir : mmdir / logdir;
		}
	}
	catch (std::exception&) {}

	// try registry
	try {
		RegKey mmKey(HKEY_LOCAL_MACHINE, "SOFTWARE\\Trustwave\\Secure Email Gateway");
		RegKey managerKey(mmKey, "Manager");
		RegKey dirKey(managerKey, "Directories");

		fs::path logdir = dirKey.GetStringValue("Logging");
		return fs::absolute(logdir, mmdir);
	}
	catch (std::exception& e) {
		printf("Error: %s\n", e.what());
	}
	
	// return default
	return mmdir / fs::path("logging");
}

fs::path GetControllerLoggingDirectory()
{
	using namespace nlohmann;

	fs::path mmdir = GetMailMarshalInstallDirectory();
	fs::path amcfg = mmdir / fs::path("controller.config.json");

	try {
		if (fs::exists(amcfg)) {
			std::ifstream cfgfile(amcfg.string().c_str());
			json cfg;
			cfgfile >> cfg;

			auto dir = cfg["Directories"];
			if (dir.is_null())
				throw std::runtime_error("no directories key");

			auto logging = dir["Logging"];
			if (logging.is_null())
				throw std::runtime_error("no logging key");

			auto logdir = fs::path(logging.get<std::string>());
			return logdir.is_absolute() ? logdir : mmdir / logdir;
		}
	}
	catch (std::exception&) {}

	// try registry
	try {
		RegKey mmKey(HKEY_LOCAL_MACHINE, "SOFTWARE\\Trustwave\\Secure Email Gateway");
		RegKey managerKey(mmKey, "Node");
		RegKey dirKey(managerKey, "Directories");

		fs::path logdir = dirKey.GetStringValue("Logging");
		return fs::absolute(logdir, mmdir);
	}
	catch (std::exception& e) {
		printf("Error: %s\n", e.what());
	}
	
	// return default
	return mmdir / fs::path("logging");
}
void DLog(const char* fmt, ...)
{
	char buf[1024];
	va_list args;
	va_start(args, fmt);
	vsnprintf(buf, sizeof(buf) - 1, fmt, args);
	OutputDebugString(buf);
	va_end(args);
}
