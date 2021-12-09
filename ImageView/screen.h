#pragma once

#include "common.h"
#include "image.h"
#include <GdiPlus.h>


class screen
{
public:
	//! Scale change types.
	enum scale {
		sc_10,			//Set scale to 10%
		sc_20,			//Set scale to 20%
		sc_30,			//Set scale to 30%
		sc_40,			//Set scale to 40%
		sc_50,			//Set scale to 50%
		sc_60,			//Set scale to 60%
		sc_70,			//Set scale to 70%
		sc_80,			//Set scale to 80%
		sc_90,			//Set scale to 90%
		sc_100,			//Set scale to 100%
		sc_optimal,		//Set optimal scale (100% or fit to screen size)
		sc_optimal125,	//Set optimal scale (125% or fit to screen size)
		sc_optimal150,	//Set optimal scale (150% or fit to screen size)
		sc_fitscr,		//Fit to screen size
		sc_increase,	//Increase by step
		sc_decrease		//Decrease by step
	};

	//! Move direction types.
	enum move_direction {
		md_left,
		md_right,
		md_top,
		md_bottom
	};

	//! Move step types.
	enum move_step {
		mt_step,
		mt_screen,
		mt_image
	};

	//! Background types.
	enum bkg_type {
		bt_solid,
		bt_grid
	};

	/**
	 * Constructor.
	 * \param rect window rect (nullptr to use whole window area)
	 */
	screen(const RECT* rect = nullptr);

	~screen();

	/**
	 * Set window to draw.
	 * \param wnd new window
	 */
	void set_window(const HWND wnd);

	/**
	 * Window resize handler.
	 * \param rect new window rect (nullptr to use whole window area)
	 */
	void window_resize(const RECT* rect = nullptr);

	/**
	 * Reset image cache.
	 */
	void reset_cache();

	/**
	 * Setup background.
	 * \param bt background type
	 * \param val depends on background type, COLORREF for bt_solid or grid step for bt_grid
	 */
	void set_background(const bkg_type bt, const DWORD val);

	/**
	 * Setup interpolation mode.
	 * \param imode interpolation mode
	 */
	void set_interpolation(const Gdiplus::InterpolationMode imode);

	/**
	 * Switch tile mode on/off.
	 */
	void switch_tile_mode();

	/**
	 * Clear the screen.
	 * \param img source image (null to clear whole draw area)
	 */
	void clear(Gdiplus::Image* img = nullptr);

	/**
	 * Draw image.
	 * \param img image to draw
	 */
	void draw(Gdiplus::Image* img);

	/**
	 * Rescale.
	 * \param img showed image
	 * \param sc_type scale operation
	 * \return true if image was scaled
	 */
	bool rescale(Gdiplus::Image* img, const scale scale_op);

	/**
	 * Move on image.
	 * \param img showed image
	 * \param md move direction
	 * \param mt move type
	 * \return true if image was moved
	 */
	bool move(Gdiplus::Image* img, const move_direction md, const move_step mt);

	/**
	 * Get scale in percent.
	 * \return scale in percent
	 */
	int get_scale() const { return static_cast<int>(floor(_scale * 100.0f)); }

	/**
	 * Start draw image in overlay mode.
	 * \param img image to draw
	 */
	void start_draw_overlay(image* img);

	/**
	 * Stop draw image in overlay mode.
	 * \return last showed frame (for animated images only)
	 */
	size_t stop_draw_overlay();

	/**
	 * Check for overlay drawing mode activity.
	 * \return true if overlay mode active
	 */
	inline bool overlay_active() const	{ return _ov_thread != nullptr; }

	/**
	 * Ge Far window handle.
	 * \return Far window handle
	 */
	static HWND far_window();

private:
	//! Move coordinates type.
	enum move_coord {
		mc_absolute,
		mc_relative
	};

	/**
	 * Move by horizontal.
	 * \param img_width showed image width size
	 * \param val position relative step or absolute value
	 * \param mc move type
	 * \return true if image was moved
	 */
	bool move_x(const int img_width, const int val, const move_coord mc);

	/**
	 * Update vertical.
	 * \param img_height showed image height size
	 * \param val position relative step or absolute value
	 * \param mc move type
	 * \return true if image was moved
	 */
	bool move_y(const int img_height, const int val, const move_coord mc);

	/**
	 * Prepare image to draw.
	 * \param src source image
	 * \param src_rect source image draw rect
	 * \param dst_rect destination draw rect
	 * \return image to show
	 */
	Gdiplus::Image* prepare_image(Gdiplus::Image* src, Gdiplus::Rect& src_rect, Gdiplus::Rect& dst_rect);

	/**
	 * Create canvas.
	 * \param use_grid use grid flag
	 * \param width canvas width size
	 * \param height canvas height size
	 * \return canvas image
	 */
	Gdiplus::Image* create_canvas(const bool use_grid, const int width, const int height) const;

	/**
	 * Fill image by tile.
	 * \param canvas canvas image (output)
	 * \param src source image
	 */
	void fill_tile(Gdiplus::Image* canvas, Gdiplus::Image* src) const;

	//! Overlay draw thread procedure.
	static DWORD WINAPI overlay_thread(PVOID param);

private:
	//! Cache image.
	struct chache_item {
		Gdiplus::Image* img;
		Gdiplus::Rect src_rect;
		Gdiplus::Rect dst_rect;
	};
	typedef std::map<Gdiplus::Image*, chache_item> cache;

private:
	HWND	_wnd;							///< Window to draw
	RECT	_rect;							///< Draw area coordinates

	Gdiplus::Graphics*			_graphics;	///< Graphics
	Gdiplus::InterpolationMode	_imode;		///< Interpolation mode

	CRITICAL_SECTION			_sync;		///< Synchronization

	cache			_cache;					///< Cached image (source, showed)

	bkg_type		_bkg_type;				///< Background type
	Gdiplus::Color	_bkg_color;				///< Background color
	int				_bkg_gstep;				///< Background grid step

	float	_scale;							///< Scale factor in range [0...1]
	int		_pos_x;							///< Left position of the image
	int		_pos_y;							///< Top position of the image

	bool	_tile_mode;						///< Tile mode

	HANDLE	_ov_thread;						///< Show overlay image thread
	HANDLE	_ov_stop;						///< Show overlay image stop event
	image*	_ov_image;						///< Overlay image to draw
};
