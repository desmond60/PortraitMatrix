#pragma once

#include "screen.h"
#include "image.h"


class viewer_qv
{
private:
	viewer_qv();
	~viewer_qv();

public:
	/**
	 * Get instance.
	 * \return viewer instance
	 */
	static viewer_qv* instance();

	/**
	 * Show image in quick view.
	 * \param img image
	 * \param pi quick view panel description
	 */
	void show(image* img, const PanelInfo& pi);

	/**
	 * Hide image in quick view.
	 */
	void hide();

private:
	/**
	 * Clear background.
	 */
	void clear_bkg() const;

private:
	image*		_image;
	screen*		_screen;
	RECT		_wnd_rect;
	PanelInfo	_panel_info;
};
