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
	if (!isDLLfile(fhandle)) {
		printf("������Ч��dll�ļ�");
		return 2;
	}
	//��ʼ����
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
