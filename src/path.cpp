#include "path.hpp"

#ifdef _WIN32
#include <windows.h>
#endif

std::filesystem::path pars::get_exe_path()
{
	// i have no idea if this actually works.
#ifdef _WIN32
	constexpr auto BUFF_SIZE = 4096;
	wchar_t buffer[BUFF_SIZE]{};
	auto len = GetModuleFileNameW(nullptr, buffer, BUFF_SIZE);
	return buffer;
#else
	return std::filesystem::canonical("/proc/self/exe");
#endif
}