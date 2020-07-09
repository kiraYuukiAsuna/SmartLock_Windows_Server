#pragma once
/*
配置文件格式如下：
	key1 = value1
	key2 = value2
键值以=连接，一条记录以换行符分割
*/
#include<stdio.h>
#include<stdlib.h>
#include<string.h>

void trim(char* strIn, char* strOut);
//去除字符串首位空格

void getValue(char* keyAndValue, char* key, char* value);
//根据key得到value

int writeConfig(const char* filename /*in*/, const char* key /*in*/, const char* value /*in*/);
//写入配置文件

void readConfig(const char* filename /*in*/, const char* key /*in*/, const char** value /*out*/);
//读取配置文件