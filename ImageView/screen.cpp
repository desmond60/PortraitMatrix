#include "screen.h"
#include "settings.h"

#define MOVE_STEP		10
#define MIN_SCALE		0.01f
#define MAX_SCALE		50.0f
#define GRID_DARK_CLR	0x99
#define GRID_LIGHT_CLR	0x66
#define OVERLAY_TIMEOUT	500
#define OVERLAY_TERM	5000


/**
 * Simple lock.
 */
class lock
{
public:
	lock(CRITICAL_SECTION* sect) : _sect(sect)	{ EnterCriticalSection(_sect); }
	~lock()										{ LeaveCriticalSection(_sect); }
private:
	CRITICAL_SECTION* _sect;
};


screen::screen(const RECT* rect /*= nullptr*/)
:	_wnd(far_window()),
	_graphics(nullptr),
	_imode(Gdiplus::InterpolationModeHighQuality),
	_bkg_type(bt_solid),
	_bkg_gstep(10),
	_scale(100.0f),
	_pos_x(0), _pos_y(0),
	_tile_mode(false),
	_ov_thread(nullptr), _ov_stop(nullptr), _ov_image(nullptr)

{
	assert(_wnd);

	_ov_stop = CreateEvent(nullptr, TRUE, TRUE, nullptr);
	InitializeCriticalSectionAndSpinCount(&_sync, 1024);

	window_resize(rect);
}


screen::~screen()
{
	stop_draw_overlay();
	reset_cache();
	CloseHandle(_ov_stop);
	DeleteCriticalSection(&_sync);
	delete _graphics;
}


void screen::set_window(const HWND wnd)
{
	assert(wnd);
	_wnd = wnd;
	window_resize();
}


void screen::set_background(const bkg_type bt, const DWORD val)
{
	_bkg_type = bt;
	if (_bkg_type == bt_grid)
		_bkg_gstep = val;
	else
		_bkg_color.SetFromCOLORREF(val);
}


void screen::set_interpolation(const Gdiplus::InterpolationMode imode)
{
	_imode = imode;
}


void screen::switch_tile_mode()
{
	if (_tile_mode)
		clear();
	reset_cache();
	_tile_mode = !_tile_mode;
}


void screen::window_resize(const RECT* rect /*= nullptr*/)
{
	delete _graphics;
	_graphics = Gdiplus::Graphics::FromHWND(_wnd);
	assert(_graphics);
	_graphics->SetInterpolationMode(_imode);

	if (rect)
		_rect = *rect;
	else
		GetClientRect(_wnd, &_rect);
}


void screen::reset_cache()
{
	for (cache::iterator it = _cache.begin(); it != _cache.end(); ++it)
		delete it->second.img;
	_cache.clear();
}


void screen::clear(Gdiplus::Image* img /*= nullptr*/)
{
	if (img == nullptr) {
		PAINTSTRUCT ps;
		BeginPaint(_wnd, &ps);
		const Gdiplus::SolidBrush brush(_bkg_color);
		_graphics->FillRectangle(&brush, _rect.left, _rect.top, _rect.right - _rect.left, _rect.bottom - _rect.top);
		EndPaint(_wnd, &ps);
	}
	else {
		const int img_bottom = _pos_y + static_cast<int>(img->GetHeight() * _scale);
		const int img_right = _pos_x + static_cast<int>(img->GetWidth() * _scale);
		const Gdiplus::SolidBrush brush(_bkg_color);
		const Gdiplus::Rect cr[] = {
			Gdiplus::Rect(_rect.left, _rect.top, _rect.right - _rect.left, _pos_y),
			Gdiplus::Rect(_rect.left, img_bottom, _rect.right - _rect.left, _rect.bottom - img_bottom),
			Gdiplus::Rect(_rect.left, _rect.top + _pos_y, _pos_x, img_bottom - _pos_y),
			Gdiplus::Rect(img_right, _pos_y, _rect.right - _rect.left - img_right, img_bottom - _pos_y)
		};
		PAINTSTRUCT ps;
		BeginPaint(_wnd, &ps);
		_graphics->FillRectangles(&brush, cr, sizeof(cr) / sizeof(cr[0]));
		EndPaint(_wnd, &ps);
	}
}


