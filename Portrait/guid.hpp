/*
 Директива препроцессора, которая говорит,
 что содержимое этого файла нужно включить в итоговый код программы лишь один раз
*/
#pragma once

/*
Подключаем заголовочный файл initguid.h от компании Microsoft
для использования макроса DEFINE_GUID, позволяющего объявить новый GUID.
GUID - это глобальный уникальный идентификатор, который можно присвоить вашему плагину, 
сервису, документу и чему угодно.
Чтобы сгенерировать новый GUID можно воспользоваться любым online-сервисом, 
например этим https://www.guidgenerator.com/online-guid-generator.aspx.

Если хотите больше узнать про GUID, начните с статьи на wiki:
https://ru.wikipedia.org/wiki/GUID
*/
#include <initguid.h>

// {40D43854-8283-452B-8878-4332C681B244}
DEFINE_GUID(MainGuid, 0x40d43854, 0x8283, 0x452b, 0x88, 0x78, 
			0x43, 0x32, 0xc6, 0x81, 0xb2, 0x44);

// {A0FB9EB5-EDEE-42C3-A37F-549DE8C597B4}
DEFINE_GUID(MenuGuid, 0xa0fb9eb5, 0xedee, 0x42c3, 0xa3, 0x7f, 
			0x54, 0x9d, 0xe8, 0xc5, 0x97, 0xb4);