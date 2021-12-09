#pragma once

#include "common.h"
#include "screen.h"
#include <GdiPlus.h>


class settings
{
public:
	/**
	 * Load settings.
	 */
	static void load();

	/**
	 * Configure settings.
	 */
	static void configure();

private:
	/**
	 * Save settings.
	 */
	static void save();

public:
	static screen::scale open_scale;		///< Initial scale on image open
	static Gdiplus::InterpolationMode interpolate;	///< Interpolate mode
	static int bg_grid_step;				///< Background grid (0 if disabled)
	static bool use_exif;					///< Use EXIF orientation
	static size_t ra_size;						///< Size of the buffer for read-ahead
	static bool ask_delete;					///< Ask before delete file
	static bool use_analyze;				///< Use analyze mode (open by Enter)
	static bool use_view;					///< Use view mode by F3
	static bool use_quickview;				///< Use quick view mode
	static bool add_to_panel_menu;			///< Add plug-in to the panel plug-in menu flag
	static std::wstring cmd_prefix;				///< Plugin command prefix
};
