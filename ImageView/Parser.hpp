#include <experimental/filesystem>
#include <iostream>
#include <fstream>
#include <vector>
#include <plugin.hpp>

#include "common.h"
#include "string_rc.h"


using std::experimental::filesystem::path;

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
    Parser(path _path) {
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
    plugin_string Creat_Parser(path);
};

plugin_string Parser::Creat_Parser(path _path) {
    std::ifstream fin(_path / "kuslau.txt");
    if (!fin.is_open()) return MErrorFileKUSLAU;
    fin >> _size;
    fin.close();

    di.resize(_size);
    ig.resize(_size + 1);

    fin.open(_path / "di.txt");
    if (!fin.is_open()) return MErrorFileDI;
    for (size_t i = 0; i < _size; i++)
        fin >> di[i];
    fin.close();

    fin.open(_path / "ig.txt");
    if (!fin.is_open()) return MErrorFileIG;
    for (size_t i = 0; i < _size + 1; i++)
        fin >> ig[i];
    fin.close();

    jg.resize(ig[_size]);
    gg.resize(ig[_size]);

    fin.open(_path / "jg.txt");
    if (!fin.is_open()) return MErrorFileJG;
    for (size_t i = 0; i < ig[_size]; i++)
        fin >> jg[i];
    fin.close();

    fin.open(_path / "gg.txt");
    if (!fin.is_open()) return MErrorFileGG;
    for (size_t i = 0; i < ig[_size]; i++)
        fin >> gg[i];
    fin.close();

    return ps_title; // ps_title = 0;
}

void Parser::Matrix_to_Pixel(size_t size_pict) {

    matrix.resize(size_pict);
    for (size_t i = 0; i < size_pict; i++)
        matrix[i].resize(size_pict);

    double k = size_pict / _size;

    if (k < 1) {
        for (size_t i = 0; i < _size; i++) {
            int i0 = ig[i];
            int i1 = ig[i + 1];

            for (int j = i0; j < i1; j++)
            {
                if (gg[j] > 0)
                    matrix[(int)(k * i)][(int)(k * jg[j])] = 1;
                if (gg[j] < 0)
                    matrix[(int)(k * i)][(int)(k * jg[j])] = -1;
            }
        }
    }
    else {
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