void screen::draw(Gdiplus::Image* img)
{
	assert(img);

	Gdiplus::Rect src, dst;
	Gdiplus::Image* showed_img;
	{
		lock __lock(&_sync);
		showed_img = prepare_image(img, src, dst);
	}
	PAINTSTRUCT ps;
	BeginPaint(_wnd, &ps);
	_graphics->DrawImage(showed_img, dst, src.X, src.Y, src.Width, src.Height, Gdiplus::UnitPixel);
	EndPaint(_wnd, &ps);
}


bool screen::rescale(Gdiplus::Image* img, const scale scale_op)
{
	assert(img);

	lock __lock(&_sync);

	if (scale_op == sc_increase && _scale >= MAX_SCALE)
		return false;
	else if (scale_op == sc_decrease && _scale <= MIN_SCALE)
		return false;

	reset_cache();

	const int max_width = _rect.right - _rect.left;
	const int max_height = _rect.bottom - _rect.top;
	const int img_width = img->GetWidth();
	const int img_height = img->GetHeight();

	const float old_scale = _scale;

	int pos_new_x = 0;
	int pos_new_y = 0;
	bool center_image = false;

	switch (scale_op) {
		case sc_10:
		case sc_20:
		case sc_30:
		case sc_40:
		case sc_50:
		case sc_60:
		case sc_70:
		case sc_80:
		case sc_90:
		case sc_100:
			_scale = static_cast<float>(scale_op - sc_10 + 1) / 10.0f;
			center_image = true;
			break;

		case sc_optimal:
		case sc_optimal125:
		case sc_optimal150:
			switch (scale_op) {
				case sc_optimal:    _scale = 1.00f; break;
				case sc_optimal125: _scale = 1.25f; break;
				case sc_optimal150: _scale = 1.50f; break;
			}
			if (img_width > max_width || img_height > max_height) {
				const float scale_w = img_width < max_width ? 1.0f : static_cast<float>(max_width) / static_cast<float>(img_width);
				const float scale_h = img_height < max_height ? 1.0f : static_cast<float>(max_height) / static_cast<float>(img_height);
				_scale = min(scale_w, scale_h);
			}
			center_image = true;
			break;

		case sc_fitscr:
			{
				const float scale_w = static_cast<float>(max_width) / static_cast<float>(img_width);
				const float scale_h = static_cast<float>(max_height) / static_cast<float>(img_height);
				_scale = min(scale_w, scale_h);
				center_image = true;
			}
			break;

		case sc_decrease:
		case sc_increase:
			{
				const int x_center_old = static_cast<int>((100.0f / _scale) * (_rect.left + max_width / 2 - _pos_x));
				const int y_center_old = static_cast<int>((100.0f / _scale) * (_rect.top + max_height / 2 - _pos_y));
				const float scale_step = _scale / 5.0f;
				_scale += scale_op == sc_increase ? scale_step : -scale_step;
				if (scale_op == sc_increase && _scale > MAX_SCALE)
					_scale = MAX_SCALE;
				else if (scale_op == sc_decrease && _scale < MIN_SCALE)
					_scale = MIN_SCALE;
				const float scale_factor = static_cast<float>(_scale) / 100.0f;
				pos_new_x = _rect.left + max_width / 2 - static_cast<int>(scale_factor * x_center_old);
				pos_new_y = _rect.top + max_height / 2 - static_cast<int>(scale_factor * y_center_old);
			}
			break;
	}

	if (center_image) {
		pos_new_x = _rect.left + max_width / 2 - static_cast<int>(_scale * img_width) / 2;
		pos_new_y = _rect.top + max_height / 2 - static_cast<int>(_scale * img_height) / 2;
	}

	if (static_cast<int>(old_scale * 10000.0f) == static_cast<int>(_scale * 10000.0f) && pos_new_x == _pos_x && pos_new_y == _pos_y)
		return false;

	move_x(img_width, pos_new_x, mc_absolute);
	move_y(img_height, pos_new_y, mc_absolute);

	//Clean background if scale changed to less size
 	if (!_tile_mode && old_scale > _scale) {
		const bool whole_screen_size = _pos_x <= _rect.left && _pos_y <= _rect.top && static_cast<int>(_scale * img_width) >= _rect.right && static_cast<int>(_scale * img_height) >= _rect.bottom;
		if (!whole_screen_size)
			clear(img);
	}

	return true;
}


