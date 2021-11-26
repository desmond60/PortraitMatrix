#include "common.h"
#include "settings.h"
#include "string_rc.h"
#include "version.h"
#include "image_lib.h"
#include "viewer.h"
#include "viewer_qv.h"
#include "PortraitUI.hpp"

bool IsImage = false;

/**
 * Open image.
 * \param file_name image file name
 * \param show_wait show wait window flag
 * \param silent silent mode flag (true to show progress and error message)
 * \return image instance (nullptr on error)
 */
image* open_image(const wchar_t* file_name, const bool silent);


void WINAPI SetStartupInfoW(const PluginStartupInfo* psi)
{
	_PSI = *psi;
	_FSF = *psi->FSF;
	_PSI.FSF = &_FSF;

	settings::load();
	image_lib::instance();	//Initialize image lib
}


void WINAPI GetGlobalInfoW(GlobalInfo* info)
{
	info->StructSize = sizeof(GlobalInfo);
	info->MinFarVersion = MAKEFARVERSION(FARMANAGERVERSION_MAJOR, FARMANAGERVERSION_MINOR, FARMANAGERVERSION_REVISION, PLUGIN_FAR_BUILD, VS_RELEASE);
	info->Version = MAKEFARVERSION(PLUGIN_VERSION_NUM, VS_RELEASE);
	info->Guid = _FPG;
	info->Title = TEXT(PLUGIN_NAME);
	info->Description = TEXT(PLUGIN_DESCR);
	info->Author = TEXT(PLUGIN_AUTHOR);
}


void WINAPI GetPluginInfoW(PluginInfo* info)
{
	assert(info);

	info->StructSize = sizeof(PluginInfo);

	if (!settings::cmd_prefix.empty())
		info->CommandPrefix = settings::cmd_prefix.c_str();

	static const wchar_t* menu_strings[1];
	menu_strings[0] = _PSI.GetMsg(&_FPG, ps_title);

	info->PluginConfig.Guids = &_FPG;
	info->PluginConfig.Strings = menu_strings;
	info->PluginConfig.Count = sizeof(menu_strings) / sizeof(menu_strings[0]);

	if (!settings::add_to_panel_menu)
		info->Flags |= PF_DISABLEPANELS;
	else {
		info->Flags |= PF_EDITOR;
		info->Flags |= PF_VIEWER;
		info->PluginMenu.Guids = &_FPG;
		info->PluginMenu.Strings = menu_strings;
		info->PluginMenu.Count = sizeof(menu_strings) / sizeof(menu_strings[0]);
	}

#ifdef _DEBUG
	info->Flags |= PF_PRELOAD;
#endif // _DEBUG
}


HANDLE WINAPI AnalyseW(const AnalyseInfo* info)
{
	if (!info || info->StructSize < sizeof(AnalyseInfo) || !info->FileName || !settings::use_analyze)
		return nullptr;
	if (!image_lib::instance()->format_supported(info->FileName))
		return nullptr;
	IsImage = true;
	return open_image(info->FileName, true);
}


void WINAPI CloseAnalyseW(const struct CloseAnalyseInfo* info)
{
	if (info && info->StructSize >= sizeof(CloseAnalyseInfo) && info->Handle)
		delete reinterpret_cast<image*>(info->Handle);
}


