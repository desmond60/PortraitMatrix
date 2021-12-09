#include "viewer.h"
#include "string_rc.h"
#include "settings.h"

#ifndef DN_ACTIVATEAPP
#define DN_ACTIVATEAPP	(DM_USER - 1)	//Minimize/show Far console window
#endif //DN_ACTIVATEAPP


viewer::viewer(image* img)
:	_image(img),
	_curr_page(std::string::npos),
	_dialog(INVALID_HANDLE_VALUE),
	_fs_wnd(nullptr),
	_image_list(img),
	_draw_count(0),
	_menu_mode(false)
{
	assert(_image);

	_screen.set_background(settings::bg_grid_step ? screen::bt_grid : screen::bt_solid, settings::bg_grid_step);
	_screen.set_interpolation(settings::interpolate);
}


viewer::~viewer()
{
	stop_animation();
	if (_dialog != INVALID_HANDLE_VALUE)
		_PSI.DialogFree(_dialog);
}


void viewer::show()
{
	SMALL_RECT rc_far_wnd;
	_PSI.AdvControl(&_FPG, ACTL_GETFARRECT, 0, &rc_far_wnd);
	const FarDialogItem dlg_items[] = { { DI_TEXT }, { DI_USERCONTROL } };
	_dialog = _PSI.DialogInit(&_FPG, &_FPG, 0, 0, rc_far_wnd.Right + 1, rc_far_wnd.Bottom - rc_far_wnd.Top + 1, nullptr, dlg_items, sizeof(dlg_items) / sizeof(dlg_items[0]), 0, FDLG_NODRAWSHADOW, &viewer::dlg_proc, this);
	assert(_dialog != INVALID_HANDLE_VALUE);
	_PSI.DialogRun(_dialog);

	//Redraw Far's panels
	InvalidateRect(screen::far_window(), nullptr, TRUE);
}


void viewer::close()
{
	stop_animation();
	if (_fs_wnd)
		DestroyWindow(_fs_wnd);
	_PSI.SendDlgMessage(_dialog, DM_CLOSE, 0, nullptr);
	_image = nullptr;
}


void viewer::open_image(image* img)
{
	if (!img) {
		close();
		return;
	}
	if (_image == img && _curr_page != std::string::npos)
		return;

	stop_animation();

	_image = img;
	_curr_page = 0;
	_screen.reset_cache();
	_screen.clear();
	_screen.rescale(_image->get_page_image(0), settings::open_scale);

	update_title(_image->get_type() != image::animation);
	if (_image->get_type() == image::animation)
		start_animation();
}


void viewer::open_page(const bool forward)
{
	if (_screen.overlay_active())
		stop_animation();
	else {
		const UINT width = _image->get_page_image(_curr_page)->GetWidth();
		const UINT height = _image->get_page_image(_curr_page)->GetHeight();
		if (forward)
			_curr_page = (_curr_page >= _image->get_pages_count() - 1 ? 0 : _curr_page + 1);
		else
			_curr_page = (_curr_page > 0 ? _curr_page - 1 : _image->get_pages_count() - 1);
		if (width != _image->get_page_image(_curr_page)->GetWidth() || height != _image->get_page_image(_curr_page)->GetHeight()) {
			_screen.clear();
			_screen.rescale(_image->get_page_image(_curr_page), settings::open_scale);
		}
	}
}

void viewer::redraw()
{
	_draw_count = 0;
	draw();
}

void viewer::draw()
{
	if (_image == nullptr)
		return;	//Image already closed
	if (_screen.overlay_active())
		return;
	if (_menu_mode)
		return;

	if (++_draw_count == 1) {
		_PSI.Text(0, 0, nullptr, nullptr); //Flush buffer
		Sleep(10); //!!! HACK -- without that small images cleared after first draw (I DON'T KNOW WHY)
	}
	_screen.draw(_image->get_page_image(_curr_page));
}


void viewer::update_title(const bool show_page_num) const
{
	assert(_dialog != INVALID_HANDLE_VALUE);

	if (_image == nullptr)
		return;	//Image already closed

	std::wstring title = _PSI.GetMsg(&_FPG, ps_title);
	title += L": ";
	title += _FSF.PointToName(_image->get_file_name());
	title += L' ';

	wchar_t sz[16];
	swprintf_s(sz, L"[%dx%d]", _image->get_page_image(_curr_page)->GetWidth(), _image->get_page_image(_curr_page)->GetHeight());
	title += sz;
	title += L' ';

	if (show_page_num && _image->get_pages_count() > 1) {
		wchar_t pnum[16];
		swprintf_s(pnum, L"%d of %d", (int)_curr_page + 1, (int)_image->get_pages_count());
		title += pnum;
		title += L", ";
	}

	wchar_t scale[16];
	swprintf_s(scale, L"%d%%", _screen.get_scale());
	title += scale;

	_PSI.SendDlgMessage(_dialog, DM_SETTEXTPTR, 0, const_cast<wchar_t*>(title.c_str()));
}


