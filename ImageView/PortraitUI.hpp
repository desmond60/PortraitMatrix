#pragma once
#include "DlgBuilder.hpp"
#include "common.h"
#include "string_rc.h"
#include "version.h"
#include <algorithm>
#include <fstream>
#include <map>

PluginStartupInfo    _PSI;
FarStandardFunctions _FSF;

//! Plugin GUID {9D4A59D9-AD2D-478C-8F66-7D233CBB788D}
const GUID _FPG = { 0x9d4a59d9, 0xad2d, 0x478c, { 0x8f, 0x66, 0x7d, 0x23, 0x3c, 0xbb, 0x78, 0x8d } };
const GUID _MenuGuid = { 0xa0fb9eb5, 0xedee, 0x42c3, {0xa3, 0x7f, 0x54, 0x9d, 0xe8, 0xc5, 0x97, 0xb4 } };


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
	PluginDialogBuilder window(_PSI, _FPG, _MenuGuid, ps_title, L"Contents", NULL);
	
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

	// Показываем окно и ждем действие пользователя
	bool isBegin = window.ShowDialog(); 

	while (isBegin)
	{
		if (is_digits(SizeImagedefault)) {
			SizeImage = wcstol(SizeImagedefault, nullptr, 10);

			PluginDialogBuilder InfoWindow(_PSI, _FPG, _MenuGuid, ps_title, L"Contents");

			InfoWindow.StartColumns(); // Делим окно на колонки
			InfoWindow.AddText(MSelectData); // Сообщение Выбранные параметры:
			FarDialogItem* Type = InfoWindow.AddReadonlyEditField(type_image[CurrentTypeImage].c_str(), 15);
			InfoWindow.AddTextBefore(Type, MImageType);
			FarDialogItem* Size = InfoWindow.AddReadonlyEditField((wchar_t*)&SizeImagedefault, 15);
			InfoWindow.AddTextBefore(Size, MSizeImage);
			InfoWindow.ColumnBreak(); // Добавить столбик
			InfoWindow.EndColumns(); // Закрываем колонки

			InfoWindow.AddText(MTitle3);

			InfoWindow.AddSeparator();
			InfoWindow.AddButtons(1, new int{ MOk }, DIF_DISABLE);
			InfoWindow.ShowDialog();

			HANDLE hScreen = _PSI.SaveScreen(0, 0, -1, -1);
			Sleep(5000); // Вставка кода с парсером матрицы
			_PSI.RestoreScreen(hScreen);

			isBegin = false;
		}
		else {
			PluginDialogBuilder WarningWindow(_PSI, _FPG, _MenuGuid, MError, L"Contents", nullptr, nullptr, FMSG_WARNING);

			WarningWindow.AddText(MIncorrectData);
			WarningWindow.AddSeparator();
			WarningWindow.AddButtons(1, new int{ MOk });
			WarningWindow.ShowDialog();

			isBegin = window.ShowDialog();
		}
	}
	
	return isBegin;
}