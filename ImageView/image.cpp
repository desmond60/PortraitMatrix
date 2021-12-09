#include "image.h"
#include "image_lib.h"
#include "settings.h"

image::image()
:	_file(INVALID_HANDLE_VALUE),
	_mbitmap(nullptr),
	_format(FIF_UNKNOWN),
	_type(single)
{
	ZeroMemory(&_file_io, sizeof(_file_io));
	_file_io.read_proc =  &image::io_read;
	_file_io.write_proc = &image::io_write;
	_file_io.seek_proc =  &image::io_seek;
	_file_io.tell_proc =  &image::io_tell;
}


image::~image()
{
	for (std::vector<page_info>::const_iterator it = _pages.begin(); it != _pages.end(); ++it) {
		if (it->bitmap)
			FreeImage_Unload(it->bitmap);
		delete it->img;
	}
	if (_mbitmap)
		FreeImage_CloseMultiBitmap(_mbitmap);
	if (_file != INVALID_HANDLE_VALUE)
		CloseHandle(_file);
}


image* image::open(const wchar_t* file_name)
{
	assert(file_name && file_name[0]);

	image* img = new image();
	if (!img->load_image(file_name)) {
		delete img;
		img = nullptr;
	}

	return img;
}


bool image::load_image(const wchar_t* file_name)
{
	assert(file_name && file_name[0]);
	assert(_pages.empty());

	_file_name = file_name;

	_file = CreateFile(file_name, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS, nullptr);
	if (_file == INVALID_HANDLE_VALUE) {
		image_lib::instance()->set_last_error(GetLastError());
		return false;
	}

	_format = FreeImage_GetFileTypeFromHandle(&_file_io, _file);
	if (_format == FIF_UNKNOWN)
		_format = FreeImage_GetFIFFromFilenameU(file_name);
	if (_format != FIF_UNKNOWN && FreeImage_FIFSupportsReading(_format)) {
		if (_format != FIF_TIFF && _format != FIF_ICO && _format != FIF_GIF) {
			//Single page image
			FIBITMAP* bitmap = FreeImage_LoadFromHandle(_format, &_file_io, _file, _format == FIF_JPEG && settings::use_exif ? JPEG_EXIFROTATE : 0);
			if (bitmap) {
				_pages.resize(1);
				ZeroMemory(&_pages.front(), _pages.size() * sizeof(page_info));
				load_page(bitmap, 0);
				FreeImage_Unload(bitmap);
				_type = single;
			}
		}
		else {
			//Multipage image
			_mbitmap = FreeImage_OpenMultiBitmapFromHandle(_format, &_file_io, _file, _format == FIF_GIF ? GIF_PLAYBACK : 0);
			if (_mbitmap) {
				const int pc = FreeImage_GetPageCount(_mbitmap);
				if (pc > 0) {
					_pages.resize(pc);
					ZeroMemory(&_pages.front(), _pages.size() * sizeof(page_info));
					load_page(0);
					if (_pages.size() == 1)
						_type = single;
					else {
						if (_format == FIF_GIF)
							_type = animation;
						else
							_type = multipage;
					}
				}
			}
		}
	}

	if (_pages.empty() || _pages[0].bitmap == nullptr) {
		if (_mbitmap)
			FreeImage_CloseMultiBitmap(_mbitmap);
		_mbitmap = nullptr;
		if (_file != INVALID_HANDLE_VALUE)
			CloseHandle(_file);
		_file = INVALID_HANDLE_VALUE;

		//Try to open width GDI+
		Gdiplus::Bitmap* gdi_img = Gdiplus::Bitmap::FromFile(file_name);
		if (gdi_img) {
			if (gdi_img->GetLastStatus() != Gdiplus::Ok)
				delete gdi_img;
			else {
				//Translate to new image (own GDI+ image failed to crop without rescale - I don't know how to do it)
				Gdiplus::Bitmap* dupl = new Gdiplus::Bitmap(gdi_img->GetWidth(), gdi_img->GetHeight(), gdi_img->GetPixelFormat());
				Gdiplus::Graphics graph(dupl);
				const Gdiplus::Rect rect_dst2(0, 0, gdi_img->GetWidth(), gdi_img->GetHeight());
				graph.DrawImage(gdi_img, rect_dst2, 0, 0, gdi_img->GetWidth(), gdi_img->GetHeight(), Gdiplus::UnitPixel);
				delete gdi_img;
				gdi_img = dupl;

				_pages.resize(1);
				_pages[0].bitmap = nullptr;
				_pages[0].duration = 0;
				_pages[0].img = gdi_img;
				_type = single;
				_format = FIF_UNKNOWN;
			}
		}
	}

	return (!_pages.empty() && _pages[0].img != nullptr);
}