operations::oper viewer::show_menu()
{
	_menu_mode = true;

	stop_animation();

	std::wstring titles[sizeof(operations::_ops) / sizeof(operations::_ops[0])];

	size_t max_title_len = 0;
	for (size_t i = 0; i < sizeof(operations::_ops) / sizeof(operations::_ops[0]); ++i) {
		if (operations::_ops[i].op_id != operations::op_none)
			titles[i] = _PSI.GetMsg(&_FPG, operations::_ops[i].name_id);
		if (max_title_len < titles[i].length())
			max_title_len = titles[i].length();
	}
	++max_title_len;

	//Add hot key info and create far list
	FarMenuItem item_list[sizeof(operations::_ops) / sizeof(operations::_ops[0])];
	ZeroMemory(item_list, sizeof(item_list));
	for (size_t i = 0; i < sizeof(operations::_ops) / sizeof(operations::_ops[0]); ++i) {
		if (operations::_ops[i].op_id == operations::op_none)
			item_list[i].Flags = MIF_SEPARATOR;
		else {
			titles[i].resize(max_title_len, L' ');
			if (operations::_ops[i].key_label)
				titles[i] += operations::_ops[i].key_label;
			item_list[i].Text = titles[i].c_str();
		}
	}

	const intptr_t chosen_num = _PSI.Menu(&_FPG, &_FPG, -1, -1, 0, FMENU_WRAPMODE,
		_PSI.GetMsg(&_FPG, ps_title),
		nullptr, nullptr, nullptr, nullptr,
		item_list, sizeof(operations::_ops) / sizeof(operations::_ops[0]));

	_menu_mode = false;
	return chosen_num < 0 ? operations::op_none : operations::_ops[chosen_num].op_id;
}


operations::oper viewer::translate_event(const INPUT_RECORD* ev) const
{
	assert(ev);

	operations::oper op = operations::op_none;

	if (ev->EventType == MOUSE_EVENT) {
		if (ev->Event.MouseEvent.dwButtonState == RIGHTMOST_BUTTON_PRESSED)
			op = operations::op_close;
		if (ev->Event.MouseEvent.dwEventFlags == MOUSE_WHEELED)
			op = (static_cast<int>(ev->Event.MouseEvent.dwButtonState) >> 16) < 0 ? operations::op_open_next_image : operations::op_open_prev_image;
	}
	else if (ev->EventType == KEY_EVENT && ev->Event.KeyEvent.bKeyDown) {
		DWORD kstate = (ev->Event.KeyEvent.dwControlKeyState & (OP_ALT | OP_CTRL | OP_SHIFT));
		const DWORD kcode = ev->Event.KeyEvent.wVirtualKeyCode;
		for (size_t i = 0; i < sizeof(operations::_ops) / sizeof(operations::_ops[0]); ++i) {
			for (size_t j = 0; j < sizeof(operations::_ops->hot_key) / sizeof(operations::_ops->hot_key[0]); ++j) {
				if (kcode == operations::_ops[i].hot_key[j].key_code && kstate == operations::_ops[i].hot_key[j].key_state)
					op = operations::_ops[i].op_id;
			}
		}
	}

	return op;
}


