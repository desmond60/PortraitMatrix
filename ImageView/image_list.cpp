#include "image_list.h"
#include "image_lib.h"
#include "string_rc.h"
#include "settings.h"


image_list::image_list(image* img)
:	_plugin_panel(false),
	_use_real_path(false),
	_ra_thread(nullptr),
	_ra_stop(nullptr)
{
	assert(img);

	_current = std::make_shared<image_item_obj>();
	_current->img = img;
	_current->delete_on_close = false;
	_current->index = initialize(img->get_file_name());

	_ra_stop = CreateEvent(nullptr, TRUE, TRUE, nullptr);

	start_read_ahead();
}


image_list::~image_list()
{
	stop_read_ahead();
	if (_ra_stop)
		CloseHandle(_ra_stop);
}


image* image_list::delete_current_file()
{
	if (_current->delete_on_close)
		return _current->img;	//Plugin-s panel

	//Check for external plug-in panel
	PanelInfo pi;
	ZeroMemory(&pi, sizeof(PanelInfo));
	pi.StructSize = sizeof(PanelInfo);
	if (_PSI.PanelControl(PANEL_ACTIVE, FCTL_GETPANELINFO, 0, &pi) && (pi.Flags & PFLAGS_PLUGIN) != 0)
		return _current->img;	//Plugin-s panel

	//Get delete mode from Far settings
	bool use_recycle_bin = true;
	FarSettingsCreate fsc;
	ZeroMemory(&fsc, sizeof(fsc));
	fsc.StructSize = sizeof(fsc);
	fsc.Guid = GUID_NULL;
	fsc.Handle = INVALID_HANDLE_VALUE;
	if (_PSI.SettingsControl(INVALID_HANDLE_VALUE, SCTL_CREATE, 0, &fsc)) {
		FarSettingsItem fsi;
		ZeroMemory(&fsi, sizeof(fsi));
		fsi.StructSize = sizeof(fsi);
		fsi.Root = FSSF_SYSTEM;
		fsi.Name = L"DeleteToRecycleBin";
		fsi.Type = FST_QWORD;
		if (_PSI.SettingsControl(fsc.Handle, SCTL_GET, 0, &fsi))
			use_recycle_bin = fsi.Number != 0;
		_PSI.SettingsControl(fsc.Handle, SCTL_FREE, 0, nullptr);
	}

	if (settings::ask_delete) {
		std::wstring fn = _FSF.PointToName(_current->img->get_file_name());
		fn += L'?';
		const wchar_t* confirm_msg[] = { _PSI.GetMsg(&_FPG, ps_del_title), _PSI.GetMsg(&_FPG, use_recycle_bin ? ps_del_recycle : ps_del_delete), fn.c_str() };
		if (_PSI.Message(&_FPG, &_FPG, FMSG_WARNING | FMSG_MB_YESNO, nullptr, confirm_msg, sizeof(confirm_msg) / sizeof(confirm_msg[0]), 0) != 0)
			return _current->img;
	}

	intptr_t curr_index = _current->index;
	const std::wstring file_to_delete = _current->img->get_file_name();
	_current.reset();

	bool file_deleted = false;

	if (!use_recycle_bin) {
		//Remove file
		file_deleted = DeleteFile(file_to_delete.c_str()) != FALSE;
		if (!file_deleted) {
			const wchar_t* err_msg[] = { _PSI.GetMsg(&_FPG, ps_del_title), _PSI.GetMsg(&_FPG, ps_err_del), _FSF.PointToName(file_to_delete.c_str()) };
			_PSI.Message(&_FPG, &_FPG, FMSG_WARNING | FMSG_ERRORTYPE, nullptr, err_msg, sizeof(err_msg) / sizeof(err_msg[0]), 0);
		}
	}
	else {
		//Move to recycle bin
		SHFILEOPSTRUCT shfos;
		ZeroMemory(&shfos, sizeof(shfos));
		shfos.wFunc = FO_DELETE;
		shfos.fFlags = FOF_ALLOWUNDO | FOF_NOCONFIRMATION | FOF_SILENT;
		std::wstring fn_zeroed = file_to_delete;
		fn_zeroed += L'\0';
		shfos.pFrom = fn_zeroed.c_str();
		file_deleted = SHFileOperation(&shfos) == 0;
		if (!file_deleted) {
			const wchar_t* err_msg[] = { _PSI.GetMsg(&_FPG, ps_del_title), _PSI.GetMsg(&_FPG, ps_err_mrb), _FSF.PointToName(file_to_delete.c_str()) };
			_PSI.Message(&_FPG, &_FPG, FMSG_WARNING | FMSG_ERRORTYPE, nullptr, err_msg, sizeof(err_msg) / sizeof(err_msg[0]), 0);
		}
	}

	image* img = step(file_deleted ? curr_index : curr_index - 1, d_forward);
	if (!img)
		img = step(curr_index, d_backward);

	return img;
}


