/****************************
description:loadlibrary from dll file
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
int isPEfile(FILE* hfile);
int isDLLfile(FILE* hfile);
int main(int argc, char* argv[]) {
	FILE* hfile = NULL;
	fopen_s(&hfile, "D:\\Assignment\\SoftwareSecurity\\���Ĵ���ҵ\\dDisk.dll", "r");
	
	if (hfile == NULL) {
		printf("open file error:%d", GetLastError());
		fclose(hfile);
		return 1;
	}
	int i = isDLLfile(hfile);
	fseek(hfile, 0, SEEK_END);
	int filesize = ftell(hfile);
	if (filesize <= 0) {
		fclose(hfile);
		return 2;
	}


	fclose(hfile);
	return 0;
}
//�ж��Ƿ�PE�ļ�
int isPEfile(FILE* hfile)
{
	//���MZͷ
	fseek(hfile, 0, SEEK_SET);
	char mz[2] = { 0 };
	fread(mz, 1, 2, hfile);
	if (mz[0] != 'M' || mz[1] != 'Z') return 0;
	//���PEͷ
	DWORD peaddr;
	char pe[4] = { 0 };
	fseek(hfile, 0x3c, SEEK_SET);
	fread(&peaddr, 4, 1, hfile);
	fseek(hfile, (long)peaddr, SEEK_SET);
	fread(pe, 1, 4, hfile);
	if (pe[0] == 'P' && pe[1] == 'E' && pe[2] == '\0' && pe[3] == '\0') return 1;
	return 0;
}
//�ж��Ƿ�DLL�ļ�
int isDLLfile(FILE* hfile)
{
	//�ж��Ƿ�PE�ļ�
	if (!isPEfile(hfile)) return 0;
	//��ȡPE�ļ�ͷ
	DWORD peaddr;
	char pe[4] = { 0 };
	fseek(hfile, 0x3c, SEEK_SET);
	fread(&peaddr, 4, 1, hfile);
	IMAGE_NT_HEADERS ntheader;
	fseek(hfile, (long)peaddr, SEEK_SET);
	fread(&ntheader, sizeof(IMAGE_NT_HEADERS), 1, hfile);
	if (ntheader.FileHeader.Characteristics & IMAGE_FILE_DLL) return 1;
	return 0;
}
