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

/* ������� �������� �������� */
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

	// ������� ������� ������
	PluginDialogBuilder window(_PSI, _FPG, _MenuGuid, ps_title, L"Contents", NULL);
	
	// ****************** ������� ***************************// 
	window.StartColumns(); // ����� ���� �� �������
	
	window.AddText(MTypeImage); // ��������� �������� ��� ��������
	const int TypeImage[] = { MTypeBMP, MTypePNG }; // ������ ���������� ��������
	window.AddRadioButtons(&CurrentTypeImage, ARRAYSIZE(TypeImage), TypeImage, true);  // ���������� �����������
	
	window.ColumnBreak(); // �������� �������

	window.AddText(MMatrixSize); // ��������� ������ �������
	const int TypeSize[] = { MIG, MKUSLAU }; // ������ ����������� ������� ��� ������
	window.AddRadioButtons(&CurrentTypeMatrixSize, ARRAYSIZE(TypeSize), TypeSize, true);  // ���������� �����������

	window.EndColumns(); // ��������� �������

	if (isDir) {
		window.StartColumns(); // ����� ���� �� �������
		window.AddText(MFileExt); // ��������� "���������� ������"
		const int TypeExt[] = { MTXT, MDAT }; // ������ ���������� ������
		window.AddRadioButtons(&CurrentTypeExtFile, ARRAYSIZE(TypeExt), TypeExt, true);  // ���������� �����������
		window.ColumnBreak(); // �������� �������
		window.EndColumns();
	}
	//********************************************************//

	window.AddSeparator(); // �������� ������ �����������

	FarDialogItem* item = window.AddEditField((wchar_t*)&SizeImagedefault, 15, 15); // ���������� ���������� ���� "����������� ��������"
	window.AddTextBefore(item, MSizeImage);
	window.AddSeparator(); // �������� ������ �����������
	item = window.AddEditField((wchar_t*)&SizeMatrix, 15, 15); // ���������� ���������� ���� "����������� �������"
	window.AddTextBefore(item, MMatrixSizeLabel);

	window.AddSeparator(); // �������� ������ �����������

	// �����
	window.AddCheckbox(MGrade, &isGrade);
	window.AddCheckbox(MShowImage, &isShowImage);

	// ������
	window.AddOKCancel(MBegin, MCancel);

	// ���������� ���� � ���� �������� ������������
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

		HANDLE hScreen = _PSI.SaveScreen(0, 0, -1, -1); // ��������� ������� 

		// �������� ������� 
		Parser parser(pathfiles);
		if (!parser.GetCorrect()) return NULL; // ���� ������ ������������

		parser.Matrix_to_Pixel(SizeImage);	      // ������� ������� �������
		std::vector<std::vector<int>> portrait = parser.GetPortrait();
		bitmap_image Image(SizeImage, SizeImage); // ������� ��������
		Image.set_all_channels(255, 255, 255);    // �������� �������� ����� ������

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
		
		// ������� �����, ��� �������� �������� � ���������� � �������
		filesystem::create_directory(path.string() + "/output");

		// ��������� �������� � ����
		Image.save_image(path.string() + "/output/portrait.bmp");

		// �������� ����� ��������
		if (isShowImage) {
			image* img = open_image((path.wstring() + L"/output/portrait.bmp").c_str(), false);
			if (img) {
				viewer v(img);
				v.show();
			}
		}

		// ������ ���������� � �������
		InfoSparse MatInfo = parser.GetInfo();
		std::ofstream fout(path.string() + "/output/info.txt");
		fout << "���������� ���������: \t\t\t"           << MatInfo.count_all			  << std::endl;
		fout << "���������� ������������� ���������: \t" << MatInfo.count_positive		  << std::endl;
		fout << "���������� ������������� ���������: \t" << MatInfo.count_negative		  << std::endl;
		fout << "���������� ��������� ���������: \t"     << MatInfo.count_non_zero		  << std::endl;
		fout << "���������� ������� ���������: \t\t"     << MatInfo.count_zero            << std::endl;
		fout << "������� �������������: \t\t\t"          << MatInfo.koef_rate      << "%" << std::endl;
		fout << "������������ �������: \t\t\t"           << MatInfo.max_element           << std::endl;
		fout << "����������� �������: \t\t\t"            << MatInfo.min_element           << std::endl;
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