#include "viewer_qv.h"
#include "screen.h"
#include "settings.h"


viewer_qv::viewer_qv()
:	_screen(nullptr)
{
}


viewer_qv::~viewer_qv()
{
	hide();
}


viewer_qv* viewer_qv::instance()
{
	static viewer_qv inst;
	return &inst;
}


void viewer_qv::show(image* img, const PanelInfo& pi)
{
	assert(img);

	hide();

	assert(_screen == nullptr);

	//Calculate position and scale
	CONSOLE_FONT_INFO cons_font;
	cons_font.dwFontSize.X = 12;
	cons_font.dwFontSize.Y = 8;
	GetCurrentConsoleFont(GetStdHandle(STD_OUTPUT_HANDLE), FALSE, &cons_font);
	RECT wnd_rect;
	wnd_rect.left = cons_font.dwFontSize.X * (pi.PanelRect.left + 1);
	wnd_rect.top = cons_font.dwFontSize.Y * (pi.PanelRect.top + 1);
	wnd_rect.right = wnd_rect.left + cons_font.dwFontSize.X * (pi.PanelRect.right - pi.PanelRect.left - 1);
	wnd_rect.bottom = wnd_rect.top + cons_font.dwFontSize.Y * (pi.PanelRect.bottom - pi.PanelRect.top - 3);

	//Move view position to clear screen
	ViewerSetPosition vsp;
	vsp.StructSize = sizeof(vsp);
	vsp.Flags = VSP_PERCENT;
	vsp.StartPos = vsp.LeftPos = 101;
	_PSI.ViewerControl(-1, VCTL_SETPOSITION, 0, &vsp);
	
	_panel_info = pi;
	_image = img;
	_screen = new screen(&wnd_rect);
	_screen->set_interpolation(settings::interpolate);
	_screen->set_background(settings::bg_grid_step ? screen::bt_grid : screen::bt_solid, settings::bg_grid_step);
	_screen->rescale(_image->get_page_image(0), screen::sc_optimal);
	_screen->start_draw_overlay(_image);
}


void viewer_qv::hide()
{
	if (_screen) {
		_screen->stop_draw_overlay();
		delete _image;
		_image = nullptr;
		delete _screen;
		_screen = nullptr;
		clear_bkg();
		InvalidateRect(screen::far_window(), nullptr, TRUE);
	}
}


void viewer_qv::clear_bkg() const
{
	HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo(console, &csbi);
	COORD coord;
	coord.X = static_cast<SHORT>(_panel_info.PanelRect.left + 1);
	coord.Y = static_cast<SHORT>(csbi.srWindow.Top + _panel_info.PanelRect.top + 1);
	const SHORT top = static_cast<SHORT>(csbi.srWindow.Top + _panel_info.PanelRect.top + 1);
	const SHORT bottom = static_cast<SHORT>(csbi.srWindow.Top + _panel_info.PanelRect.bottom - 2);
	DWORD written;
	for (coord.Y = top; coord.Y < bottom; ++coord.Y)
		FillConsoleOutputCharacter(console, L' ', _panel_info.PanelRect.right - _panel_info.PanelRect.left - 1, coord, &written);

	// Not worked correctly :-(
	//FarColor bkg_color;
	//_PSI.AdvControl(&_FPG, ACTL_GETCOLOR, COL_PANELBOX, &bkg_color);
	//const wstring empty_line(_panel_info.PanelRect.right - _panel_info.PanelRect.left - 1, L' ');
	//for (LONG y = _panel_info.PanelRect.top + 1; y < _panel_info.PanelRect.bottom - 2; ++y)
	//	_PSI.Text(_panel_info.PanelRect.left + 1, y, &bkg_color, empty_line.c_str());
	//_PSI.Text(0, 0, nullptr, nullptr); //Flush buffer
}