bool screen::move(Gdiplus::Image* img, const move_direction md, const move_step mt)
{
	assert(img);

	lock __lock(&_sync);

	bool rc = false;
	int move_step = MOVE_STEP;

	//Calculate move step
	if (mt == mt_image)
		move_step = 0xffffff;
	else if (mt == mt_screen) {
		move_step = static_cast<int>(md == md_right || md == md_left ? (_rect.right - _rect.left) : (_rect.bottom - _rect.top));
	}
	if (md == md_right || md == md_bottom)
		move_step = -move_step;

	//Let's move!
	switch (md) {
		case md_top:
		case md_bottom:
			rc = move_y(img->GetHeight(), move_step, mc_relative);
			break;
		case md_left:
		case md_right:
			rc = move_x(img->GetWidth(), move_step, mc_relative);
			break;
	}

	return rc;
}


bool screen::move_x(const int img_width, const int val, const move_coord mc)
{
	reset_cache();

	const int scaled_width = static_cast<int>(_scale * img_width);
	const int max_width = _rect.right - _rect.left;
	const int prev_x = _pos_x;

	if (mc == mc_absolute)
		_pos_x = val;
	else
		_pos_x += val;

	//Prevent outside positioning
	if (_pos_x > _rect.left)
		_pos_x = _rect.left;
	else if (_pos_x < _rect.left && scaled_width + _pos_x < _rect.right)
		_pos_x = max_width - scaled_width;

	//Center image if it smaller then window
	if (scaled_width <= _rect.right)
		_pos_x = _rect.left + (max_width - scaled_width) / 2;

	return (prev_x != _pos_x);
}


bool screen::move_y(const int img_height, const int val, const move_coord mc)
{
	reset_cache();

	const int scaled_height = static_cast<int>(_scale * img_height);
	const int max_height = _rect.bottom - _rect.top;
	const int prev_y = _pos_y;

	if (mc == mc_absolute)
		_pos_y = val;
	else
		_pos_y += val;

	//Prevent outside positioning
	if (_pos_y > _rect.top)
		_pos_y = _rect.top;
	else if (_pos_y < _rect.top && scaled_height + _pos_y < _rect.bottom)
		_pos_y = max_height - scaled_height;

	//Center image if it smaller then window
	if (scaled_height <= max_height)
		_pos_y = _rect.top + (max_height - scaled_height) / 2;

	return (prev_y != _pos_y);
}



HWND screen::far_window()
{
	static HWND wnd = reinterpret_cast<HWND>(_PSI.AdvControl(&_FPG, ACTL_GETFARHWND, 0, nullptr));
	return wnd;
}


void screen::start_draw_overlay(image* img)
{
	assert(img);
	assert(_ov_thread == nullptr);
	assert(_ov_image == nullptr);

	_ov_image = img;
	ResetEvent(_ov_stop);
	_ov_thread = CreateThread(nullptr, 0, &screen::overlay_thread, this, 0, nullptr);
}


size_t screen::stop_draw_overlay()
{
	size_t rc = 0;

	if (_ov_thread) {
		SetEvent(_ov_stop);
		WaitForSingleObject(_ov_thread, INFINITE);
		DWORD page_num = 0;
		GetExitCodeThread(_ov_thread, &page_num);
		rc = static_cast<size_t>(page_num);
		CloseHandle(_ov_thread);
		_ov_thread = nullptr;
		_ov_image = nullptr;
	}

	return rc;
}


