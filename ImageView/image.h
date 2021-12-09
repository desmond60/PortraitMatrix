#pragma once

#include "common.h"
#include <FreeImage.h>

#pragma warning(push)
#pragma warning(disable : 4458)
#include <GdiPlus.h>


#pragma warning(pop)

class image
{
private:
	image();

public:
	~image();

	//! Image type.
	enum type {
		single,
		multipage,
		animation
	};

	/**
	 * Open image.
	 * \param file_name image file name
	 * \return image instance (nullptr if error)
	 */
	static image* open(const wchar_t* file_name);

	/**
	 * Get image type.
	 * \return image type
	 */
	type get_type() const { return _type; }

	/**
	 * Get image file name.
	 * \return image file name
	 */
	const wchar_t* get_file_name() const { return _file_name.c_str(); }

	/**
	 * Get pages count.
	 * \return pages count
	 */
	size_t get_pages_count() const { return _pages.size(); }

	/**
	 * Get page animation duration (GIF only).
	 * \param page page number
	 * \return page animation duration in ms
	 */
	int get_page_duration(const size_t page);

	/**
	 * Get page as GDI+ image.
	 * \param page page number
	 * \return page image
	 */
	Gdiplus::Image* get_page_image(const size_t page);

private:
	/**
	 * Load image from file.
	 * \param file_name image file name
	 * \return true if image loaded
	 */
	bool load_image(const wchar_t* file_name);

	/**
	 * Load page from file.
	 * \param page page number
	 */
	void load_page(const size_t page);

	/**
	 * Load page from file.
	 * \param bitmap bitmap pointer
	 * \param page page number
	 */
	void load_page(FIBITMAP* bitmap, const size_t page);

private:
	//FreeImage IO stream implementation
	static unsigned int DLL_CALLCONV io_read(void* buffer, unsigned int size, unsigned int count, fi_handle handle);
	static unsigned int DLL_CALLCONV io_write(void* buffer, unsigned int size, unsigned int count, fi_handle handle);
	static int DLL_CALLCONV io_seek(fi_handle handle, long offset, int origin);
	static long DLL_CALLCONV io_tell(fi_handle handle);

private:
	std::wstring		_file_name;
	FreeImageIO _file_io;
	HANDLE		_file;

	FIMULTIBITMAP*		_mbitmap;

	FREE_IMAGE_FORMAT	_format;
	type				_type;

	struct page_info {
		FIBITMAP*	bitmap;			///< Image bitmap
		int			duration;		///< Page show time in ms (only for animated GIFs)
		Gdiplus::Image* img;		///< Page GDI+ image
	};
	std::vector<page_info> _pages;
};
