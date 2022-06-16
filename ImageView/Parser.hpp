#include <experimental/filesystem>
#include <iostream>
#include <fstream>
#include <vector>
#include <plugin.hpp>

#include "common.h"
#include "string_rc.h"


using std::experimental::filesystem::path;

enum class TypeImage { BMP, PNG };
std::map<int, std::wstring> type_image = {
    { 0, L"BMP"},
    { 1, L"PNG"}
};

enum class TypeExtFile { TXT, DAT, NONE };
std::map<int, std::wstring> type_ext = {
    { 0, L"TXT"},
    { 1, L"DATA"}
};

enum class MatrixSize { IG, KUSLAU };
std::map<int, std::wstring> type_size = {
    { 0, L"IG"},
    { 1, L"KUSLAU"}
};

static int CurrentTypeExtFile = static_cast<int>(TypeExtFile::TXT);
static int CurrentTypeMatrixSize = static_cast<int>(MatrixSize::IG);
static int CurrentTypeImage = static_cast<int>(TypeImage::BMP);
static std::string expansion;
static wchar_t SizeImagedefault[16] = L"100";
static wchar_t SizeMatrix[16] = L"0";
static int isGrade = 0;
static int isShowImage = 0;
static int SizeImage = 0;
static rgb_t rgb_p = { 255, 0, 0 };
static rgb_t rgb_n = { 0, 0, 255 };
static rgb_t rgb = { 0, 0, 0 };

struct InfoSparse {
    size_t count_all = 0;       // Количество элементов в матрице
    size_t count_positive = 0;  // Количество положительных элементов
    size_t count_negative = 0;  // Количество негативных элементов
    size_t count_non_zero = 0;  // Количество ненулевых элементов
    size_t count_zero = 0;      // Количество нулевых элементов 
    double max_element;         // Максимальный элемент 
    double min_element;         // Минимальный элемент

    double koef_rate; /// Коэффициент заполненности
};

struct PathFiles {
    size_t SizeMatrix;
    path fkuslau;
    path fdi;
    path fig;
    path fjg;
    path fgg;
};


class Parser
{
private:
    size_t _size;
    InfoSparse info;

    // Матрица
    std::vector<double> gg;
    std::vector<double> di;
    std::vector<size_t>    ig;
    std::vector<size_t>    jg;

    std::vector<std::vector<int>> matrix; // Картинка

    bool IsCor = true; // Переменная на корректность данных

public:
    Parser(PathFiles _path) {
        plugin_string Mes = Creat_Parser(_path);

        if (Mes) {
            IsCor = false;
            const wchar_t* MsgItems[] =
            {
                _PSI.GetMsg(&_FPG, ps_title),
                _PSI.GetMsg(&_FPG, Mes),
                L"\x01",
                _PSI.GetMsg(&_FPG, MOk),
            };

            _PSI.Message(&_FPG, NULL, FMSG_WARNING | FMSG_LEFTALIGN, L"Contents",
                MsgItems, ARRAYSIZE(MsgItems), 1);
        }
    }

    void print() const;
    bool GetCorrect()    { return IsCor;  };
    InfoSparse GetInfo() { return info;   };
    std::vector<std::vector<int>>& GetPortrait() { return matrix; };

    void Matrix_to_Pixel(size_t);

private:
    plugin_string Creat_Parser(PathFiles _path);
};