Gdiplus::Image* screen::prepare_image(Gdiplus::Image* src, Gdiplus::Rect& src_rect, Gdiplus::Rect& dst_rect)
{
	assert(src);

	//Search for cached image
	cache::iterator it_cache = _cache.find(src);
	if (it_cache != _cache.end()) {
		src_rect = it_cache->second.src_rect;
		dst_rect = it_cache->second.dst_rect;
		return it_cache->second.img;
	}

	Gdiplus::Image* img_show = nullptr;

	const int src_width = src->GetWidth();
	const int src_height = src->GetHeight();
	const int src_scaled_width = static_cast<int>(_scale * src_width);
	const int src_scaled_height = static_cast<int>(_scale * src_height);
	const int max_width = _rect.right - _rect.left;
	const int max_height = _rect.bottom - _rect.top;

	if (src->GetPixelFormat() != PixelFormat32bppARGB) {
		if (!_tile_mode) {
			//No background needed
			src_rect.X = 0;
			src_rect.Y = 0;
			src_rect.Width = src_width;
			src_rect.Height = src_height;
			dst_rect.X = _pos_x;
			dst_rect.Y = _pos_y;
			dst_rect.Width = src_scaled_width;
			dst_rect.Height = src_scaled_height;
			img_show = src;
		}
		else {
			img_show = create_canvas(false, max_width, max_height);
			fill_tile(img_show, src);
			src_rect.X = dst_rect.X = _rect.left;
			src_rect.Y = dst_rect.Y = _rect.top;
			src_rect.Width = dst_rect.Width = max_width;
			src_rect.Height = dst_rect.Height = max_height;
		}
	}
	else {
		//Make image to show
		const int canvas_width = _tile_mode ? max_width : min(src_scaled_width, max_width);
		const int canvas_height = _tile_mode ? max_height : min(src_scaled_height, max_height);
		img_show = create_canvas(_bkg_type == bt_grid, canvas_width, canvas_height);

		if (_tile_mode) {
			fill_tile(img_show, src);
			src_rect.X = dst_rect.X = _rect.left;
			src_rect.Y = dst_rect.Y = _rect.top;
			src_rect.Width = dst_rect.Width = canvas_width;
			src_rect.Height = dst_rect.Height = canvas_height;
		}
		else {
			//Draw source image on background
			const int src_draw_left =   static_cast<int>(static_cast<float>(_pos_x < 0 ? -_pos_x : 0) / _scale);
			const int src_draw_top =    static_cast<int>(static_cast<float>(_pos_y < 0 ? -_pos_y : 0) / _scale);
			const int src_draw_width =  static_cast<int>(static_cast<float>(canvas_width) / _scale);
			const int src_draw_height = static_cast<int>(static_cast<float>(canvas_height) / _scale);
			Gdiplus::Graphics graph(img_show);
			graph.SetInterpolationMode(_imode);
			graph.SetPixelOffsetMode(Gdiplus::PixelOffsetModeNone);
			const Gdiplus::Rect canvas_rect(0, 0, canvas_width, canvas_height);
			graph.DrawImage(src, canvas_rect, src_draw_left, src_draw_top, src_draw_width, src_draw_height, Gdiplus::UnitPixel);

			src_rect.X = 0;
			src_rect.Y = 0;
			src_rect.Width = canvas_width;
			src_rect.Height = canvas_height;
			dst_rect.X = _pos_x < _rect.left ? _rect.left : _pos_x;
			dst_rect.Y = _pos_y < _rect.top ? _rect.top :_pos_y;
			dst_rect.Width = canvas_width;
			dst_rect.Height = canvas_height;
		}
	}

	assert(img_show && img_show->GetLastStatus() == Gdiplus::Ok);

	if (img_show != src) {
		chache_item ci;
		ci.img = img_show;
		ci.src_rect = src_rect;
		ci.dst_rect = dst_rect;
		_cache.insert(std::make_pair(src, ci));
	}
	return img_show;
}


