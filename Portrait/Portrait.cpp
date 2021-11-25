#include "stdafx.h"

#include "PortraitUI.hpp"



/*
Функция, которая вызывается Far-ом для того, 
чтобы плагин предоставил информацию о себе.

Если хотя-бы одно из полей структуры GlobalInfo не будет заполнено - плагин не инициализируется.
*/
void WINAPI GetGlobalInfoW(struct GlobalInfo *Info)
{
	Info->StructSize = sizeof(struct GlobalInfo);
	Info->MinFarVersion = FARMANAGERVERSION;
	Info->Version = PLUGIN_VERSION;
	Info->Guid = MainGuid;
	Info->Title = PLUGIN_NAME;
	Info->Description = PLUGIN_DESC;
	Info->Author = PLUGIN_AUTHOR;
}

/*
Используя эту функцию, Far передает плагину указатель на структуру, 
посредством которой будет в дальнейшем осуществляется доступ к Far API.
*/
void WINAPI SetStartupInfoW(const struct PluginStartupInfo *startupInfo)
{
	Info = *startupInfo;
}
 
/*
Эта функция используется Far-ом, чтобы выяснить детальную информацию о плагине.
А конкретно - плагин может зарегистрироваться в меню команд, 
которое вызывается в Far через нажатие F11.
*/
void WINAPI GetPluginInfoW(struct PluginInfo *pluginInfo)
{
	pluginInfo->StructSize = sizeof(*pluginInfo);
	pluginInfo->Flags = PF_EDITOR;
	
	static const wchar_t* PluginMenuStrings[1];
	PluginMenuStrings[0] = Info.GetMsg(&MainGuid, MTitle);
	pluginInfo->PluginMenu.Guids = &MenuGuid;
	pluginInfo->PluginMenu.Strings = PluginMenuStrings;
	pluginInfo->PluginMenu.Count = ARRAYSIZE(PluginMenuStrings);
}
 
/*
Эта функция вызывается при нажатии Enter в меню команд.
*/
HANDLE WINAPI OpenW(const struct OpenInfo *OInfo)
{
	// Если главное окно не может отобразиться, то выходим
	if (!ShowDialog()) return NULL;

	const wchar_t* MsgItems[] =
	{
		Info.GetMsg(&MainGuid, MTitle),
		Info.GetMsg(&MainGuid, MSelectData),
		L"\x01",
		Info.GetMsg(&MainGuid, MOk),
	};

	Info.Message(&MainGuid, NULL, FMSG_WARNING | FMSG_LEFTALIGN, L"Contents",
		MsgItems, ARRAYSIZE(MsgItems), 1);
	                     
	return NULL;
}