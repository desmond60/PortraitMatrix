#pragma once

#include "common.h"
#include "string_rc.h"

#define OP_ALT   (RIGHT_ALT_PRESSED | LEFT_ALT_PRESSED)
#define OP_CTRL  (RIGHT_CTRL_PRESSED | LEFT_CTRL_PRESSED)
#define OP_SHIFT SHIFT_PRESSED


namespace operations {

	//! Supported operations.
	enum oper {
		op_none,
		op_open_first_image,
		op_open_last_image,
		op_open_next_image,
		op_opensel_next_image,
		op_open_prev_image,
		op_delete_image,
		op_show_next_page,
		op_show_prev_page,
		op_scale_optimal,
		op_scale_fit,
		op_scale_increase,
		op_scale_decrease,
		op_scale_set10,
		op_scale_set20,
		op_scale_set30,
		op_scale_set40,
		op_scale_set50,
		op_scale_set60,
		op_scale_set70,
		op_scale_set80,
		op_scale_set90,
		op_scale_set100,
		op_move_step_left,
		op_move_step_right,
		op_move_step_top,
		op_move_step_bottom,
		op_move_screen_left,
		op_move_screen_right,
		op_move_screen_top,
		op_move_screen_bottom,
		op_move_image_left,
		op_move_image_right,
		op_move_image_top,
		op_move_image_bottom,
		op_fullscreen_mode,
		op_rotate_right,
		op_rotate_left,
		op_flip_vert,
		op_flip_hor,
		op_tile_mode,
		op_close
	};

	//! Hot key description
	struct hkey {
		WORD		key_code;
		DWORD		key_state;
	};

	//! Operation description
	struct ops {
		oper			op_id;
		intptr_t		name_id;
		const wchar_t*	key_label;
		hkey			hot_key[5];
	};