void image_list::set_selection() const
{
	const intptr_t idx = _current->index;
	if (idx < 0)
		return;

	const BOOL selected = TRUE;
	_PSI.PanelControl(PANEL_ACTIVE, FCTL_BEGINSELECTION, 0, nullptr);
	_PSI.PanelControl(PANEL_ACTIVE, FCTL_SETSELECTION, idx, reinterpret_cast<void*>(static_cast<size_t>(selected)));
	_PSI.PanelControl(PANEL_ACTIVE, FCTL_ENDSELECTION, 0, nullptr);

	PanelInfo pi;
	ZeroMemory(&pi, sizeof(pi));
	pi.StructSize = sizeof(pi);
	_PSI.PanelControl(PANEL_ACTIVE, FCTL_GETPANELINFO, 0, &pi);
	PanelRedrawInfo pri;
	ZeroMemory(&pri, sizeof(pri));
	pri.StructSize = sizeof(pri);
	pri.CurrentItem = idx;
	pri.TopPanelItem = pi.TopPanelItem;
	_PSI.PanelControl(PANEL_ACTIVE, FCTL_REDRAWPANEL, 0, &pri);
}


image* image_list::last()
{
	PanelInfo pi;
	ZeroMemory(&pi, sizeof(pi));
	pi.StructSize = sizeof(pi);
	_PSI.PanelControl(PANEL_ACTIVE, FCTL_GETPANELINFO, 0, &pi);
	step(pi.ItemsNumber, d_backward);
	 return _current->img;
}


intptr_t image_list::initialize(const wchar_t* file_name)
{
	assert(file_name);

	intptr_t idx = -1;

	//Read panel info
	PanelInfo pi;
	ZeroMemory(&pi, sizeof(pi));
	pi.StructSize = sizeof(pi);
	_plugin_panel = _PSI.PanelControl(PANEL_ACTIVE, FCTL_GETPANELINFO, 0, &pi) && (pi.Flags & PFLAGS_PLUGIN) != 0;
	_use_real_path = (pi.Flags & PFLAGS_REALNAMES) != 0;

	//Find current file index
	for (size_t i = 0; idx < 0 && i < pi.ItemsNumber; ++i) {
		const intptr_t ppi_len = _PSI.PanelControl(PANEL_ACTIVE, FCTL_GETPANELITEM, static_cast<intptr_t>(i), nullptr);
		if (ppi_len == 0)
			continue;
		std::vector<unsigned char> buffer(ppi_len);
		PluginPanelItem* ppi = reinterpret_cast<PluginPanelItem*>(&buffer.front());
		FarGetPluginPanelItem fgppi;
		ZeroMemory(&fgppi, sizeof(fgppi));
		fgppi.StructSize = sizeof(fgppi);
		fgppi.Size = buffer.size();
		fgppi.Item = ppi;
		if (!_PSI.PanelControl(PANEL_ACTIVE, FCTL_GETPANELITEM, static_cast<intptr_t>(i), &fgppi))
			continue;
		if (_wcsicmp(_FSF.PointToName(ppi->FileName), _FSF.PointToName(file_name)) == 0)
			idx = i;
	}

	return idx;
}


bool image_list::get_next_file(const intptr_t start_index, const direction dir, std::wstring& file_name, intptr_t& file_index) const
{
	if (dir == d_backward && start_index <= 0)
		return false;

	PanelInfo pi;
	ZeroMemory(&pi, sizeof(pi));
	pi.StructSize = sizeof(pi);
	_PSI.PanelControl(PANEL_ACTIVE, FCTL_GETPANELINFO, 0, &pi);
	file_index = start_index + (dir == d_forward ? 1 : -1);
	while (file_index >= 0 && file_index < static_cast<intptr_t>(pi.ItemsNumber)) {
		const intptr_t ppi_len = _PSI.PanelControl(PANEL_ACTIVE, FCTL_GETPANELITEM, file_index, nullptr);
		if (ppi_len == 0)
			continue;
		std::vector<unsigned char> buffer(ppi_len);
		PluginPanelItem* ppi = reinterpret_cast<PluginPanelItem*>(&buffer.front());
		FarGetPluginPanelItem fgppi;
		ZeroMemory(&fgppi, sizeof(fgppi));
		fgppi.StructSize = sizeof(fgppi);
		fgppi.Size = buffer.size();
		fgppi.Item = ppi;
		if (!_PSI.PanelControl(PANEL_ACTIVE, FCTL_GETPANELITEM, file_index, &fgppi))
			continue;
		if (image_lib::instance()->format_supported(ppi->FileName)) {
			if (_plugin_panel)
				file_name = ppi->FileName;
			else {
				file_name.clear();
				const size_t file_name_len = _FSF.ConvertPath(CPM_FULL, ppi->FileName, nullptr, 0);
				if (file_name_len) {
					file_name.resize(file_name_len, 0);
					_FSF.ConvertPath(CPM_FULL, ppi->FileName, &file_name[0], file_name_len);
				}
				if (GetFileAttributes(file_name.c_str()) == INVALID_FILE_ATTRIBUTES)
					file_name.clear();
			}
			if (!file_name.empty())
				return true;
		}
		file_index += (dir == d_forward ? 1 : -1);
	}

	return false;
}


