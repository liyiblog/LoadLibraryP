/****************************
Description:load dll file
Date:2019/3/22
Version:1.0.0
Author:tkyzp,PPPPPotato,Jin
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
FARPROC getProcAddress(DWORD image_base, DWORD export_table_rva, LPCSTR function_name);
typedef void (*getFileSystemName)(LPCWSTR path);
wchar_t* c2w(const char* str);//charתwchar_t
typedef BOOL(WINAPI* DllEntryProc)(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved);


int main(int argc, char* argv[]) {
	//���ļ����
	HANDLE fhandle = CreateFile(L"dDisk.dll",
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
		MEM_RESERVE | MEM_COMMIT,
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
	ReadFile(fhandle, section_header, ntheader.FileHeader.NumberOfSections * sizeof(IMAGE_SECTION_HEADER), &readsize, &over);
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






	//��������
	//TODO:
	printf("���벿�ֿ�ʼ\n");
	IMAGE_IMPORT_DESCRIPTOR importDesc = { 0 };//����������

	DWORD IDT_va = (DWORD)image_base + ntheader.OptionalHeader.DataDirectory[1].VirtualAddress;//����Ŀ¼�������ַ
	DWORD IDT_size = ntheader.OptionalHeader.DataDirectory[1].Size;//����Ŀ¼���С
	DWORD IDT_offset = 0;//����������������Ŀ¼����ƫ��

	while (IDT_offset < IDT_size - 0x14)//���һ������������Ϊȫ�㣬��������
	{
		importDesc = *(IMAGE_IMPORT_DESCRIPTOR*)(IDT_va + IDT_offset);
		DWORD* nameRef = (DWORD*)((DWORD)image_base + importDesc.OriginalFirstThunk);//INT��
		DWORD* symbolRef = (DWORD*)((DWORD)image_base + importDesc.FirstThunk);//IAT��
		CHAR* dllname = (CHAR*)((DWORD)image_base + importDesc.Name);//dll����
		WCHAR* w_dllname = c2w(dllname);

		HINSTANCE handle = LoadLibrary(w_dllname);//����dll
		for (; *nameRef; nameRef++, symbolRef++)
		{
			PIMAGE_IMPORT_BY_NAME thunkData = (PIMAGE_IMPORT_BY_NAME)((DWORD)image_base + *nameRef);//��ȡ������	
			*symbolRef = (DWORD)GetProcAddress(handle, (LPCSTR)&thunkData->Name);//дIAT��
			if (*symbolRef == 0)
			{
				return 5;
			}
		}
		IDT_offset += 0x14;//��һ������������ƫ��
	}
	printf("���벿�ֽ���\n");


	//���������
	getFileSystemName GetFileSystemName = (getFileSystemName)getProcAddress((DWORD)image_base, ntheader.OptionalHeader.DataDirectory[0].VirtualAddress, "GetFileSystemName");


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
	//����DLL��ڵ㣨��AddressOfEntryPoint���壩�������֪ͨ���йظ��ӵ����̵���Ϣ��
	DllEntryProc entry = (DllEntryProc)((DWORD)image_base + ntheader.OptionalHeader.AddressOfEntryPoint);
	(*entry)((HINSTANCE)image_base, DLL_PROCESS_ATTACH, 0);


	WCHAR path[4];
	swprintf(path, 4, L"%S", "F:\\123.txt");
	GetFileSystemName(path);

	/*
	unsigned char *p = (unsigned char *)GetFileSystemName;
	for (int i = 0; i < 16 * 16; i++) {
		printf("%02x  ", p[i]);
		if ((i + 1) % 16 == 0) {
			printf("\n");
		}
	}
	*/
	system("pause");
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

FARPROC getProcAddress(DWORD image_base, DWORD export_table_rva, LPCSTR function_name) {
	DWORD export_table_address = (DWORD)image_base + export_table_rva;
	IMAGE_EXPORT_DIRECTORY* export_directory = (IMAGE_EXPORT_DIRECTORY*)export_table_address;
	//���Ƚ���������ĵ�ַ���
	DWORD* name_point_table = (DWORD*)(image_base + export_directory->AddressOfNames);
	WORD* ordinal_table = (WORD*)(image_base + export_directory->AddressOfNameOrdinals);
	DWORD* address_table = (DWORD*)(image_base + export_directory->AddressOfFunctions);
	WORD ordinal = 0;
	DWORD* function_address = NULL;
	//���ʵ�һ�������Ӧ�������±�
	bool find_function = false;//�����Ƿ��ҵ������ı�־��
	int index = 0;//�����ҵ����±�
	for (; index < export_directory->NumberOfNames; index++) {
		char* str = (char*)image_base + name_point_table[index];//ָ�������ֵ�ָ��
		if (0 == strcmp(str, function_name)) {
			find_function = true;
			break;
		}
	}
	if (find_function) {
		//��DLL��������ű��ж�����ţ��������ֽڵ��֣�Ȼ����EAT���ҵ������ĵ�ַ��
		ordinal = ordinal_table[index];
		if (ordinal >= export_directory->NumberOfFunctions) {
			printf("��ų����������������ţ���");
		}
		else {
			function_address = (DWORD*)(image_base + address_table[ordinal]);
		}
	}
	else {
		printf("dll�в����ڴ˺���");
	}
	return (FARPROC)function_address;
}

wchar_t* c2w(const char* str)
{
	int length = strlen(str) + 1;
	wchar_t* t = (wchar_t*)malloc(sizeof(wchar_t) * length);
	memset(t, 0, length * sizeof(wchar_t));
	MultiByteToWideChar(CP_ACP, 0, str, strlen(str), t, length);
	return t;
}