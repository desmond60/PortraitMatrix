#include "settings.h"
#include "string_rc.h"

screen::scale settings::open_scale = screen::sc_optimal;
Gdiplus::InterpolationMode settings::interpolate = Gdiplus::InterpolationModeHighQuality;
int settings::bg_grid_step = 10;
bool settings::use_exif = true;
size_t settings::ra_size = 3;
bool settings::ask_delete = true;
bool settings::use_analyze = true;
bool settings::use_view = false;
bool settings::use_quickview = true;
bool settings::add_to_panel_menu = true;
std::wstring settings::cmd_prefix = L"ImageView";


#define SAVE_SETTINGS(s, p) s.set(L#p, p);
#define LOAD_SETTINGS(s, p) p = s.get(L#p, p);

class settings_serializer
{
public:
	settings_serializer()
		: _handle(INVALID_HANDLE_VALUE)
	{
		FarSettingsCreate fsc;
		ZeroMemory(&fsc, sizeof(fsc));
		fsc.StructSize = sizeof(fsc);
		fsc.Guid = _FPG;
		fsc.Handle = INVALID_HANDLE_VALUE;
		if (_PSI.SettingsControl(INVALID_HANDLE_VALUE, SCTL_CREATE, 0, &fsc))
			_handle = fsc.Handle;
	}

	~settings_serializer()
	{
		_PSI.SettingsControl(_handle, SCTL_FREE, 0, nullptr);
	}
	
	/**
	 * Set value (scalar types).
	 * \param name value name
	 * \param val value
	 * \return false if error
	 */
	template<class T> bool set(const wchar_t* name, const T& val) const
	{
		assert(name && *name);

		FarSettingsItem fsi;
		ZeroMemory(&fsi, sizeof(fsi));
		fsi.StructSize = sizeof(fsi);
		fsi.Name = name;
		fsi.Type = FST_QWORD;
		fsi.Number = static_cast<unsigned __int64>(val);
		return (_PSI.SettingsControl(_handle, SCTL_SET, 0, &fsi) != FALSE);
	}
	
	/**
	 * Set value (string type).
	 * \param name value name
	 * \param val value
	 * \return false if error
	 */
	template<> bool set(const wchar_t* name, const std::wstring& val) const
	{
		assert(name && *name);

		FarSettingsItem fsi;
		ZeroMemory(&fsi, sizeof(fsi));
		fsi.StructSize = sizeof(fsi);
		fsi.Name = name;
		fsi.Type = FST_STRING;
		fsi.String = val.c_str();
		return (_PSI.SettingsControl(_handle, SCTL_SET, 0, &fsi) != FALSE);
	}

	/**
	 * Set value (boolean type).
	 * \param name value name
	 * \param val value
	 * \return false if error
	 */
	template<> bool set(const wchar_t* name, const bool& val) const
	{
		return set<unsigned char>(name, val ? 1 : 0);
	}

	/**
	 * Get value (scalar types).
	 * \param name value name
	 * \param default_val default value
	 * \return value
	 */
	template<class T> T get(const wchar_t* name, const T& default_val) const
	{
		assert(name && *name);

		FarSettingsItem fsi;
		ZeroMemory(&fsi, sizeof(fsi));
		fsi.StructSize = sizeof(fsi);
		fsi.Name = name;
		fsi.Type = FST_QWORD;
		return (_PSI.SettingsControl(_handle, SCTL_GET, 0, &fsi) ? static_cast<T>(fsi.Number) : default_val);
	}

	/**
	 * Get value (string types).
	 * \param name value name
	 * \param default_val default value
	 * \return value
	 */
	template<> std::wstring get(const wchar_t* name, const std::wstring& default_val) const
	{
		assert(name && *name);

		FarSettingsItem fsi;
		ZeroMemory(&fsi, sizeof(fsi));
		fsi.StructSize = sizeof(fsi);
		fsi.Name = name;
		fsi.Type = FST_STRING;
		return (_PSI.SettingsControl(_handle, SCTL_GET, 0, &fsi) ? std::wstring(fsi.String) : default_val);
	}

	/**
	 * Get value (boolean types).
	 * \param name value name
	 * \param default_val default value
	 * \return value
	 */
	template<> bool get(const wchar_t* name, const bool& default_val) const
	{
		return get<unsigned char>(name, default_val ? 1 : 0) != 0;
	}

private:
	HANDLE _handle;
};



