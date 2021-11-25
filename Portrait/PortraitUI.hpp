#pragma once
#include "DlgBuilder.hpp"
#include "guid.hpp"
#include "PortraitLang.hpp"
#include "targetver.h"
#include "version.hpp"
#include <algorithm>
#include <fstream>
#include <map>

//����������� ����������, ������� ������ ��������� ��� ������� � Far API.
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

	// ������� ������� ������
	PluginDialogBuilder window(Info, MainGuid, MenuGuid, MTitle, L"Contents", NULL);
	
	window.StartColumns(); // ����� ���� �� �������
	window.AddText(MTypeImage); // ��������� �������� ��� ��������
	const int TypeIDs[] = { MTypeBMP, MTypePNG }; // ������ ���������� ��������
	window.AddRadioButtons(&CurrentTypeImage, ARRAYSIZE(TypeIDs), TypeIDs, true);  // ���������� �����������
	window.ColumnBreak(); // �������� �������
	window.EndColumns(); // ��������� �������

	window.AddSeparator(); // �������� ������ �����������

	FarDialogItem* item = window.AddEditField((wchar_t*)&SizeImagedefault, 15, 15); // ���������� ���������� ����
	window.AddTextBefore(item, MSizeImage);

	window.AddSeparator(); // �������� ������ �����������

	// �����
	window.AddCheckbox(MGrade, &isGrade);
	window.AddCheckbox(MShowImage, &isShowImage);

	// ������
	window.AddOKCancel(MBegin, MCancel);

	
	return window.ShowDialog();
}

	// ���������� ���� � ���� �������� ������������
	//bool isBegin = window.ShowDialog(); 

	//while (isBegin)
	//{
	//	if (is_digits(SizeImagedefault)) {
	//		SizeImage = wcstol(SizeImagedefault, nullptr, 10);

	//		PluginDialogBuilder InfoWindow(Info, MainGuid, MenuGuid, MTitle, L"Contents");

	//		InfoWindow.StartColumns(); // ����� ���� �� �������
	//		InfoWindow.AddText(MSelectData); // ��������� ��������� ���������:
	//		FarDialogItem* Type = InfoWindow.AddReadonlyEditField(type_image[CurrentTypeImage].c_str(), 15);
	//		InfoWindow.AddTextBefore(Type, MImageType);
	//		FarDialogItem* Size = InfoWindow.AddReadonlyEditField((wchar_t*)&SizeImagedefault, 15);
	//		InfoWindow.AddTextBefore(Size, MSizeImage);
	//		InfoWindow.ColumnBreak(); // �������� �������
	//		InfoWindow.EndColumns(); // ��������� �������

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