// TestYandex.cpp: определяет точку входа для консольного приложения.
// Входной файл: "file"
// Выходной файл: "file_rlc"

#include "stdafx.h"

#include <iostream>
#include <fstream>
#include <stdint.h>
#include <typeinfo>
#include <list>
#include <map>
#include <Windows.h>


struct THeader {
	THeader()
		: key(0), flags(0), crc(0), size(0)
	{}

    uint64_t        key;
    uint64_t        flags;
    uint64_t        crc;
    uint64_t        size;
};

struct TElement {
	TElement(const THeader& _header, const uint64_t& _offset)
		: header(_header)
		, offset(_offset)
	{}

    THeader			header;
	uint64_t		offset;
};

typedef std::multimap<uint64_t, TElement*> TElementsMap;
typedef std::pair<uint64_t, TElement*> TElementPair;

void RelocateFile(const char* fileName, TElementsMap elementsMap)
{
	std::string relocateFileName = std::string(fileName).append("_rlc");
	std::ofstream outFile(relocateFileName.data(), std::ios_base::binary);
	std::ifstream inputStream(fileName, std::ios_base::binary);

	for(TElementsMap::iterator it = elementsMap.begin(); it != elementsMap.end(); it++)
	{
		inputStream.seekg((*it).second->offset, 0);
		uint64_t sizeToRelocate = (*it).second->header.size + sizeof(THeader);
		char* data = new char[sizeToRelocate];
		inputStream.read(data, sizeToRelocate);
		outFile.write(data, sizeToRelocate);
		delete data;
	}

	inputStream.close();
	outFile.close();
}

void WriteData(uint64_t key, uint64_t size, std::ofstream& outFile)
{
	THeader header;
	header.key = key;
	header.size = size;

	char* data = new char[size];
	ZeroMemory(data, size);

	outFile.write(reinterpret_cast<char*>(&header), sizeof(THeader));
	outFile.write(data, size);

	delete data;
}

void CreateTestFile()
{
	std::ofstream outFile("file", std::ios_base::binary);
	
	WriteData(5, 16, outFile);
	WriteData(1, 16, outFile);
	WriteData(25, 16, outFile);
	WriteData(16, 16, outFile);
	WriteData(2, 16, outFile);
	
	outFile.close();
}

int _tmain(int argc, _TCHAR* argv[])
{
	//CreateTestFile();
	//return 0;

	TElementsMap elementsMap;

	std::ifstream inputFile("file", std::ios::binary);
	inputFile.seekg(0, std::ios_base::end);
	uint64_t fileSize = inputFile.tellg();

	inputFile.seekg(0, std::ios_base::beg);
	uint64_t offset_abs = 0;

	// читаем файл и собираем карту заголовков
	while(!inputFile.eof() && offset_abs < fileSize)
	{
		const int headerSize = sizeof(THeader);
		char* data = new char[headerSize];
		inputFile.read(data, headerSize);
		THeader* header = reinterpret_cast<THeader*>(data);

		uint64_t offset_rel = headerSize + header->size;
		
		TElement* element = new TElement(*header, offset_abs);
		elementsMap.insert(TElementPair(header->key, element));	// сохраняем заголовок в карту
		
		delete header;
		offset_abs += offset_rel;

		inputFile.seekg(offset_abs, 0); // пропускаем данные
	}

	inputFile.close();

	// в силу особенностей классов типа map объект elementsMap будет автоматически отсортирован по ключу
	
	RelocateFile("file", elementsMap);	// собираем новый файл с отсортированными данными

	return 0;
}