void viewer::handle_operation(const operations::oper op)
{
	Gdiplus::Image* img = _image->get_page_image(_curr_page);

	switch (op) {
		case operations::op_open_first_image:
			open_image(_image_list.first());
			break;
		case operations::op_open_last_image:
			open_image(_image_list.last());
			break;
		case operations::op_open_next_image:
			open_image(_image_list.next());
			break;
		case operations::op_opensel_next_image:
			_image_list.set_selection();
			open_image(_image_list.next());
			break;
		case operations::op_open_prev_image:
			open_image(_image_list.previous());
			break;
		case operations::op_show_next_page:
			open_page(true);
			break;
		case operations::op_show_prev_page:
			open_page(false);
			break;

		case operations::op_scale_optimal:		_screen.rescale(img, screen::sc_optimal); break;
		case operations::op_scale_fit:			_screen.rescale(img, screen::sc_fitscr); break;
		case operations::op_scale_increase:		_screen.rescale(img, screen::sc_increase); break;
		case operations::op_scale_decrease:		_screen.rescale(img, screen::sc_decrease); break;
		case operations::op_scale_set10:		_screen.rescale(img, screen::sc_10); break;
		case operations::op_scale_set20:		_screen.rescale(img, screen::sc_20); break;
		case operations::op_scale_set30:		_screen.rescale(img, screen::sc_30); break;
		case operations::op_scale_set40:		_screen.rescale(img, screen::sc_40); break;
		case operations::op_scale_set50:		_screen.rescale(img, screen::sc_50); break;
		case operations::op_scale_set60:		_screen.rescale(img, screen::sc_60); break;
		case operations::op_scale_set70:		_screen.rescale(img, screen::sc_70); break;
		case operations::op_scale_set80:		_screen.rescale(img, screen::sc_80); break;
		case operations::op_scale_set90:		_screen.rescale(img, screen::sc_90); break;
		case operations::op_scale_set100:		_screen.rescale(img, screen::sc_100); break;

		case operations::op_move_step_left:		_screen.move(img, screen::md_left, screen::mt_step); break;
		case operations::op_move_step_right:	_screen.move(img, screen::md_right, screen::mt_step); break;
		case operations::op_move_step_top:		_screen.move(img, screen::md_top, screen::mt_step); break;
		case operations::op_move_step_bottom:	_screen.move(img, screen::md_bottom, screen::mt_step); break;
		case operations::op_move_screen_left:	_screen.move(img, screen::md_left, screen::mt_screen); break;
		case operations::op_move_screen_right:	_screen.move(img, screen::md_right, screen::mt_screen); break;
		case operations::op_move_screen_top:	_screen.move(img, screen::md_top, screen::mt_screen); break;
		case operations::op_move_screen_bottom:	_screen.move(img, screen::md_bottom, screen::mt_screen); break;
		case operations::op_move_image_left:	_screen.move(img, screen::md_left, screen::mt_image); break;
		case operations::op_move_image_right:	_screen.move(img, screen::md_right, screen::mt_image); break;
		case operations::op_move_image_top:		_screen.move(img, screen::md_top, screen::mt_image); break;
		case operations::op_move_image_bottom:	_screen.move(img, screen::md_bottom, screen::mt_image); break;

		case operations::op_rotate_right:
			img->RotateFlip(Gdiplus::Rotate90FlipNone);
			_screen.clear();
			_screen.rescale(img, settings::open_scale);
			break;
		case operations::op_rotate_left:
			img->RotateFlip(Gdiplus::Rotate270FlipNone);
			_screen.clear();
			_screen.rescale(img, settings::open_scale);
			break;
		case operations::op_flip_vert:
			img->RotateFlip(Gdiplus::RotateNoneFlipY);
			break;
		case operations::op_flip_hor:
			img->RotateFlip(Gdiplus::RotateNoneFlipX);
			break;
		case operations::op_tile_mode:
			_screen.switch_tile_mode();
			break;
		case operations::op_close:
			close();
			break;
		case operations::op_none:
			break;
		default:
			assert(false && "Unknown operation");
	}

	if (op != operations::op_none) {
		update_title(!_screen.overlay_active());
		draw();
	}
}


void viewer::dlg_handle_operation(const operations::oper op)
{
	if (op == operations::op_fullscreen_mode)
		enable_fullscreen();
	else if (op == operations::op_delete_image) {
		stop_animation();
		open_image(_image_list.delete_current_file());
	}
	else
		handle_operation(op);
}


void viewer::start_animation()
{
	_screen.start_draw_overlay(_image);
}


void viewer::stop_animation()
{
	if (_screen.overlay_active()) {
		_curr_page = _screen.stop_draw_overlay();
		update_title(true);
	}
}