HANDLE WINAPI OpenW(const OpenInfo* info)
{
	if (IsImage) {
		using std::wstring;
		using std::string;
		if (!info || info->StructSize < sizeof(OpenInfo))
			return nullptr;

		image* img = nullptr;

		if (info->OpenFrom == OPEN_ANALYSE && info->Data)
			img = reinterpret_cast<image*>(reinterpret_cast<OpenAnalyseInfo*>(info->Data)->Handle);
		else {
			//Determine file name for open
			wstring file_name;
			if (info->OpenFrom == OPEN_COMMANDLINE && info->Data) {
				const OpenCommandLineInfo* ocli = reinterpret_cast<const OpenCommandLineInfo*>(info->Data);
				if (!ocli || ocli->StructSize < sizeof(OpenCommandLineInfo) || !ocli->CommandLine || !ocli->CommandLine[0])
					return nullptr;
				//Get command line
				wstring cmd_line = ocli->CommandLine;
				size_t pos = 0;
				while ((pos = cmd_line.find(L'\"', pos)) != string::npos)
					cmd_line.erase(pos, 1);
				while (!cmd_line.empty() && iswspace(cmd_line[0]))
					cmd_line.erase(0, 1);
				while (!cmd_line.empty() && iswspace(cmd_line[cmd_line.length() - 1]))
					cmd_line.erase(cmd_line.length() - 1, 1);
				if (cmd_line.empty())
					return nullptr;
				//Expand environment variables in path string
				wstring exp_path(2048, 0);
				if (ExpandEnvironmentStrings(cmd_line.c_str(), &exp_path.front(), static_cast<DWORD>(exp_path.size() - 1)))
					exp_path.resize(lstrlen(exp_path.c_str()));
				else
					exp_path = cmd_line;
				const size_t path_len = _FSF.ConvertPath(CPM_FULL, exp_path.c_str(), nullptr, 0);
				if (path_len) {
					file_name.resize(path_len);
					_FSF.ConvertPath(CPM_FULL, exp_path.c_str(), &file_name[0], path_len);
				}
			}
			else if (info->OpenFrom == OPEN_PLUGINSMENU) {
				PanelInfo pi;
				ZeroMemory(&pi, sizeof(pi));
				pi.StructSize = sizeof(pi);
				if (!_PSI.PanelControl(PANEL_ACTIVE, FCTL_GETPANELINFO, 0, &pi))
					return nullptr;
				const intptr_t ppi_len = _PSI.PanelControl(PANEL_ACTIVE, FCTL_GETPANELITEM, static_cast<intptr_t>(pi.CurrentItem), nullptr);
				if (ppi_len == 0)
					return nullptr;
				std::vector<unsigned char> buffer(ppi_len);
				PluginPanelItem* ppi = reinterpret_cast<PluginPanelItem*>(&buffer.front());
				FarGetPluginPanelItem fgppi;
				ZeroMemory(&fgppi, sizeof(fgppi));
				fgppi.StructSize = sizeof(fgppi);
				fgppi.Size = buffer.size();
				fgppi.Item = ppi;
				if (!_PSI.PanelControl(PANEL_ACTIVE, FCTL_GETPANELITEM, static_cast<intptr_t>(pi.CurrentItem), &fgppi))
					return nullptr;
				const size_t file_name_len = _FSF.ConvertPath(CPM_FULL, ppi->FileName, nullptr, 0);
				if (file_name_len) {
					file_name.resize(file_name_len);
					_FSF.ConvertPath(CPM_FULL, ppi->FileName, &file_name[0], file_name_len);
				}
			}
			else if (info->OpenFrom == OPEN_VIEWER) {
				const intptr_t buff_len = _PSI.ViewerControl(-1, VCTL_GETFILENAME, 0, nullptr);
				if (buff_len) {
					file_name.resize(buff_len + 1, 0);
					_PSI.ViewerControl(-1, VCTL_GETFILENAME, static_cast<intptr_t>(file_name.size()), &file_name[0]);
				}
			}
			else if (info->OpenFrom == OPEN_EDITOR) {
				const intptr_t buff_len = _PSI.EditorControl(CURRENT_EDITOR, ECTL_GETFILENAME, 0, nullptr);
				if (buff_len) {
					file_name.resize(buff_len + 1, 0);
					_PSI.EditorControl(CURRENT_EDITOR, ECTL_GETFILENAME, static_cast<intptr_t>(file_name.size()), &file_name[0]);
				}
			}

			if (!file_name.empty())
				img = open_image(file_name.c_str(), false);
		}

		if (img) {
			viewer v(img);
			v.show();
			IsImage = false;
		}
		return (info->OpenFrom == OPEN_ANALYSE ? PANEL_STOP : nullptr);
	}


	// Если главное окно не может отобразиться, то выходим
	if (!ShowDialog()) return NULL;

	const wchar_t* MsgItems[] =
	{
		_PSI.GetMsg(&_FPG, ps_title),
		_PSI.GetMsg(&_FPG, MSelectData),
		L"\x01",
		_PSI.GetMsg(&_FPG, MOk),
	};

	_PSI.Message(&_FPG, NULL, FMSG_WARNING | FMSG_LEFTALIGN, L"Contents",
		MsgItems, ARRAYSIZE(MsgItems), 1);

	return NULL;

	
}


