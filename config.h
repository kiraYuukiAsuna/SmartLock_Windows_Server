#pragma once
/*
�����ļ���ʽ���£�
	key1 = value1
	key2 = value2
��ֵ��=���ӣ�һ����¼�Ի��з��ָ�
*/
#include<stdio.h>
#include<stdlib.h>
#include<string.h>

void trim(char* strIn, char* strOut);
//ȥ���ַ�����λ�ո�

void getValue(char* keyAndValue, char* key, char* value);
//����key�õ�value

int writeConfig(const char* filename /*in*/, const char* key /*in*/, const char* value /*in*/);
//д�������ļ�

void readConfig(const char* filename /*in*/, const char* key /*in*/, const char** value /*out*/);
//��ȡ�����ļ