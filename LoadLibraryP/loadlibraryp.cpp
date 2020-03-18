/****************************
description:load dll file
date:2019/3/17
version:0.0.0
--tkyzp
*****************************/
//#define DEBUG
#ifdef DEBUG
	#define _CRT_SECURE_NO_WARNINGS
#endif // DEBUG

#include<stdio.h>
#include<stdlib.h>
#include<Windows.h>
#include<winnt.h>



int isPEfile(HANDLE hfile);
int isDLLfile(HANDLE hfile);
int main(int argc, char* argv[]) {
	//���ļ����
	HANDLE fhandle = CreateFile(L"D:\\Assignment\\SoftwareSecurity\\���Ĵ���ҵ\\dDisk.dll",
		GENERIC_READ,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_FLAG_OVERLAPPED,
		NULL);
	if (fhandle == INVALID_HANDLE_VALUE) {
		printf("open file error:%d", GetLastError());
		return 1;
	}

	//����Ƿ�dll
	//�ж��Ƿ�PE�ļ�
	if (!isPEfile(fhandle)) {
		CloseHandle(fhandle);
		return 2;
	}
	//��ȡPE�ļ�ͷ
	OVERLAPPED over = { 0 };
	DWORD readsize = 0;
	DWORD peaddr = 0;
	IMAGE_NT_HEADERS ntheader;
	over.Offset = 0x3c;
	ReadFile(fhandle, &peaddr, 4, &readsize, &over);
	over.Offset = peaddr;
	ReadFile(fhandle, &ntheader, sizeof(IMAGE_NT_HEADERS), &readsize, &over);
	if (!(ntheader.FileHeader.Characteristics & IMAGE_FILE_DLL)) {
		CloseHandle(fhandle);
		return 2;
	}
	//��ʼ����
	//�����ڴ�
	void* image_base = VirtualAlloc((void*)ntheader.OptionalHeader.ImageBase,
		ntheader.OptionalHeader.SizeOfImage,
		MEM_RESERVE|MEM_COMMIT,
		PAGE_READWRITE);
	if (image_base == NULL) {
		printf("�����ڴ�ʧ��:%d", GetLastError());
		CloseHandle(fhandle);
		return 3;
	}
	//���ڴ���0
	RtlZeroMemory(image_base, ntheader.OptionalHeader.SizeOfImage);
	int RElocation = 0;//�Ƿ���Ҫ�ض�λ

	if ((DWORD)image_base != ntheader.OptionalHeader.ImageBase) RElocation = 1;

	//���ƽڼ��ض�λ��tkyzp��
	//��ȡ�ڱ�
	IMAGE_SECTION_HEADER* section_header = (IMAGE_SECTION_HEADER*)malloc(sizeof(IMAGE_SECTION_HEADER) * ntheader.FileHeader.NumberOfSections);
	if (section_header == NULL) {
		printf("�����ڴ�ʧ��:%d", GetLastError());
		CloseHandle(fhandle);
		return 3;
	}
	DWORD section_header_ptr = peaddr + 0x18 + ntheader.FileHeader.SizeOfOptionalHeader;
	over.Offset = section_header_ptr;
	ReadFile(fhandle, section_header,ntheader.FileHeader.NumberOfSections * sizeof(IMAGE_SECTION_HEADER),&readsize,&over);
	//�����ڱ����ƽ�
	unsigned char* buffer = NULL;
	void* dest = NULL;
	DWORD VA = 0;
	DWORD access = 0;
	for (int i = 0; i < ntheader.FileHeader.NumberOfSections; i++) {
		
		//���仺����
		buffer = (unsigned char*)malloc(section_header[i].SizeOfRawData);
		if (buffer == NULL) {
			printf("�����ڴ�ʧ��:%d", GetLastError());
			CloseHandle(fhandle);
			return 3;
		}
		//���ļ��ж�ȡ��
		
		over.Offset = section_header[i].PointerToRawData;
		ReadFile(fhandle, buffer, section_header[i].SizeOfRawData, &readsize, &over);
		//���ƽ�
		/*
		//��ȡ��Ȩ��
		switch (section_header[i].Characteristics & 0xa0000000) {
		case IMAGE_SCN_MEM_EXECUTE:
			access = PAGE_EXECUTE_READ;
			break;
		case IMAGE_SCN_MEM_WRITE:
			access = PAGE_READWRITE;
			break;
		case IMAGE_SCN_MEM_EXECUTE| IMAGE_SCN_MEM_WRITE:
			access = PAGE_EXECUTE_READWRITE;
			break;
		default:
			access = PAGE_READONLY;
		}
		*/
		//���������ַ
		VA = (DWORD)image_base + section_header[i].VirtualAddress;
		//�������ݵ������ַ
		RtlCopyMemory((void*)VA, buffer, section_header[i].SizeOfRawData);
		//�ύ����
		dest = VirtualAlloc((void*)VA,
			section_header[i].SizeOfRawData,
			MEM_COMMIT,
			PAGE_READWRITE);
		if (dest == NULL) return 4;
		free(buffer);
	}
	//�ض�λ
	if (RElocation) {
		//��ȡ�ض�λ��
		DWORD reloc_va = (DWORD)image_base + ntheader.OptionalHeader.DataDirectory[5].VirtualAddress;//�ض�λ�������ַ
		DWORD reloc_size = ntheader.OptionalHeader.DataDirectory[5].Size;//�ض�λ���С
		DWORD reloc_offset = 0;//�ض�λ�����ض�λ����ƫ��
		IMAGE_BASE_RELOCATION cur_reloc_tab = { 0 };
		WORD* TypeOffset = NULL;
		while (reloc_offset < reloc_size)
		{
			cur_reloc_tab = *(IMAGE_BASE_RELOCATION*)(reloc_va + reloc_offset);//��ǰ�ض�λ��ͷ
			TypeOffset = (WORD*)(reloc_va + reloc_offset + 0x8);//�ض�λ������
			//��Ҫ�޸��ض�λ��
			for (int i = 0; i < cur_reloc_tab.SizeOfBlock / 2 - 4; i++) {
				if ((TypeOffset[i] & 0xf000) == 0x3000) {
					*(DWORD*)((DWORD)image_base + cur_reloc_tab.VirtualAddress + (TypeOffset[i] & 0xfff)) += (DWORD)image_base - ntheader.OptionalHeader.ImageBase;
				}
			}
			reloc_offset += cur_reloc_tab.SizeOfBlock;
		}
	}






	//�����������ף�
	//TODO:

	//���������������
	//TODO:

	//�ڴ汣���������ڴ�Ȩ��
	for (int i = 0; i < ntheader.FileHeader.NumberOfSections; i++) {
		//��ȡ��Ȩ��
		switch (section_header[i].Characteristics & 0xa0000000) {
		case IMAGE_SCN_MEM_EXECUTE:
			access = PAGE_EXECUTE_READ;
			break;
		case IMAGE_SCN_MEM_WRITE:
			access = PAGE_READWRITE;
			break;
		case IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_WRITE:
			access = PAGE_EXECUTE_READWRITE;
			break;
		default:
			access = PAGE_READONLY;
		}
		//���������ַ
		VA = (DWORD)image_base + section_header[i].VirtualAddress;
		dest = VirtualAlloc((void*)VA,
			section_header[i].SizeOfRawData,
			MEM_COMMIT,
			access);
	}
	//����dll���


	//������������

	//VirtualFree(memory, ntheader.OptionalHeader.SizeOfImage,);
	CloseHandle(fhandle);
	return 0;
}
//�ж��Ƿ�PE�ļ�
int isPEfile(HANDLE hfile)
{
	OVERLAPPED over = { 0 };
	DWORD readsize = 0;
	//���MZͷ
	char mz[2] = { 0 };
	ReadFile(hfile, mz, 2, &readsize, &over);
	if (mz[0] != 'M' || mz[1] != 'Z') return 0;
	//���PEͷ
	DWORD peaddr = 0;
	char pe[4] = { 0 };
	over.Offset = 0x3c;
	ReadFile(hfile, &peaddr, 4, &readsize, &over);
	over.Offset = peaddr;
	ReadFile(hfile, pe, 4, &readsize, &over);
	if (pe[0] == 'P' && pe[1] == 'E' && pe[2] == '\0' && pe[3] == '\0') return 1;
	return 0;
}

//�ж��Ƿ�DLL�ļ�
int isDLLfile(HANDLE hfile)
{
	//�ж��Ƿ�PE�ļ�
	if (!isPEfile(hfile)) return 0;
	//��ȡPE�ļ�ͷ
	OVERLAPPED over = { 0 };
	DWORD readsize = 0;
	DWORD peaddr = 0;
	IMAGE_NT_HEADERS ntheader;
	over.Offset = 0x3c;
	ReadFile(hfile, &peaddr, 4, &readsize, &over);
	over.Offset = peaddr;
	ReadFile(hfile, &ntheader, sizeof(IMAGE_NT_HEADERS), &readsize, &over);
	if (ntheader.FileHeader.Characteristics & IMAGE_FILE_DLL) return 1;
	return 0;
}