	//! Supported operations
	static const ops _ops[] = {
		{ op_open_first_image,   ps_op_open_first_image,   L"Home",   { { VK_HOME, 0 }, { VK_NUMPAD7, 0 } } },
		{ op_open_last_image,	 ps_op_open_last_image,    L"End",    { { VK_END, 0 },  { VK_NUMPAD1, 0 } } },
		{ op_open_next_image,	 ps_op_open_next_image,    L"Space, Page Down",  { { VK_SPACE, 0 } , { VK_NEXT, 0 }, { VK_NUMPAD3, 0 } } },
		{ op_opensel_next_image, ps_op_opensel_next_image, L"Insert", { { VK_INSERT, 0 },  { VK_NUMPAD0, 0 } } },
		{ op_open_prev_image,	 ps_op_open_prev_image,    L"Shift+Space, Backspace, Page Up", { { VK_SPACE, OP_SHIFT }, { VK_BACK, 0 }, { VK_PRIOR, 0 }, { VK_NUMPAD9, 0 } } },
		{ op_delete_image,		 ps_op_delete_image,       L"Delete, F8", { { VK_DELETE, 0 }, { VK_F8, 0 } } },
		{ op_none },
		{ op_show_next_page,	 ps_op_show_next_page,     L"Shift+Down, Shift+Right", { { VK_DOWN, OP_SHIFT }, { VK_RIGHT, OP_SHIFT } } },
		{ op_show_prev_page,	 ps_op_show_prev_page,     L"Shift+Up, Shift+Left",    { { VK_UP, OP_SHIFT },   { VK_LEFT, OP_SHIFT }  } },
		{ op_none },
		{ op_scale_optimal,      ps_op_scale_optimal,      L"&~, /", { { VK_OEM_3, 0 }, { VK_OEM_2, 0 }, { VK_DIVIDE, 0 } } },
		{ op_scale_fit,          ps_op_scale_fit,          L"&*", { { VK_MULTIPLY, 0 }, { '8', OP_SHIFT } } },
		{ op_scale_increase,	 ps_op_scale_increase,     L"&+", { { VK_ADD, 0 },      { VK_OEM_PLUS, 0 } } },
		{ op_scale_decrease,	 ps_op_scale_decrease,     L"&-", { { VK_SUBTRACT, 0 }, { VK_OEM_MINUS, 0 } } },
		{ op_scale_set10,        ps_op_scale_set10,        L"&1", { { '1', 0 } } },
		{ op_scale_set20,        ps_op_scale_set20,        L"&2", { { '2', 0 } } },
		{ op_scale_set30,        ps_op_scale_set30,        L"&3", { { '3', 0 } } },
		{ op_scale_set40,        ps_op_scale_set40,        L"&4", { { '4', 0 } } },
		{ op_scale_set50,        ps_op_scale_set50,        L"&5", { { '5', 0 } } },
		{ op_scale_set60,        ps_op_scale_set60,        L"&6", { { '6', 0 } } },
		{ op_scale_set70,        ps_op_scale_set70,        L"&7", { { '7', 0 } } },
		{ op_scale_set80,        ps_op_scale_set80,        L"&8", { { '8', 0 } } },
		{ op_scale_set90,        ps_op_scale_set90,        L"&9", { { '9', 0 } } },
		{ op_scale_set100,       ps_op_scale_set100,       L"&0", { { '0', 0 } } },
		{ op_none },
		{ op_move_step_left,     ps_op_move_step_left,     L"Left",       { { VK_LEFT,  0 },       { VK_NUMPAD4, 0 }       } },
		{ op_move_step_right,    ps_op_move_step_right,    L"Right",      { { VK_RIGHT, 0 },       { VK_NUMPAD6, 0 }       } },
		{ op_move_step_top,      ps_op_move_step_top,      L"Up",         { { VK_UP,    0 },       { VK_NUMPAD8, 0 }       } },
		{ op_move_step_bottom,   ps_op_move_step_bottom,   L"Down",       { { VK_DOWN,  0 },       { VK_NUMPAD2, 0 }       } },
		{ op_move_screen_left,   ps_op_move_screen_left,   L"Alt+Left",   { { VK_LEFT,  OP_ALT  }, { VK_NUMPAD4, OP_ALT }  } },
		{ op_move_screen_right,  ps_op_move_screen_right,  L"Alt+Right",  { { VK_RIGHT, OP_ALT  }, { VK_NUMPAD6, OP_ALT }  } },
		{ op_move_screen_top,    ps_op_move_screen_top,    L"Alt+Up",     { { VK_UP,    OP_ALT  }, { VK_NUMPAD8, OP_ALT }  } },
		{ op_move_screen_bottom, ps_op_move_screen_bottom, L"Alt+Down",   { { VK_DOWN,  OP_ALT  }, { VK_NUMPAD2, OP_ALT }  } },
		{ op_move_image_left,    ps_op_move_image_left,    L"Ctrl+Left",  { { VK_LEFT,  OP_CTRL }, { VK_NUMPAD4, OP_CTRL } } },
		{ op_move_image_right,   ps_op_move_image_right,   L"Ctrl+Right", { { VK_RIGHT, OP_CTRL }, { VK_NUMPAD6, OP_CTRL } } },
		{ op_move_image_top,     ps_op_move_image_top,     L"Ctrl+Up",    { { VK_UP,    OP_CTRL }, { VK_NUMPAD8, OP_CTRL } } },
		{ op_move_image_bottom,  ps_op_move_image_bottom,  L"Ctrl+Down",  { { VK_DOWN,  OP_CTRL }, { VK_NUMPAD2, OP_CTRL } } },
		{ op_none },
		{ op_fullscreen_mode,    ps_op_fullscreen_mode,    L"F11",        { { VK_F11, 0 } } },
		{ op_none },
		{ op_rotate_right,       ps_op_rotate_right,       L"&R",         { { 'R', 0 } } },
		{ op_rotate_left,        ps_op_rotate_left,        L"Shift+R",    { { 'R', OP_SHIFT } } },
		{ op_flip_hor,           ps_op_flip_hor,           L"&F",         { { 'F', 0 } } },
		{ op_flip_vert,          ps_op_flip_vert,          L"Shift+F",    { { 'F', OP_SHIFT } } },
		{ op_none },
		{ op_tile_mode,          ps_op_tile_mode,          L"&T",         { { 'T', 0 } } },
		{ op_none },
		{ op_close,              ps_op_close,              L"Escape, Enter, F3, F10", { { VK_ESCAPE, 0 }, { VK_RETURN, 0 }, { VK_F10, 0 }, { VK_F3, 0 }, { VK_CLEAR, 0 } } }
	};
}
