#include "image_lib.h"

#pragma warning(push)
#pragma warning(disable : 4458)
#include <GdiPlus.h>
#pragma warning(pop)
#pragma warning (disable : 4244)


image_lib::image_lib()
: _gdiplus_token(0)
{
	//Initialize GDI+
	Gdiplus::GdiplusStartupInput gdiplus_si(nullptr, TRUE);
	Gdiplus::GdiplusStartupOutput gdiplus_so;
	ZeroMemory(&gdiplus_so, sizeof(gdiplus_so));
	Gdiplus::GdiplusStartup(&_gdiplus_token, &gdiplus_si, &gdiplus_so);

	//Initialize FreeImage library
	FreeImage_Initialise();
	FreeImage_SetOutputMessage(&image_lib::error_handler);
}


image_lib::~image_lib()
{
	FreeImage_DeInitialise();
	Gdiplus::GdiplusShutdown(_gdiplus_token);
}


image_lib* image_lib::instance()
{
	static image_lib inst;
	return &inst;
}


bool image_lib::format_supported(const wchar_t* file_name) const
{
	assert(file_name);

	static std::set<std::wstring> exts;
	if (exts.empty()) {
		const int n = FreeImage_GetFIFCount();
		for (int i = 0; i < n; ++i) {
			const char* ext = FreeImage_GetFIFExtensionList(static_cast<FREE_IMAGE_FORMAT>(i));
			const int len = static_cast<int>(ext ? strlen(ext) : 0);
			if (len != 0) {
				std::wstring wide;
				const int req = MultiByteToWideChar(CP_ACP, 0, ext, len, nullptr, 0);
				if (req) {
					wide.resize(static_cast<size_t>(req));
					MultiByteToWideChar(CP_ACP, 0, ext, len, &wide.front(), req);
				}
				std::transform(wide.begin(), wide.end(), wide.begin(), ::tolower);
				size_t last_pos = wide.find_first_not_of(L',', 0);
				size_t next_pos = wide.find_first_of(L',', last_pos);
				while (next_pos != std::string::npos || last_pos != std::string::npos) {
					exts.insert(wide.substr(last_pos, next_pos - last_pos));
					last_pos = wide.find_first_not_of(L',', next_pos);
					next_pos = wide.find_first_of(L',', last_pos);
				}
			}
		}
	}

	const wchar_t* ext = file_name ? wcsrchr(file_name, L'.') : nullptr;
	if (!ext)
		return false;
	std::wstring lext(ext + 1);
	std::transform(lext.begin(), lext.end(), lext.begin(), ::tolower);
	return exts.find(lext) != exts.end();
}


void image_lib::set_last_error(const DWORD error_code)
{
	wchar_t err_num[64];
	swprintf_s(err_num, L"[0x08%X] ", error_code);
	_last_error = err_num;

	wchar_t* err_msg = nullptr;
	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, error_code, 0, reinterpret_cast<LPTSTR>(&err_msg), 512, nullptr);
	if (err_msg) {
		_last_error += err_msg;
		LocalFree(err_msg);
	}
	size_t pos = 0;
	while ((pos = _last_error.find_first_of(L"\r\n", pos)) != std::string::npos)
		_last_error[pos] = L' ';
}


void image_lib::error_handler(FREE_IMAGE_FORMAT fif, const char* message)
{
	using namespace std;
	wstring& err_msg = instance()->_last_error;

	const char* img_format = FreeImage_GetFormatFromFIF(fif);
	if (!img_format || !*img_format)
		img_format = "Unknown";
	string full_msg;
	full_msg += "Format [";
	full_msg += img_format;
	full_msg += "]: ";
	full_msg += message && *message ? message : "Unknown error";
	size_t pos = 0;
	while ((pos = full_msg.find_first_of("\r\n", pos)) != string::npos)
		full_msg[pos] = ' ';

	const int len = static_cast<int>(full_msg.length());
	const int req = MultiByteToWideChar(CP_ACP, 0, full_msg.c_str(), len, nullptr, 0);
	if (req) {
		err_msg.resize(static_cast<size_t>(req));
		MultiByteToWideChar(CP_ACP, 0, full_msg.c_str(), len, &err_msg.front(), req);
	}

#ifdef _DEBUG
	OutputDebugString(wstring(wstring(L"FarImageViewer: ") + err_msg).c_str());
#endif //_DEBUG
}
