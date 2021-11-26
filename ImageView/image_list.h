#pragma once

#include "common.h"
#include "image.h"
#include <memory>



class image_list
{
public:
	image_list(image* img);
	~image_list();

	/**
	 * Set selection for current file on active panel.
	 */
	void set_selection() const;

	/**
	 * Delete current file.
	 * \return next opened image (nullptr if not found - no more files in folder)
	 */
	image* delete_current_file();

	/**
	 * Get current image.
	 * \return current image
	 */
	image* current()	{ return _current->img; }

	/**
	 * Open next file.
	 * \return opened image (nullptr if not found)
	 */
	image* next()		{ step(_current->index, d_forward); return _current->img; }

	/**
	 * Open previous file.
	 * \return opened image (nullptr if not found)
	 */
	image* previous()	{ step(_current->index, d_backward); return _current->img; }

	/**
	 * Open first file.
	 * \return opened image (nullptr if not found)
	 */
	image* first()		{ step(-1, d_forward); return _current->img; }

	/**
	 * Open last file.
	 * \return opened image (nullptr if not found)
	 */
	image* last();

private:
	//! Image item description.
	struct image_item_obj {
		image_item_obj();
		~image_item_obj();
		image*		img;
		intptr_t	index;
		bool		delete_on_close;
	};
	typedef std::shared_ptr<image_item_obj> image_item;
	typedef std::list<image_item> cache;

	//! Step direction.
	enum direction {
		d_forward,
		d_backward
	};

	/**
	 * Initialization.
	 * \param file_name file name
	 * \return file index on panel (-1 if not found)
	 */
	intptr_t initialize(const wchar_t* file_name);

	/**
	 * Step trough file list.
	 * \param dir direction
	 * \param start_index start index
	 * \return opened image (nullptr if not found)
	 */
	image* step(const intptr_t start_index, const direction dir);

	/**
	 * Open image.
	 * \param file_name file name
	 * \param file_index file index
	 * \param ii output item
	 * \return false if error opening image
	 */
	bool open_image(const wchar_t* file_name, const intptr_t file_index, image_item& ii) const;

	/**
	 * Get next file from panel.
	 * \param start_index start index
	 * \param dir search direction
	 * \param file_name output file name
	 * \param file_index output file index
	 * \return false if no more images found
	 */
	bool get_next_file(const intptr_t start_index, const direction dir, std::wstring& file_name, intptr_t& file_index) const;

	/**
	 * Extract file from external plug-in.
	 * \param index file index
	 * \return path to extracted file (empty on error)
	 */
	std::wstring extract_file(const intptr_t index) const;

	/**
	 * Start read ahead procedure.
	 */
	void start_read_ahead();

	/**
	 * Stop read ahead procedure.
	 */
	void stop_read_ahead();

	/**
	 * Read ahead thread procedure.
	 */
	static DWORD WINAPI read_ahead_thread(PVOID param);

private:
	bool		_plugin_panel;		///< Flag signaled that active panel is external plug-in
	bool		_use_real_path;		///< Flag signaled that active panel is external plug-in with real file paths

	image_item	_current;			///< Current image item
	image_item	_previous;			///< Previous image item
	cache		_next;				///< Next image items

	HANDLE		_ra_thread;			///< Read ahead thread
	HANDLE		_ra_stop;			///< Read ahead stop event
};
