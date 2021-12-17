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
using namespace std::experimental;

PluginStartupInfo    _PSI;
FarStandardFunctions _FSF;

//! Plugin GUID
const GUID _FPG = { 0x9d4a59d9, 0xad2d, 0x478c, { 0x8f, 0x66, 0x7d, 0x23, 0x3c, 0xbb, 0x78, 0x8d } };
const GUID _MenuGuid = { 0xa0fb9eb5, 0xedee, 0x42c3, {0xa3, 0x7f, 0x54, 0x9d, 0xe8, 0xc5, 0x97, 0xb4 } };

/* Функция открытия картинки */
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

bool is_digits(const std::wstring& str) {
	return std::all_of(str.begin(), str.end(), isdigit);
}

bool ShowDialog(path path, bool isDir) {

	// Создаем главное окошко
	PluginDialogBuilder window(_PSI, _FPG, _MenuGuid, ps_title, L"Contents", NULL);
	
	// ****************** КОЛОНКИ ***************************// 
	window.StartColumns(); // Делим окно на колонки
	
	window.AddText(MTypeImage); // Сообщение выберите тип картинки
	const int TypeImage[] = { MTypeBMP, MTypePNG }; // Список расширений картинки
	window.AddRadioButtons(&CurrentTypeImage, ARRAYSIZE(TypeImage), TypeImage, true);  // Добавление радиокнопок
	
	window.ColumnBreak(); // Добавить столбик

	window.AddText(MMatrixSize); // Сообщение Размер матрицы
	const int TypeSize[] = { MIG, MKUSLAU }; // Список размерность матрицы как читать
	window.AddRadioButtons(&CurrentTypeMatrixSize, ARRAYSIZE(TypeSize), TypeSize, true);  // Добавление радиокнопок

	window.EndColumns(); // Закрываем колонки

	if (isDir) {
		window.StartColumns(); // Делим окно на колонки
		window.AddText(MFileExt); // Сообщение "Расширение файлов"
		const int TypeExt[] = { MTXT, MDAT }; // Список расширение файлов
		window.AddRadioButtons(&CurrentTypeExtFile, ARRAYSIZE(TypeExt), TypeExt, true);  // Добавление радиокнопок
		window.ColumnBreak(); // Добавить столбик
		window.EndColumns();
	}
	//********************************************************//

	window.AddSeparator(); // Вставить черный разделитель

	FarDialogItem* item = window.AddEditField((wchar_t*)&SizeImagedefault, 15, 15); // Добавление текстового поля "Размерность картинки"
	window.AddTextBefore(item, MSizeImage);
	window.AddSeparator(); // Вставить черный разделитель
	item = window.AddEditField((wchar_t*)&SizeMatrix, 15, 15); // Добавление текстового поля "Размерность матрицы"
	window.AddTextBefore(item, MMatrixSizeLabel);

	window.AddSeparator(); // Вставить черный разделитель

	// Флаги
	window.AddCheckbox(MGrade, &isGrade);
	window.AddCheckbox(MShowImage, &isShowImage);

	// Кнопки
	window.AddOKCancel(MBegin, MCancel);

	// Показываем окно и ждем действие пользователя
	bool isBegin = window.ShowDialog(); 

	if (is_digits(SizeImagedefault) && is_digits(SizeMatrix)) {
		PathFiles pathfiles;
		SizeImage            = wcstol(SizeImagedefault, nullptr, 10);
		pathfiles.SizeMatrix = wcstol(SizeMatrix,       nullptr, 10);
		switch (CurrentTypeExtFile)
		{
		case 0:
			pathfiles.fdi = path / "di.txt";
			pathfiles.fgg = path / "gg.txt";
			pathfiles.fig = path / "ig.txt";
			pathfiles.fjg = path / "jg.txt";
			if (pathfiles.SizeMatrix == 0)
				pathfiles.fkuslau = path / "kuslau.txt";
			break;
		case 1:
			pathfiles.fdi = path / "di.dat";
			pathfiles.fgg = path / "gg.dat";
			pathfiles.fig = path / "ig.dat";
			pathfiles.fjg = path / "jg.dat";
			if (pathfiles.SizeMatrix == 0)
				pathfiles.fkuslau = path / "kuslau.dat";
			break;
		case 2:
			pathfiles.fdi = path / ("di" + expansion);
			pathfiles.fgg = path / ("gg" + expansion);
			pathfiles.fig = path / ("ig" + expansion);
			pathfiles.fjg = path / ("jg" + expansion);
			if (pathfiles.SizeMatrix == 0)
				pathfiles.fkuslau = path / ("kuslau" + expansion);
			break;
		}

		HANDLE hScreen = _PSI.SaveScreen(0, 0, -1, -1); // Остановка плагина 

		// Создание Парсера 
		Parser parser(pathfiles);
		if (!parser.GetCorrect()) return NULL; // Если данные некорректные

		parser.Matrix_to_Pixel(SizeImage);	      // Создаем портрет матрицы
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
		
		// Создаем папку, где сохраним картинку и информацию о матрице
		filesystem::create_directory(path.string() + "/output");

		// Сохраняем картинку в файл
		Image.save_image(path.string() + "/output/portrait.bmp");

		// Показать сразу картинку
		if (isShowImage) {
			image* img = open_image((path.wstring() + L"/output/portrait.bmp").c_str(), false);
			if (img) {
				viewer v(img);
				v.show();
			}
		}

		// Запись информации о матрице
		InfoSparse MatInfo = parser.GetInfo();
		std::ofstream fout(path.string() + "/output/info.txt");
		fout << "Количество элементов: \t\t\t"           << MatInfo.count_all			  << std::endl;
		fout << "Количество положительных элементов: \t" << MatInfo.count_positive		  << std::endl;
		fout << "Количество отрицательных элементов: \t" << MatInfo.count_negative		  << std::endl;
		fout << "Количество ненулевых элементов: \t"     << MatInfo.count_non_zero		  << std::endl;
		fout << "Количество нулевых элементов: \t\t"     << MatInfo.count_zero            << std::endl;
		fout << "Процент заполненности: \t\t\t"          << MatInfo.koef_rate      << "%" << std::endl;
		fout << "Максимальный элемент: \t\t\t"           << MatInfo.max_element           << std::endl;
		fout << "Минимальный элемент: \t\t\t"            << MatInfo.min_element           << std::endl;
		fout.close();

		_PSI.RestoreScreen(hScreen);
	}
	else {
		PluginDialogBuilder WarningWindow(_PSI, _FPG, _MenuGuid, MError, L"Contents", nullptr, nullptr, FMSG_WARNING);

		WarningWindow.AddText(MIncorrectData);
		WarningWindow.AddSeparator();
		WarningWindow.AddButtons(1, new int{ MOk });
		WarningWindow.ShowDialog();

		isBegin = window.ShowDialog();
	}
	return isBegin;
}