bool image_list::open_image(const wchar_t* file_name, const intptr_t file_index, image_item& ii) const
{
	assert(file_name && file_name[0]);

	//Try to open file as image
	if (!_plugin_panel || _use_real_path)
		ii->img = image::open(file_name);
	else {
		//Extract file from external plug-in panel
		const std::wstring new_file_name = extract_file(file_index);
		if (!new_file_name.empty()) {
			ii->img = image::open(new_file_name.c_str());
			if (!ii->img)
				DeleteFile(new_file_name.c_str());
			else
				ii->delete_on_close = true;
		}
	}
	if (ii->img != nullptr)
		ii->index = file_index;

	return (ii->img != nullptr);
}


image* image_list::step(const intptr_t start_index, const direction dir)
{
	if (dir == d_backward && start_index <= 0)
		return nullptr;

	stop_read_ahead();

	bool file_opened = false;
	std::wstring file_name;
	intptr_t index = -1;
	intptr_t last_index = start_index;
	while (!file_opened && get_next_file(last_index, dir, file_name, index)) {
		//Try to find cached item
		if (dir == d_backward) {
			file_opened = _previous.get() && _wcsicmp(_FSF.PointToName(file_name.c_str()), _FSF.PointToName(_previous->img->get_file_name())) == 0;
			if (file_opened) {
				_next.insert(_next.begin(), _current);
				_current = _previous;
			}
		}
		else /*if (dir == d_forward)*/ {
			file_opened = !_next.empty() && _wcsicmp(_FSF.PointToName(file_name.c_str()), _FSF.PointToName(_next.front()->img->get_file_name())) == 0;
			if (file_opened) {
				_previous = _current;
				_current = _next.front();
				_next.erase(_next.begin());
			}
		}
		if (!file_opened) {
			//Try to open file as image
			image_item ii = std::make_shared<image_item_obj>();
			if (open_image(file_name.c_str(), index, ii)) {
				if (dir == d_forward)
					_previous = _current;
				else
					_next.insert(_next.begin(), _current);
				_current = ii;
				file_opened = true;
			}
		}
		last_index += dir == d_forward ? 1 : -1;
	}

	if (!file_opened)
		return nullptr;

	start_read_ahead();

	//Select file on panel
	PanelInfo pi;
	ZeroMemory(&pi, sizeof(pi));
	pi.StructSize = sizeof(pi);
	_PSI.PanelControl(PANEL_ACTIVE, FCTL_GETPANELINFO, 0, &pi);
	PanelRedrawInfo pri;
	ZeroMemory(&pri, sizeof(pri));
	pri.StructSize = sizeof(pri);
	pri.CurrentItem = _current->index;
	pri.TopPanelItem = pi.TopPanelItem;
	_PSI.PanelControl(PANEL_ACTIVE, FCTL_REDRAWPANEL, 0, &pri);

	return _current->img;
}


