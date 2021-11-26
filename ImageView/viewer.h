#pragma once

#include "common.h"
#include "image.h"
#include "operations.h"
#include "image_list.h"
#include "screen.h"
#include <GdiPlus.h>


class viewer
{
public:
	/**
	 * Constructor.
	 * \param img showed image
	 */
	viewer(image* img);

	~viewer();

	/**
	 * Show image in internal dialog mode.
	 */
	void show();

private:
	/**
	 * Close viewer.
	 */
	void close();

	/**
	 * Open image.
	 * \param img new image
	 */
	void open_image(image* img);

	/**
	 * Open next page from image.
	 * \param direction (true = forward, false = backward)
	 */
	void open_page(const bool forward);

	/**
	 * Draw image.
	 */
	void draw();
	void redraw();

	/**
	 * Update window title.
	 * \param show page numbers
	 */
	void update_title(const bool show_page_num) const;

	/**
	 * Show operations menu.
	 * \return chosen operation id
	 */
	operations::oper show_menu();

	/**
	 * Translate event.
	 * \param rec event
	 * \return operation
	 */
	operations::oper translate_event(const INPUT_RECORD* ev) const;

	/**
	 * Handle operation.
	 * \param op operation
	 */
	void handle_operation(const operations::oper op);

	/**
	 * Handle operation in dialog mode.
	 * \param op operation
	 */
	void dlg_handle_operation(const operations::oper op);

	/**
	 * Start animation.
	 */
	void start_animation();

	/**
	 * Stop animation.
	 */
	void stop_animation();

	/**
	 * Switch to full screen mode.
	 */
	void enable_fullscreen();

private:
	//! Far dialog's callback.
	static intptr_t WINAPI dlg_proc(HANDLE dlg, intptr_t msg, intptr_t param1, void* param2);

	//! Processes messages for the full screen window.
	static LRESULT CALLBACK wnd_proc(HWND wnd, UINT message, WPARAM wparam, LPARAM lparam);

private:
	image*	_image;			///< Currently showed image
	size_t	_curr_page;		///< Current page number
	screen	_screen;		///< Screen implementation
	HANDLE	_dialog;		///< Far dialog handle
	HWND	_fs_wnd;		///< Full screen mode window handle
	image_list _image_list;	///< Image list

	unsigned _draw_count;
	bool     _menu_mode;
};
