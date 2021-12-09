#pragma once
#include <algorithm>
#include <fstream>
#include <map>

#include "DlgBuilder.hpp"
#include "common.h"
#include "string_rc.h"
#include "version.h"
#include "bitmap_image.hpp"
#include "Parser.hpp"
#include "image_lib.h"
#include "viewer.h"
#include "viewer_qv.h"

using std::experimental::filesystem::path;

PluginStartupInfo    _PSI;
FarStandardFunctions _FSF;

//! Plugin GUID {9D4A59D9-AD2D-478C-8F66-7D233CBB788D}
const GUID _FPG = { 0x9d4a59d9, 0xad2d, 0x478c, { 0x8f, 0x66, 0x7d, 0x23, 0x3c, 0xbb, 0x78, 0x8d } };
const GUID _MenuGuid = { 0xa0fb9eb5, 0xedee, 0x42c3, {0xa3, 0x7f, 0x54, 0x9d, 0xe8, 0xc5, 0x97, 0xb4 } };

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
static rgb_t rgb_p = {255, 0, 0};
static rgb_t rgb_n = {0, 0, 255};
static rgb_t rgb = { 0, 0, 0 };


bool ShowDialog(path path, Parser parser) {

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
			parser.Matrix_to_Pixel(SizeImage);	// Создаем портрет матрицы
			std::vector<std::vector<int>> portrait = parser.GetPortrait();
			bitmap_image Image(SizeImage, SizeImage); // Создаем картинку
			Image.set_all_channels(255, 255, 255);    // Заливаем картинку белым цветом

			if (isGrade) {
				for (size_t y = 0; y < SizeImage; y++)
					for (size_t x = 0; x < SizeImage; x++)
						if (portrait[x][y] > 0) {
							Image.set_pixel(x, y, rgb_p);
							Image.set_pixel(y, x, rgb_p);
						}
						else if (portrait[x][y] < 0) {
							Image.set_pixel(x, y, rgb_n);
							Image.set_pixel(y, x, rgb_n);
						}
			}
			else {
				for (size_t y = 0; y < SizeImage; y++)
					for (size_t x = 0; x < SizeImage; x++)
						if (portrait[x][y] != 0) {
							Image.set_pixel(x, y, rgb);
							Image.set_pixel(y, x, rgb);
						}
			}
			
			// Сохраняем картинку в файл
			Image.save_image(path.string() + "/output.bmp");

			// Показать сразу картинку
			if (isShowImage) {
				image* img = open_image((path.wstring() + L"/output.bmp").c_str(), false);
				if (img) {
					viewer v(img);
					v.show();
				}
			}

			
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