Gdiplus::Image* screen::create_canvas(const bool use_grid, const int width, const int height) const
{
	Gdiplus::Bitmap* canvas = new Gdiplus::Bitmap(width, height, PixelFormat24bppRGB);
	assert(canvas && canvas->GetLastStatus() == Gdiplus::Ok);
	Gdiplus::Graphics graph(canvas);

	if (!use_grid)
		graph.Clear(_bkg_color);
	else {
		std::vector<Gdiplus::Rect> rects;
		rects.reserve((width / _bkg_gstep) * (height / _bkg_gstep) / 2);
		for (int y = 0; y < height; y += _bkg_gstep) {
			for (int x = (y % (_bkg_gstep * 2) ? 0 : _bkg_gstep); x < width; x += _bkg_gstep * 2) {
				rects.push_back(Gdiplus::Rect(x, y, _bkg_gstep, _bkg_gstep));
			}
		}
		if (!rects.empty()) {
			const Gdiplus::SolidBrush dark_brush(Gdiplus::Color(GRID_LIGHT_CLR, GRID_LIGHT_CLR, GRID_LIGHT_CLR));
			graph.Clear(Gdiplus::Color(GRID_DARK_CLR, GRID_DARK_CLR, GRID_DARK_CLR));
			graph.FillRectangles(&dark_brush, &rects.front(), static_cast<int>(rects.size()));
		}
	}

	return canvas;
}


void screen::fill_tile(Gdiplus::Image* canvas, Gdiplus::Image* src) const
{
	assert(canvas);
	assert(src);

	const int src_width = src->GetWidth();
	const int src_height = src->GetHeight();
	const int src_scaled_width = static_cast<int>(_scale * src_width);
	const int src_scaled_height = static_cast<int>(_scale * src_height);

	Gdiplus::Graphics graph(canvas);
	graph.SetInterpolationMode(_imode);
	graph.SetPixelOffsetMode(Gdiplus::PixelOffsetModeNone);

	const int tile_x_start = _pos_x > 0 ? (((_pos_x - _rect.left) % src_scaled_width) - src_scaled_width) : _pos_x;
	const int tile_y_start = _pos_y > 0 ? (((_pos_y - _rect.top) % src_scaled_height) - src_scaled_height) : _pos_y;
	for (int y = tile_y_start; y < _rect.bottom; y += src_scaled_height) {
		for (int x = tile_x_start; x < _rect.right; x += src_scaled_width) {
			const Gdiplus::Rect rect_dst(x, y, src_scaled_width, src_scaled_height);
			graph.DrawImage(src, rect_dst, 0, 0, src_width, src_height, Gdiplus::UnitPixel);
		}
	}
}


DWORD WINAPI screen::overlay_thread(PVOID param)
{
	screen* inst = reinterpret_cast<screen*>(param);
	assert(inst);

	const bool animate = inst->_ov_image->get_type() == image::animation;
	const size_t pages_count = inst->_ov_image->get_pages_count();
	DWORD timeout = OVERLAY_TIMEOUT;
	DWORD draw_start = 0;
	size_t page = 0;

	do {
// #ifdef _DEBUG
// 		wchar_t dbg[32];
// 		swprintf_s(dbg, L"[screen] Draw frame %i\n", page);
// 		OutputDebugString(dbg);
// #endif // _DEBUG

		if (animate)
			draw_start = GetTickCount();

		inst->draw(inst->_ov_image->get_page_image(page));

		if (animate) {
			//Calculate frame duration
			const DWORD draw_timeout = GetTickCount() - draw_start;
			timeout = inst->_ov_image->get_page_duration(page);
			if (timeout < draw_timeout)
				timeout = 0;
			else
				timeout -= draw_timeout;

			//Step to the next frame
			++page;
			if (page >= pages_count)
				page = 0;
		}

	} while (WaitForSingleObject(inst->_ov_stop, timeout) == WAIT_TIMEOUT);

	return static_cast<DWORD>(page);
}