void settings::load()
{
	settings_serializer s;
	LOAD_SETTINGS(s, open_scale);
	LOAD_SETTINGS(s, interpolate);
	LOAD_SETTINGS(s, bg_grid_step);
	LOAD_SETTINGS(s, use_exif);
	LOAD_SETTINGS(s, ra_size);
	LOAD_SETTINGS(s, ask_delete);
	LOAD_SETTINGS(s, use_analyze);
	LOAD_SETTINGS(s, use_view);
	LOAD_SETTINGS(s, use_quickview);
	LOAD_SETTINGS(s, add_to_panel_menu);
	LOAD_SETTINGS(s, cmd_prefix);
}


void settings::configure()
{
	const screen::scale scales[] = {
		screen::sc_optimal,
		screen::sc_optimal125,
		screen::sc_optimal150,
		screen::sc_fitscr,
		screen::sc_100
	};
	FarListItem list_scale_open[] = {
		{ open_scale == scales[0] ? LIF_SELECTED : LIF_NONE, _PSI.GetMsg(&_FPG, ps_cfg_sc_opt100) },
		{ open_scale == scales[1] ? LIF_SELECTED : LIF_NONE, _PSI.GetMsg(&_FPG, ps_cfg_sc_opt125) },
		{ open_scale == scales[2] ? LIF_SELECTED : LIF_NONE, _PSI.GetMsg(&_FPG, ps_cfg_sc_opt150) },
		{ open_scale == scales[3] ? LIF_SELECTED : LIF_NONE, _PSI.GetMsg(&_FPG, ps_cfg_sc_fit) },
		{ open_scale == scales[4] ? LIF_SELECTED : LIF_NONE, _PSI.GetMsg(&_FPG, ps_cfg_100) }
	};
	FarList list_so;
	list_so.Items = list_scale_open;
	list_so.ItemsNumber = sizeof(list_scale_open) / sizeof(list_scale_open[0]);

	FarListItem list_interpolate[] = {
		{ LIF_NONE, _PSI.GetMsg(&_FPG, ps_cfg_i_lq) },
		{ LIF_NONE, _PSI.GetMsg(&_FPG, ps_cfg_i_hq) },
		{ LIF_NONE, _PSI.GetMsg(&_FPG, ps_cfg_i_bil) },
		{ LIF_NONE, _PSI.GetMsg(&_FPG, ps_cfg_i_bic) },
		{ LIF_NONE, _PSI.GetMsg(&_FPG, ps_cfg_i_nn) },
		{ LIF_NONE, _PSI.GetMsg(&_FPG, ps_cfg_i_hqbil) },
		{ LIF_NONE, _PSI.GetMsg(&_FPG, ps_cfg_i_hqbic) }
	};
	list_interpolate[interpolate - Gdiplus::InterpolationModeLowQuality].Flags = LIF_SELECTED;
	FarList list_im;
	list_im.Items = list_interpolate;
	list_im.ItemsNumber = sizeof(list_interpolate) / sizeof(list_interpolate[0]);

	FarListItem list_background[] = {
		{ LIF_NONE, _PSI.GetMsg(&_FPG, ps_cfg_bckgr_disabled) },
		{ LIF_NONE, _PSI.GetMsg(&_FPG, ps_cfg_bckgr_grid10) },
		{ LIF_NONE, _PSI.GetMsg(&_FPG, ps_cfg_bckgr_grid20) },
		{ LIF_NONE, _PSI.GetMsg(&_FPG, ps_cfg_bckgr_grid30) }
	};
	list_background[bg_grid_step / 10].Flags = LIF_SELECTED;
	FarList list_bg;
	list_bg.Items = list_background;
	list_bg.ItemsNumber = sizeof(list_background) / sizeof(list_background[0]);

	const size_t ra_sizes[] = { 0, 1, 3, 5, 10 };
	FarListItem list_ra_size[] = {
		{ ra_size == ra_sizes[0] ? LIF_SELECTED : LIF_NONE, _PSI.GetMsg(&_FPG, ps_cfg_ra_0) },
		{ ra_size == ra_sizes[1] ? LIF_SELECTED : LIF_NONE, _PSI.GetMsg(&_FPG, ps_cfg_ra_1) },
		{ ra_size == ra_sizes[2] ? LIF_SELECTED : LIF_NONE, _PSI.GetMsg(&_FPG, ps_cfg_ra_3) },
		{ ra_size == ra_sizes[3] ? LIF_SELECTED : LIF_NONE, _PSI.GetMsg(&_FPG, ps_cfg_ra_5) },
		{ ra_size == ra_sizes[4] ? LIF_SELECTED : LIF_NONE, _PSI.GetMsg(&_FPG, ps_cfg_ra_10) }
	};
	FarList list_ras;
	list_ras.Items = list_ra_size;
	list_ras.ItemsNumber = sizeof(list_ra_size) / sizeof(list_ra_size[0]);

	const FarDialogItem dlg_items[] = {
		/*  0 */ { DI_DOUBLEBOX, 3, 1, 59, 18, 0, nullptr, nullptr, LIF_NONE, _PSI.GetMsg(&_FPG, ps_title) },
		/*  1 */ { DI_TEXT,      5, 2, 57, 2, 0, nullptr, nullptr, LIF_NONE, _PSI.GetMsg(&_FPG, ps_cfg_sc_title) },
		/*  2 */ { DI_COMBOBOX,  5 + static_cast<intptr_t>(wcslen(_PSI.GetMsg(&_FPG, ps_cfg_sc_title)) + 1), 2, 57, 2, reinterpret_cast<intptr_t>(&list_so), nullptr, nullptr, DIF_DROPDOWNLIST | DIF_LISTWRAPMODE | DIF_LISTNOAMPERSAND },
		/*  3 */ { DI_TEXT,      5, 3, 57, 3, 0, nullptr, nullptr, LIF_NONE, _PSI.GetMsg(&_FPG, ps_cfg_interpolate) },
		/*  4 */ { DI_COMBOBOX,  5 + static_cast<intptr_t>(wcslen(_PSI.GetMsg(&_FPG, ps_cfg_interpolate)) + 1), 3, 57, 3, reinterpret_cast<intptr_t>(&list_im), nullptr, nullptr, DIF_DROPDOWNLIST | DIF_LISTWRAPMODE | DIF_LISTNOAMPERSAND },
		/*  5 */ { DI_TEXT,      5, 4, 57, 4, 0, nullptr, nullptr, LIF_NONE, _PSI.GetMsg(&_FPG, ps_cfg_bckgr) },
		/*  6 */ { DI_COMBOBOX,  5 + static_cast<intptr_t>(wcslen(_PSI.GetMsg(&_FPG, ps_cfg_bckgr)) + 1), 4, 57, 4, reinterpret_cast<intptr_t>(&list_bg), nullptr, nullptr, DIF_DROPDOWNLIST | DIF_LISTWRAPMODE | DIF_LISTNOAMPERSAND  },
		/*  7 */ { DI_CHECKBOX,  5, 5, 57, 5, use_exif ? 1 : 0, nullptr, nullptr, LIF_NONE, _PSI.GetMsg(&_FPG, ps_cfg_exif_orient) },
		/*  8 */ { DI_TEXT,      0, 6,  0, 6, 0, nullptr, nullptr, DIF_SEPARATOR },
		/*  9 */ { DI_TEXT,      5, 7, 57, 7, 0, nullptr, nullptr, LIF_NONE, _PSI.GetMsg(&_FPG, ps_cfg_ra_tilte) },
		/* 10 */ { DI_COMBOBOX,  7 + static_cast<intptr_t>(wcslen(_PSI.GetMsg(&_FPG, ps_cfg_ra_tilte)) + 1), 7, 57, 7, reinterpret_cast<intptr_t>(&list_ras), nullptr, nullptr, DIF_DROPDOWNLIST | DIF_LISTWRAPMODE | DIF_LISTNOAMPERSAND  },
		/* 11 */ { DI_TEXT,      0, 8,  0, 8, 0, nullptr, nullptr, DIF_SEPARATOR },
		/* 12 */ { DI_CHECKBOX,  5, 9, 57, 9, ask_delete ? 1 : 0, nullptr, nullptr, LIF_NONE, _PSI.GetMsg(&_FPG, ps_cfg_ask_delete) },
		/* 13 */ { DI_TEXT,      0, 10,  0, 10, 0, nullptr, nullptr, DIF_SEPARATOR },
		/* 14 */ { DI_CHECKBOX,  5, 11, 57, 11, use_analyze ? 1 : 0, nullptr, nullptr, LIF_NONE, _PSI.GetMsg(&_FPG, ps_cfg_use_analyze) },
		/* 15 */ { DI_CHECKBOX,  5, 12, 57, 12, use_view ? 1 : 0, nullptr, nullptr, LIF_NONE, _PSI.GetMsg(&_FPG, ps_cfg_use_view) },
		/* 16 */ { DI_CHECKBOX,  5, 13, 57, 13, use_quickview ? 1 : 0, nullptr, nullptr, LIF_NONE, _PSI.GetMsg(&_FPG, ps_cfg_use_quickview) },
		/* 17 */ { DI_CHECKBOX,  5, 14, 57, 14, add_to_panel_menu ? 1 : 0, nullptr, nullptr, LIF_NONE, _PSI.GetMsg(&_FPG, ps_cfg_add_to_pm) },
		/* 18 */ { DI_TEXT,      5, 15, 57, 15, 0, nullptr, nullptr, LIF_NONE, _PSI.GetMsg(&_FPG, ps_cfg_prefix) },
		/* 19 */ { DI_EDIT,      5 + static_cast<intptr_t>(wcslen(_PSI.GetMsg(&_FPG, ps_cfg_prefix))) + 1, 15, 57, 15, 0, nullptr, nullptr, LIF_NONE, cmd_prefix.c_str() },
		/* 20 */ { DI_TEXT,      0, 16,  0, 16, 0, nullptr, nullptr, DIF_SEPARATOR },
		/* 21 */ { DI_BUTTON,    0, 17,  0, 17, 0, nullptr, nullptr, DIF_CENTERGROUP | DIF_DEFAULTBUTTON, _PSI.GetMsg(&_FPG, ps_ok) },
		/* 22 */ { DI_BUTTON,    0, 17,  0, 17, 0, nullptr, nullptr, DIF_CENTERGROUP, _PSI.GetMsg(&_FPG, ps_cancel) }
	};

	const HANDLE dlg = _PSI.DialogInit(&_FPG, &_FPG, -1, -1, 63, 20, nullptr, dlg_items, sizeof(dlg_items) / sizeof(dlg_items[0]), 0, FDLG_NONE, nullptr, nullptr);
	const intptr_t rc = _PSI.DialogRun(dlg);
	if (rc >= 0 && rc != sizeof(dlg_items) / sizeof(dlg_items[0]) - 1) {
		open_scale = scales[_PSI.SendDlgMessage(dlg, DM_LISTGETCURPOS, 2, nullptr)];
		interpolate = static_cast<Gdiplus::InterpolationMode>(Gdiplus::InterpolationModeLowQuality + _PSI.SendDlgMessage(dlg, DM_LISTGETCURPOS, 4, nullptr));
		bg_grid_step = static_cast<int>(_PSI.SendDlgMessage(dlg, DM_LISTGETCURPOS, 6, nullptr) * 10);
		use_exif = _PSI.SendDlgMessage(dlg, DM_GETCHECK, 7, nullptr) != 0;
		ra_size = ra_sizes[_PSI.SendDlgMessage(dlg, DM_LISTGETCURPOS, 10, nullptr)];
		ask_delete = _PSI.SendDlgMessage(dlg, DM_GETCHECK, 12, nullptr) != 0;
		use_analyze = _PSI.SendDlgMessage(dlg, DM_GETCHECK, 14, nullptr) != 0;
		use_view = _PSI.SendDlgMessage(dlg, DM_GETCHECK, 15, nullptr) != 0;
		use_quickview = _PSI.SendDlgMessage(dlg, DM_GETCHECK, 16, nullptr) != 0;
		add_to_panel_menu = _PSI.SendDlgMessage(dlg, DM_GETCHECK, 17, nullptr) != 0;
		cmd_prefix = reinterpret_cast<const wchar_t*>(_PSI.SendDlgMessage(dlg, DM_GETCONSTTEXTPTR, 19, nullptr));
		save();
	}
	_PSI.DialogFree(dlg);
}


void settings::save()
{
	settings_serializer s;
	SAVE_SETTINGS(s, open_scale);
	SAVE_SETTINGS(s, interpolate);
	SAVE_SETTINGS(s, bg_grid_step);
	SAVE_SETTINGS(s, use_exif);
	SAVE_SETTINGS(s, ra_size);
	SAVE_SETTINGS(s, ask_delete);
	SAVE_SETTINGS(s, use_analyze);
	SAVE_SETTINGS(s, use_view);
	SAVE_SETTINGS(s, use_quickview);
	SAVE_SETTINGS(s, add_to_panel_menu);
	SAVE_SETTINGS(s, cmd_prefix);
}
