#pragma once
#include "DlgBuilder.hpp"
#include "guid.hpp"
#include "PortraitLang.hpp"
#include "targetver.h"
#include "version.hpp"
#include <algorithm>
#include <fstream>
#include <map>

//Статическая переменная, которая хранит структуру для доступа к Far API.
static struct PluginStartupInfo Info;

enum class TypeImage { BMP, PNG };
std::map<int, std::wstring> type_image = {
	{ 0, L"BMP"},
	{ 1, L"PNG"}
};

bool is_digits(const std::wstring& str)
{
	bool is = std::all_of(str.begin(), str.end(), isdigit);
	return is;
}

static int CurrentTypeImage = static_cast<int>(TypeImage::BMP);
static wchar_t SizeImagedefault[16] = L"100";
static int isGrade = 0;
static int isShowImage = 0;
static int SizeImage = 0;


bool ShowDialog() {

	// Создаем главное окошко
	PluginDialogBuilder window(Info, MainGuid, MenuGuid, MTitle, L"Contents", NULL);
	
	window.StartColumns(); // Делим окно на колонки
	window.AddText(MTypeImage); // Сообщение выберите тип картинки
	const int TypeIDs[] = { MTypeBMP, MTypePNG }; // Список расширений картинки
	window.AddRadioButtons(&CurrentTypeImage, ARRAYSIZE(TypeIDs), TypeIDs, true);  // Добавление радиокнопок
	window.ColumnBreak(); // Добавить столбик
	window.EndColumns(); // Закрываем колонки

	window.AddSeparator(); // Вставить черный разделитель

	FarDialogItem* item = window.AddEditField((wchar_t*)&SizeImagedefault, 15, 15); // Добавление текстового поля
	window.AddTextBefore(item, MSizeImage);

	window.AddSeparator(); // Вставить черный разделитель

	// Флаги
	window.AddCheckbox(MGrade, &isGrade);
	window.AddCheckbox(MShowImage, &isShowImage);

	// Кнопки
	window.AddOKCancel(MBegin, MCancel);

	
	return window.ShowDialog();
}

	// Показываем окно и ждем действие пользователя
	//bool isBegin = window.ShowDialog(); 

	//while (isBegin)
	//{
	//	if (is_digits(SizeImagedefault)) {
	//		SizeImage = wcstol(SizeImagedefault, nullptr, 10);

	//		PluginDialogBuilder InfoWindow(Info, MainGuid, MenuGuid, MTitle, L"Contents");

	//		InfoWindow.StartColumns(); // Делим окно на колонки
	//		InfoWindow.AddText(MSelectData); // Сообщение Выбранные параметры:
	//		FarDialogItem* Type = InfoWindow.AddReadonlyEditField(type_image[CurrentTypeImage].c_str(), 15);
	//		InfoWindow.AddTextBefore(Type, MImageType);
	//		FarDialogItem* Size = InfoWindow.AddReadonlyEditField((wchar_t*)&SizeImagedefault, 15);
	//		InfoWindow.AddTextBefore(Size, MSizeImage);
	//		InfoWindow.ColumnBreak(); // Добавить столбик
	//		InfoWindow.EndColumns(); // Закрываем колонки

	//		InfoWindow.AddText(MTitle3);


	//		InfoWindow.AddSeparator();
	//		InfoWindow.AddButtons(1, new int{ MOk }, DIF_DISABLE);
	//		InfoWindow.ShowDialog();

	//		isBegin = false;
	//	}
	//	else {
	//		PluginDialogBuilder WarningWindow(Info, MainGuid, MenuGuid, MError, L"Contents", nullptr, nullptr, FMSG_WARNING);

	//		WarningWindow.AddText(MIncorrectData);
	//		WarningWindow.AddSeparator();
	//		WarningWindow.AddButtons(1, new int{ MOk });
	//		WarningWindow.ShowDialog();

	//		isBegin = window.ShowDialog();
	//	}
	//} 