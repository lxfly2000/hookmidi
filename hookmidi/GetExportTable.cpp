#include "GetExportTable.h"
#include <Windows.h>
#include <ImageHlp.h>
#pragma comment(lib,"ImageHlp.lib")

//https://blog.csdn.net/simon798/article/details/103564491
// �ڴ�ƫ��ת�ļ�ƫ��
int rva_to_raw(PIMAGE_SECTION_HEADER pSection, int nSectionNum, DWORD nRva)
{
	int nRet = 0;

	// ��������
	for (int i = 0; i < nSectionNum; i++) {
		// �������ַ�����������
		if (pSection[i].VirtualAddress <= nRva && nRva < pSection[i + 1].VirtualAddress) {
			// �ļ�ƫ�� = �öε� PointerToRawData + ���ڴ�ƫ�� - �ö���ʼ��RVA(VirtualAddress)��
			nRet = nRva - pSection[i].VirtualAddress + pSection[i].PointerToRawData;
			break;
		}
	}

	return nRet;
}

std::vector<std::string> GetExportTable(const char* path)
{
    std::vector<std::string>table;
    PLOADED_IMAGE pi = ImageLoad(path, path);
	if (pi)
	{
		// �����Ʒ�ʽ���ļ�
		PBYTE pFile = pi->MappedAddress;

		// �� dos ͷ
		IMAGE_DOS_HEADER dosHeader;
		memcpy(&dosHeader, pFile, sizeof(IMAGE_DOS_HEADER));

		// �� nt ͷ
		IMAGE_NT_HEADERS32 ntHeader32{};
		IMAGE_NT_HEADERS64 ntHeader64{};
		pFile = pi->MappedAddress + dosHeader.e_lfanew;
		WORD machine = reinterpret_cast<PIMAGE_NT_HEADERS>(pFile)->FileHeader.Machine;
		int nSectionNum = 0;
		switch (machine)
		{
		case IMAGE_FILE_MACHINE_I386:default:
			memcpy(&ntHeader32, pFile, sizeof(IMAGE_NT_HEADERS32));
			pFile += sizeof(IMAGE_NT_HEADERS32);
			if (!ntHeader32.OptionalHeader.DataDirectory[0].VirtualAddress)
				return table;
			nSectionNum = ntHeader32.FileHeader.NumberOfSections;
			break;
		case IMAGE_FILE_MACHINE_AMD64:
		case IMAGE_FILE_MACHINE_IA64:
			memcpy(&ntHeader64, pFile, sizeof(IMAGE_NT_HEADERS64));
			pFile += sizeof(IMAGE_NT_HEADERS64);
			if (!ntHeader64.OptionalHeader.DataDirectory[0].VirtualAddress)
				return table;
			nSectionNum = ntHeader64.FileHeader.NumberOfSections;
			break;
		}

		// ������ͷ
		std::vector<IMAGE_SECTION_HEADER>Section;
		for (int i = 0; i < nSectionNum; i++)
			Section.push_back(reinterpret_cast<PIMAGE_SECTION_HEADER>(pFile)[i]);

		// ���㵼���� RAW
		IMAGE_EXPORT_DIRECTORY expDir;
		int nExportOffset = 0;
		switch (machine)
		{
		case IMAGE_FILE_MACHINE_I386:default:
			nExportOffset = rva_to_raw(Section.data(), nSectionNum, ntHeader32.OptionalHeader.DataDirectory[0].VirtualAddress);
			break;
		case IMAGE_FILE_MACHINE_AMD64:
		case IMAGE_FILE_MACHINE_IA64:
			nExportOffset = rva_to_raw(Section.data(), nSectionNum, ntHeader64.OptionalHeader.DataDirectory[0].VirtualAddress);
			break;
		}
		if (!nExportOffset)
			return table;

		// ��������
		pFile = pi->MappedAddress + nExportOffset;
		memcpy(&expDir, pFile, sizeof(IMAGE_EXPORT_DIRECTORY));

		// ��������ͷ
		pFile = pi->MappedAddress + rva_to_raw(Section.data(), nSectionNum, expDir.Name);

		// ��ȡ������������
		int nAddressNum = expDir.NumberOfFunctions;

		// ��ȡ����������
		std::vector<DWORD>NameAddress;
		pFile = pi->MappedAddress + rva_to_raw(Section.data(), nSectionNum, expDir.AddressOfNames);
		for (int i = 0; i < nAddressNum; i++)
			NameAddress.push_back(reinterpret_cast<DWORD*>(pFile)[i]);

		// ����������
		for (int i = 0; i < nAddressNum; i++)
		{
			pFile = pi->MappedAddress + rva_to_raw(Section.data(), nSectionNum, NameAddress[i]);
			table.push_back((PCHAR)pFile);
		}
		ImageUnload(pi);
	}

    return table;
}