plugin_string Parser::Creat_Parser(PathFiles _path) {    
    std::ifstream fin;
    FILE* f;
    std::ofstream out("D:\\hello.txt");
    if (_path.SizeMatrix == 0) {
        switch (CurrentTypeMatrixSize)
        {
        case 0:
            if (expansion == ".dat") {
                f = fopen(_path.fig.string().c_str(), "rb");
                if (!f) return MErrorFileIG;
                int temp;
                fread(&temp, sizeof(int), 1, f);
                do {
                    ig.push_back(temp);
                    fread(&temp, sizeof(int), 1, f);
                } while (!feof(f));
                fclose(f);
                _size = ig.size() - 1;
            }
            else {
                fin.open(_path.fig);
                if (!fin.is_open()) return MErrorFileIG;
                size_t temp;
                while (fin >> temp)
                    ig.push_back(temp);
                if (ig.empty()) return MErrorIncorrectIG;
                fin.close();
                _size = ig.size() - 1;
            }
            break;
        case 1:
            if (expansion == ".dat") {
                f = fopen(_path.fkuslau.string().c_str(), "rb");
                if (!f) return MErrorFileKUSLAU;
                fread(&_size, sizeof(int), 1, f);
                fclose(f);

                ig.resize(_size + 1);
                f = fopen(_path.fig.string().c_str(), "rb");
                if (!f) return MErrorFileIG;
                for (int i = 0; i < _size + 1; i++) {
                    fread(&ig[i], sizeof(int), 1, f);
                }
                fclose(f);
            }
            else {
                fin.open(_path.fkuslau);
                if (!fin.is_open()) return MErrorFileKUSLAU;
                fin >> _size;
                fin.close();

                ig.resize(_size + 1);
                fin.open(_path.fig);
                if (!fin.is_open()) return MErrorFileIG;
                for (size_t i = 0; i < _size + 1; i++)
                    fin >> ig[i];
                fin.close();
            }

            break;
        }
    }
    else {
        _size = _path.SizeMatrix;
        ig.resize(_size + 1);
        if (expansion == ".dat") {
            f = fopen(_path.fig.string().c_str(), "rb");
            if (!f) return MErrorFileIG;
            for (size_t i = 0; i < _size + 1; i++)
                fread(&ig[i], sizeof(int), 1, f);
            fclose(f);
        }
        else {
            fin.open(_path.fig);
            if (!fin.is_open()) return MErrorFileIG;
            for (size_t i = 0; i < _size + 1; i++)
                fin >> ig[i];
            fin.close();
        }
        
    }

    di.resize(_size);
    jg.resize(ig[_size]);
    gg.resize(ig[_size]);

    if (expansion == ".dat") {
        f = fopen(_path.fdi.string().c_str(), "rb");
        if (!f) return MErrorFileDI;
        for (size_t i = 0; i < _size; i++)
            fread(&di[i], sizeof(double), 1, f);
        fclose(f);

        f = fopen(_path.fjg.string().c_str(), "rb");
        if (!f) return MErrorFileJG;
        for (size_t i = 0; i < ig[_size]; i++)
            fread(&jg[i], sizeof(int), 1, f);
        fclose(f);

        f = fopen(_path.fgg.string().c_str(), "rb");
        if (!f) return MErrorFileGG;
        for (size_t i = 0; i < ig[_size]; i++)
            fread(&gg[i], sizeof(double), 1, f);
        fclose(f);
    }
    else {
        fin.open(_path.fdi);
        if (!fin.is_open()) return MErrorFileDI;
        for (size_t i = 0; i < _size; i++)
            fin >> di[i];
        fin.close();

        fin.open(_path.fjg);
        if (!fin.is_open()) return MErrorFileJG;
        for (size_t i = 0; i < ig[_size]; i++)
            fin >> jg[i];
        fin.close();

        fin.open(_path.fgg);
        if (!fin.is_open()) return MErrorFileGG;
        for (size_t i = 0; i < ig[_size]; i++)
            fin >> gg[i];
        fin.close();
    }
    out.close();
    return ps_title; // ps_title = 0;
}

void Parser::Matrix_to_Pixel(size_t size_pict) {

    matrix.resize(size_pict);
    for (size_t i = 0; i < size_pict; i++)
        matrix[i].resize(size_pict);

    double k = (double)size_pict / (double)_size;

    for (size_t i = 0; i < _size; i++) {
        int i0 = ig[i];
        int i1 = ig[i + 1];
        for (int i_b = 0; i_b + k * i < k * (i + 1); i_b++)
        {
            for (int j_b = 0; j_b + k * i < k * (i + 1); j_b++)
            {
                if (di[i] > 0)
                    matrix[(int)(k * i) + i_b][(int)(k * i) + j_b] = 1;
                if (di[i] < 0)
                    matrix[(int)(k * i) + i_b][(int)(k * i) + j_b] = -1;
            }
            for (int j = i0; j < i1; j++)
            {
                for (int i_b = 0; i_b + k * i < k * (i + 1); i_b++)
                {
                    for (int j_b = 0; j_b + k * i < k * (i + 1); j_b++)
                    {
                        if (gg[j] > 0)
                            matrix[(int)(k * i) + i_b][(int)(k * jg[j]) + j_b] = 1;
                        if (gg[j] < 0)
                            matrix[(int)(k * i) + i_b][(int)(k * jg[j]) + j_b] = -1;
                    }
                }
            }
        }
    }


    /// Сбор информации
    size_t pos{ 0 };
    info.max_element = di[0];
    info.min_element = di[0];
    for (size_t i = 0; i < _size; i++) {
        if (di[i] > 0) info.count_positive++;
        if (di[i] < 0) info.count_negative++;
        if (di[i] > info.max_element) info.max_element = di[i];
        if (di[i] < info.min_element) info.min_element = di[i];

        for (size_t j = ig[i]; j < ig[i + 1]; j++, pos++) {
            if (gg[pos] > 0) info.count_positive += 2;
            if (gg[pos] < 0) info.count_negative += 2;
            if (gg[pos] > info.max_element) info.max_element = gg[pos];
            if (gg[pos] < info.min_element) info.min_element = gg[pos];
        }
    }
    info.count_all = _size * _size;
    info.count_non_zero = info.count_negative + info.count_positive;
    info.count_zero = info.count_all - info.count_non_zero;
    info.koef_rate = static_cast<double>(info.count_non_zero) / info.count_all * 100;
}