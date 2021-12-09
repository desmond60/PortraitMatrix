#pragma once
#include "common.h"
#include <FreeImage.h>

class image_lib
{
private:
	image_lib();
	~image_lib();

public:
	/**
	 * Get class singleton instance.
	 * \return instance
	 */
	static image_lib* instance();

	/**
	 * Check for format supported by its file extension.
	 * \param file_name checked file name
	 * \return true if file format supported
	 */
	bool format_supported(const wchar_t* file_name) const;

	/**
	 * Set last error description from Win32 error code.
	 * \param error_code Win32 error code
	 */
	void set_last_error(const DWORD error_code);

	/**
	 * Get last error description.
	 * \return last error description
	 */
	const wchar_t* get_last_error() const { return _last_error.c_str(); }

private:
	//FreeImage error handler
	static void error_handler(FREE_IMAGE_FORMAT fif, const char* message);

private:
	ULONG_PTR _gdiplus_token;
	std::wstring   _last_error;
};