std::wstring image_list::extract_file(const intptr_t index) const
{
	//Get panel description
	PanelInfo pi;
	ZeroMemory(&pi, sizeof(pi));
	pi.StructSize = sizeof(pi);
	_PSI.PanelControl(PANEL_ACTIVE, FCTL_GETPANELINFO, 0, &pi);

	//Get panel item description
	const intptr_t ppi_len = _PSI.PanelControl(PANEL_ACTIVE, FCTL_GETPANELITEM, index, nullptr);
	if (ppi_len == 0)
		return std::wstring();
	std::vector<unsigned char> buffer(ppi_len);
	PluginPanelItem* ppi = reinterpret_cast<PluginPanelItem*>(&buffer.front());
	FarGetPluginPanelItem fgppi;
	ZeroMemory(&fgppi, sizeof(fgppi));
	fgppi.StructSize = sizeof(fgppi);
	fgppi.Size = buffer.size();
	fgppi.Item = ppi;
	if (!_PSI.PanelControl(PANEL_ACTIVE, FCTL_GETPANELITEM, index, &fgppi))
		return std::wstring();

	//Get panel plug-in module
	HANDLE plugin_handle = reinterpret_cast<HANDLE>(_PSI.PluginsControl(nullptr, PCTL_FINDPLUGIN, PFM_GUID, &pi.OwnerGuid));
	if (!plugin_handle)
		return std::wstring();
	const intptr_t pli_len = _PSI.PluginsControl(plugin_handle, PCTL_GETPLUGININFORMATION, 0, nullptr);
	if (pli_len <= 0)
		return std::wstring();
	std::vector<unsigned char> pli_buff(pli_len);
	FarGetPluginInformation* plugin_info = reinterpret_cast<FarGetPluginInformation*>(&pli_buff.front());
	plugin_info->StructSize = sizeof(FarGetPluginInformation);
	if (!_PSI.PluginsControl(plugin_handle, PCTL_GETPLUGININFORMATION, pli_len, plugin_info))
		return std::wstring();
	HMODULE plugin_lib = GetModuleHandle(plugin_info->ModuleName);
	if (!plugin_lib)
		return std::wstring();

	//Get copy files routine
	typedef int (WINAPI *get_files_fx)(GetFilesInfo*);
	get_files_fx get_files_fx_ptr = reinterpret_cast<get_files_fx>(reinterpret_cast<size_t>(GetProcAddress(plugin_lib, "GetFilesW")));
	if (!get_files_fx_ptr)
		return std::wstring();

	//Prepare output directory
	wchar_t tmp_path[MAX_PATH];
	if (!GetTempPath(MAX_PATH, tmp_path))
		return std::wstring();
	wcscat_s(tmp_path, L"far_imageview\\");
	if (!CreateDirectory(tmp_path, nullptr) && GetLastError() != ERROR_ALREADY_EXISTS)
		return std::wstring();
	std::wstring file_name = tmp_path;
	file_name += ppi->FileName;

	//Extract file
	GetFilesInfo gfi;
	ZeroMemory(&gfi, sizeof(gfi));
	gfi.StructSize = sizeof(gfi);
	gfi.hPanel = pi.PluginHandle;
	gfi.PanelItem = const_cast<PluginPanelItem*>(ppi);
	gfi.ItemsNumber = 1;
	gfi.OpMode = OPM_SILENT;
	gfi.DestPath = tmp_path;
	try {
		if (get_files_fx_ptr(&gfi) != 1 || GetFileAttributes(file_name.c_str()) == INVALID_FILE_ATTRIBUTES)
			return std::wstring();
	}
	catch(...) {
		return std::wstring();
	}

	return file_name;
}


void image_list::start_read_ahead()
{
	if (!_plugin_panel && settings::ra_size > 0 && _current->index >= 0) {
		assert(_ra_thread == nullptr);
		ResetEvent(_ra_stop);
		_ra_thread = CreateThread(nullptr, 0, &image_list::read_ahead_thread, this, 0, nullptr);
	}
}


void image_list::stop_read_ahead()
{
	if (_ra_thread) {
		SetEvent(_ra_stop);
		WaitForSingleObject(_ra_thread, INFINITE);
		CloseHandle(_ra_thread);
		_ra_thread = nullptr;
	}
}


DWORD WINAPI image_list::read_ahead_thread(PVOID param)
{
	image_list* instance = reinterpret_cast<image_list*>(param);
	assert(instance);
	assert(instance->_current->index >= 0);

	cache new_cache;
	intptr_t start_index = instance->_current->index;
	std::wstring file_name;
	intptr_t index = -1;
	while (WaitForSingleObject(instance->_ra_stop, 0) == WAIT_TIMEOUT && instance->get_next_file(start_index, d_forward, file_name, index)) {
		//Search in existing cache

		bool cached_found = false;
		for (cache::iterator it = instance->_next.begin(); !cached_found && it != instance->_next.end(); ++it) {
			cached_found = ((*it)->img && _wcsicmp(_FSF.PointToName(file_name.c_str()), _FSF.PointToName((*it)->img->get_file_name())) == 0);
			if (cached_found)
				new_cache.insert(new_cache.end(), *it);
		}

		if (!cached_found) {
			image_item ii = std::make_shared<image_item_obj>();
			if (instance->open_image(file_name.c_str(), index, ii))
				new_cache.insert(new_cache.end(), ii);
		}

		if (new_cache.size() >= settings::ra_size)
			break;

		start_index = index;
	}

	new_cache.swap(instance->_next);

	return 0;
}

image_list::image_item_obj::image_item_obj()
:	img(nullptr),
	index(-1),
	delete_on_close(false)
{
}


image_list::image_item_obj::~image_item_obj()
{
	if (img) {
		const std::wstring file_name = img->get_file_name();
		delete img;
		if (delete_on_close)
			DeleteFile(file_name.c_str());
	}
	img = nullptr;
	index = -1;
	delete_on_close = false;
}