void viewer::enable_fullscreen()
{
	assert(_fs_wnd == nullptr);

	const wchar_t* wnd_class_name = L"far_imageview_wnd_class";
	HWND far_wnd = reinterpret_cast<HWND>(_PSI.AdvControl(&_FPG, ACTL_GETFARHWND, 0, nullptr));

	HBRUSH bkg_brush = nullptr;
	try {
		bkg_brush = CreateSolidBrush(0);

		WNDCLASSEX wcex;
		ZeroMemory(&wcex, sizeof(wcex));
		wcex.cbSize = sizeof(wcex);
		wcex.style			= CS_HREDRAW | CS_VREDRAW;
		wcex.lpfnWndProc	= &viewer::wnd_proc;
		wcex.hCursor		= LoadCursor(nullptr, IDC_ARROW);
		wcex.hbrBackground	= bkg_brush;
		wcex.lpszClassName	= wnd_class_name;
		RegisterClassEx(&wcex);

		MONITORINFO monitor_info;
		ZeroMemory(&monitor_info, sizeof(monitor_info));
		monitor_info.cbSize = sizeof(monitor_info);
		if (!GetMonitorInfo(MonitorFromWindow(far_wnd, MONITOR_DEFAULTTONEAREST), &monitor_info))
			throw GetLastError();

		_fs_wnd = CreateWindowEx(WS_EX_NOPARENTNOTIFY | WS_EX_TOPMOST, wnd_class_name, nullptr, 0,
			0, 0, 0, 0, far_wnd, nullptr, nullptr, nullptr);
		if (!_fs_wnd)
			throw GetLastError();

		SetWindowLongPtr(_fs_wnd, GWL_STYLE, GetWindowLongPtr(_fs_wnd, GWL_STYLE) & ~(WS_CAPTION | WS_THICKFRAME));
		SetWindowLongPtr(_fs_wnd, GWL_EXSTYLE, GetWindowLongPtr(_fs_wnd, GWL_EXSTYLE) & ~(WS_EX_DLGMODALFRAME |	WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE));
		SetWindowLongPtr(_fs_wnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

		SetWindowPos(_fs_wnd, nullptr,
			monitor_info.rcMonitor.left, monitor_info.rcMonitor.top,
			monitor_info.rcMonitor.right - monitor_info.rcMonitor.left, monitor_info.rcMonitor.bottom - monitor_info.rcMonitor.top,
			SWP_NOZORDER | SWP_FRAMECHANGED);

		_screen.set_window(_fs_wnd);
		_screen.rescale(_image->get_page_image(_curr_page), settings::open_scale);

		UpdateWindow(_fs_wnd);
		ShowWindow(_fs_wnd, SW_SHOW);
		SetForegroundWindow(_fs_wnd);
		SetActiveWindow(_fs_wnd);

		MSG msg;
		while (GetMessage(&msg, nullptr, 0, 0)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	catch (...) {
	}

	_fs_wnd = nullptr;
	_screen.set_window(screen::far_window());
	SetForegroundWindow(screen::far_window());
	SetActiveWindow(screen::far_window());
	if (_image)
		_screen.rescale(_image->get_page_image(_curr_page), settings::open_scale);

	if (bkg_brush)
		DeleteObject(bkg_brush);
	UnregisterClass(wnd_class_name, nullptr);
}


intptr_t WINAPI viewer::dlg_proc(HANDLE dlg, intptr_t msg, intptr_t param1, void* param2)
{
	viewer* instance = nullptr;
	if (msg != DN_INITDIALOG)
		instance = reinterpret_cast<viewer*>(_PSI.SendDlgMessage(dlg, DM_GETDLGDATA, 0, nullptr));
	else {
		instance = reinterpret_cast<viewer*>(param2);
		_PSI.SendDlgMessage(dlg, DM_SETDLGDATA, 0, instance);
		_PSI.SendDlgMessage(dlg, DM_SETMOUSEEVENTNOTIFY, 1, nullptr);
		instance->open_image(instance->_image);
	}
	assert(instance);

	switch (msg) {
		case DN_DRAGGED:
			return FALSE;
		case DN_CTLCOLORDIALOG:
			if (param2)
				reinterpret_cast<FarColor*>(param2)->ForegroundColor = reinterpret_cast<FarColor*>(param2)->BackgroundColor = 0;
			return TRUE;
		case DN_CTLCOLORDLGITEM:
			if (param2) {
				for (size_t i = 0; i < reinterpret_cast<FarDialogItemColors*>(param2)->ColorsCount; ++i)
					reinterpret_cast<FarDialogItemColors*>(param2)->Colors[i].ForegroundColor = reinterpret_cast<FarDialogItemColors*>(param2)->Colors[i].BackgroundColor = 0;
			}
			return TRUE;
		case DN_DRAWDLGITEM:
			return FALSE;
		case DN_ACTIVATEAPP:
		case DN_DRAWDIALOGDONE:
			instance->draw();
			return FALSE;
		case DN_CONTROLINPUT:
		case DN_INPUT:
			if (param2) {
				const INPUT_RECORD* ev = reinterpret_cast<const INPUT_RECORD*>(param2);
				operations::oper op = instance->translate_event(ev);
				if (op == operations::op_none && ev->EventType == KEY_EVENT && ev->Event.KeyEvent.bKeyDown && ev->Event.KeyEvent.wVirtualKeyCode >= VK_F1 && ev->Event.KeyEvent.wVirtualKeyCode <= VK_F9)
				{
					if (msg == DN_CONTROLINPUT)
						return TRUE;
					else {
						op = instance->show_menu();
						if (op == operations::op_none) {
							instance->redraw();
							return TRUE;
						}
					}
				}
				if (op != operations::op_none) {
					if (msg != DN_CONTROLINPUT)
						instance->dlg_handle_operation(op);
					return TRUE;
				}
			}
			break;
		case DN_RESIZECONSOLE:
			if (param2) {
				_PSI.SendDlgMessage(dlg, DM_RESIZEDIALOG, 0, reinterpret_cast<COORD*>(param2));
				instance->_screen.window_resize();
				instance->_screen.rescale(instance->_image->get_page_image(instance->_curr_page), settings::open_scale);
				instance->draw();
			}
			break;
	}

	return _PSI.DefDlgProc(dlg, msg, param1, param2);
}


LRESULT CALLBACK viewer::wnd_proc(HWND wnd, UINT message, WPARAM wparam, LPARAM lparam)
{
	viewer* instance = reinterpret_cast<viewer*>(GetWindowLongPtr(wnd, GWLP_USERDATA));

	switch (message) {
		case WM_MBUTTONDOWN:
		case WM_MOUSEWHEEL:
			{
				INPUT_RECORD rec;
				ZeroMemory(&rec, sizeof(rec));
				rec.EventType = MOUSE_EVENT;
				switch (GET_KEYSTATE_WPARAM(wparam)) {
					case MK_LBUTTON:	rec.Event.MouseEvent.dwButtonState = FROM_LEFT_1ST_BUTTON_PRESSED; break;
					case MK_MBUTTON:	rec.Event.MouseEvent.dwButtonState = FROM_LEFT_2ND_BUTTON_PRESSED; break;
					case MK_RBUTTON:	rec.Event.MouseEvent.dwButtonState = RIGHTMOST_BUTTON_PRESSED; break;
				}
				if (message == WM_MOUSEWHEEL) {
					rec.Event.MouseEvent.dwEventFlags = MOUSE_WHEELED;
					rec.Event.MouseEvent.dwButtonState |= GET_WHEEL_DELTA_WPARAM(wparam);
				}
				instance->handle_operation(instance->translate_event(&rec));
			}
			break;
		case WM_SYSKEYDOWN:
		case WM_KEYDOWN:
			{
				INPUT_RECORD rec;
				ZeroMemory(&rec, sizeof(rec));
				rec.EventType = KEY_EVENT;
				rec.Event.KeyEvent.bKeyDown = TRUE;
				rec.Event.KeyEvent.wVirtualKeyCode = static_cast<WORD>(wparam);
				const unsigned short alt_state = GetKeyState(VK_MENU);
				const unsigned short ctrl_state = GetKeyState(VK_CONTROL);
				const unsigned short shift_state = GetKeyState(VK_SHIFT);
				if (HIBYTE(alt_state))
					rec.Event.KeyEvent.dwControlKeyState |= OP_ALT;
				if (HIBYTE(ctrl_state))
					rec.Event.KeyEvent.dwControlKeyState |= OP_CTRL;
				if (HIBYTE(shift_state))
					rec.Event.KeyEvent.dwControlKeyState |= OP_SHIFT;
				const operations::oper op = instance->translate_event(&rec);
				if (op == operations::op_fullscreen_mode)
					DestroyWindow(wnd);
				else if (op != operations::op_delete_image)
					instance->handle_operation(op);
			}
			break;

		case WM_PAINT:
			if (!instance->_screen.overlay_active()) {
				PAINTSTRUCT p;
				BeginPaint(wnd, &p);
				instance->draw();
				EndPaint(wnd, &p);
			}
			break;
		case WM_DESTROY:
			PostQuitMessage(0);
			break;
		default:
			return DefWindowProc(wnd, message, wparam, lparam);
	}
	return 0;
}
