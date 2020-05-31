#include "utils.h"
#include <shlwapi.h>
#include <boost/filesystem.hpp>

void OpenLogFile(const boost::filesystem::path& file)
{
	auto res = ShellExecute(nullptr, "open", file.string().c_str(), nullptr, nullptr, SW_SHOWDEFAULT);
}

fs::path GetMailMarshalInstallDirectory()
{
	const char* defaultDir = "c:\\Program Files\\Trustwave\\Secure Email Gateway";
	HKEY hKey = nullptr;
	try {
		if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\Trustwave\\Secure Email Gateway", 0,
				KEY_WOW64_64KEY|GENERIC_READ, &hKey) != ERROR_SUCCESS)
			throw std::runtime_error("Cannot open registry");

		DWORD type;
		BYTE buf[1024];
		DWORD len = sizeof(buf);
		if (RegQueryValueEx(hKey, "InstallPath", nullptr, &type, buf, &len) != ERROR_SUCCESS)
			throw std::runtime_error("Cannot get install path");

		if (type != REG_SZ)
			throw std::runtime_error("Invalid type - install path");
		return fs::path((char*)buf);
	}
	catch (std::exception& e) {
		printf("Error: %s\n", e.what());
	}

	if (hKey)
		RegCloseKey(hKey);

	printf("Cannot get MailMarshal install directory, defaulting to %s", defaultDir);
	return fs::path(defaultDir);
}