void image::load_page(const size_t page)
{
	assert(page < _pages.size());
	assert(_pages[page].bitmap == nullptr);
	assert(_mbitmap);

	FIBITMAP* bitmap = FreeImage_LockPage(_mbitmap, static_cast<int>(page));
	if (!bitmap)
		return;

	if (_format == FIF_GIF) {
		//FITAG* tag_disposal = nullptr;
		//FreeImage_GetMetadata(FIMD_ANIMATION, bitmap, "DisposalMethod", &tag_disposal);
		//gif_disposal_method = (tag_disposal ? *reinterpret_cast<const unsigned char*>(FreeImage_GetTagValue(tag_disposal)) : gif_disposal_method);
		FITAG* tag_duration = nullptr;
		FreeImage_GetMetadata(FIMD_ANIMATION, bitmap, "FrameTime", &tag_duration);
		if (tag_duration)
			_pages[page].duration = *reinterpret_cast<const unsigned long*>(FreeImage_GetTagValue(tag_duration));
		if (_pages[page].duration < 100)
			_pages[page].duration = 100;
	}
	load_page(bitmap, page);
	FreeImage_UnlockPage(_mbitmap, bitmap, FALSE);

	//Check for all pages load
	if (_mbitmap && _file != INVALID_HANDLE_VALUE) {
		bool all_loaded = true;
		for (std::vector<page_info>::const_iterator it = _pages.begin(); all_loaded && it != _pages.end(); ++it)
			all_loaded = (it->bitmap != nullptr);
		if (all_loaded) {
			FreeImage_CloseMultiBitmap(_mbitmap);
			_mbitmap = nullptr;
			CloseHandle(_file);
			_file = INVALID_HANDLE_VALUE;
		}
	}
}


void image::load_page(FIBITMAP* bitmap, const size_t page)
{
	assert(bitmap);
	assert(page < _pages.size());
	assert(_pages[page].bitmap == nullptr);

	Gdiplus::PixelFormat pf;
	FIBITMAP* tmp_bitmap = nullptr;
	if (FreeImage_IsTransparent(bitmap) && _format != FIF_BMP) {
		pf = PixelFormat32bppARGB;
		tmp_bitmap = FreeImage_ConvertTo32Bits(bitmap);
	}
	else {
		pf = PixelFormat24bppRGB;
		tmp_bitmap = FreeImage_ConvertTo24Bits(bitmap);
	}
	if (!tmp_bitmap) {
		tmp_bitmap = FreeImage_ConvertToFloat(bitmap);
		if (!tmp_bitmap)
			tmp_bitmap = FreeImage_ConvertToUINT16(bitmap);
		if (!tmp_bitmap)
			tmp_bitmap = FreeImage_ConvertToRGB16(bitmap);
		if (!tmp_bitmap)
			tmp_bitmap = FreeImage_ConvertToRGBF(bitmap);
		if (!tmp_bitmap)
			tmp_bitmap = FreeImage_ConvertToStandardType(bitmap);
 		int bpp = FreeImage_GetBPP(tmp_bitmap);
		if (bpp == 8) {
			FIBITMAP* tmp_bitmap2 = FreeImage_ConvertTo24Bits(tmp_bitmap);
			FreeImage_Unload(tmp_bitmap);
			tmp_bitmap = tmp_bitmap2;
			bpp = 24;
		}
		pf = bpp == 32 ? PixelFormat32bppARGB : PixelFormat24bppRGB;
	}
	if (!tmp_bitmap)
		return;

	_pages[page].bitmap = tmp_bitmap;
	_pages[page].img = new Gdiplus::Bitmap(
		FreeImage_GetWidth(tmp_bitmap),
		FreeImage_GetHeight(tmp_bitmap),
		FreeImage_GetPitch(tmp_bitmap),
		pf,
		FreeImage_GetBits(tmp_bitmap));
	if (_pages[page].img)
		_pages[page].img->RotateFlip(Gdiplus::RotateNoneFlipY);
	else
		_pages[page].img = new Gdiplus::Bitmap(1, 1, PixelFormat24bppRGB); //Dummy
}


int image::get_page_duration(const size_t page)
{
	if (!_pages[page].img)
		load_page(page);
	return _pages[page].duration;
}


Gdiplus::Image* image::get_page_image(const size_t page)
{
	if (!_pages[page].img)
		load_page(page);
	return _pages[page].img;
}


unsigned int DLL_CALLCONV image::io_read(void* buffer, unsigned int size, unsigned int count, fi_handle handle)
{
	DWORD bytes_read = static_cast<DWORD>(size * count);
	return ReadFile(handle, buffer, bytes_read, &bytes_read, nullptr) ? bytes_read/size: 0;
}


unsigned int DLL_CALLCONV image::io_write(void* /*buffer*/, unsigned int /*size*/, unsigned int /*count*/, fi_handle /*handle*/)
{
	assert(false && "Read only!");
	return 0;
}


int DLL_CALLCONV image::io_seek(fi_handle handle, long offset, int origin)
{
	LARGE_INTEGER file_position;
	file_position.QuadPart = offset;
	return SetFilePointerEx(handle, file_position, nullptr, origin) ? 0 : -1;
}


long DLL_CALLCONV image::io_tell(fi_handle handle)
{
	LARGE_INTEGER file_position;
	ZeroMemory(&file_position, sizeof(file_position));
	if (!SetFilePointerEx(handle, file_position, &file_position, FILE_CURRENT))
		return -1;
	return static_cast<long>(file_position.QuadPart);
}