intptr_t WINAPI ProcessViewerEventW(const ProcessViewerEventInfo* info)
{
	if (!info || info->StructSize < sizeof(ProcessViewerEventInfo))
		return 0;

	if (info->Event != VE_READ && settings::use_quickview)
		viewer_qv::instance()->hide();
	else if (info->Event == VE_READ && (settings::use_quickview || settings::use_view)) {
		//Determine quick view panel
		PanelInfo pi;
		ZeroMemory(&pi, sizeof(pi));
		pi.StructSize = sizeof(pi);
		_PSI.PanelControl(PANEL_PASSIVE, FCTL_GETPANELINFO, 0, &pi);
		if (pi.PanelType != PTYPE_QVIEWPANEL)
			_PSI.PanelControl(PANEL_ACTIVE, FCTL_GETPANELINFO, 0, &pi);
		const bool is_quick_view = pi.PanelType == PTYPE_QVIEWPANEL;

		if (is_quick_view && !settings::use_quickview)
			return 0;
		if (!is_quick_view && !settings::use_view)
			return 0;

		const intptr_t buff_len = _PSI.ViewerControl(-1, VCTL_GETFILENAME, 0, nullptr);
		if (buff_len) {
			std::wstring file_name(buff_len + 1, 0);
			_PSI.ViewerControl(-1, VCTL_GETFILENAME, static_cast<intptr_t>(file_name.size()), &file_name[0]);
			if (image_lib::instance()->format_supported(file_name.c_str())) {
				image* img = open_image(file_name.c_str(), false);
				if (img) {
					if (is_quick_view)
						viewer_qv::instance()->show(img, pi);
					else {
						viewer v(img);
						v.show();
						PostMessage(screen::far_window(), WM_KEYDOWN, VK_F10, 0);
					}
				}
			}
		}
	}

	return 0;
}


intptr_t WINAPI ConfigureW(const ConfigureInfo* info)
{
	if (!info || info->StructSize < sizeof(ConfigureInfo))
		return 0;
	settings::configure();
	if (!settings::use_quickview)
		viewer_qv::instance()->hide();
	return 0;
}


image* open_image(const wchar_t* file_name, const bool silent)
{
	if (!silent) {
		const wchar_t* msg[] = { _PSI.GetMsg(&_FPG, ps_title), _PSI.GetMsg(&_FPG, ps_read_in_progress), _FSF.PointToName(file_name) };
		_PSI.Message(&_FPG, &_FPG, FMSG_NONE, nullptr, msg, sizeof(msg) / sizeof(msg[0]), 0);
	}

	image* img = image::open(file_name);
	if (!silent) {
		if (img)
			_PSI.AdvControl(&_FPG, ACTL_REDRAWALL, 0, nullptr);
		else {
			const wchar_t* err_msg[] = { _PSI.GetMsg(&_FPG, ps_title), _PSI.GetMsg(&_FPG, ps_err_open), _FSF.PointToName(file_name), image_lib::instance()->get_last_error() };
			_PSI.Message(&_FPG, &_FPG, FMSG_WARNING | FMSG_MB_OK, nullptr, err_msg, sizeof(err_msg) / sizeof(err_msg[0]), 0);
		}
	}

	return img;
}
