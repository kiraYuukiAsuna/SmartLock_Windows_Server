#include "config.h"
#pragma warning(disable:4996)
const int flen = 4096;
void trim(char* strIn, char* strOut)
{

    char* start, * end, * temp; //定义去除空格后字符串的头尾指针和遍历指针

    temp = strIn;

    while (*temp == ' ')
    {
        ++temp;
    }

    start = temp; //求得头指针

    temp = strIn + strlen(strIn) - 1; //得到原字符串最后一个字符的指针(不是'\0')

    while (*temp == ' ')
    {
        --temp;
    }

    end = temp; //求得尾指针

    for (strIn = start; strIn <= end;)
    {
        *strOut++ = *strIn++;
    }

    *strOut = '\0';
}

void getValue(char* keyAndValue, const char* key, char* value)
{

    char* p = keyAndValue;

    p = strstr(keyAndValue, key);
    if (p == NULL)
    {
        //printf("没有key\n");
        return;
    }

    p += strlen(key);
    trim(p, value);

    p = strstr(value, "=");
    if (p == NULL)
    {
        printf("没有=\n");
        return;
    }
    p += strlen("=");
    trim(p, value);

    p = strstr(value, "=");
    if (p != NULL)
    {
        printf("多余的=\n");
        return;
    }
    p = value;
    trim(p, value);
}
int writeConfig(const char* filename /*in*/, const char* key /*in*/, const char* value /*in*/)
{

    FILE* pf = NULL;
    char ftemp[flen] = { 0 }, fline[1024] = { 0 }, * fp; //文件缓存数组
    long fsize = 0;
    int reg = 0;
    int exit = 0;
    int i = 0;

    pf = fopen(filename, "r+");
    if (pf == NULL)
    {
        pf = fopen(filename, "w+");
    }
    //获得文件大小
    fseek(pf, 0, SEEK_END); // 将文件指针指向末尾
    fsize = ftell(pf);
    if (fsize > flen)
    {
        printf("文件不能超过8k\n");
        reg = -1;
        goto end;
    }
    fseek(pf, 0, SEEK_SET); //将文件指针指向开头

    //一行一行的读，如果存在key则修改value存到缓存数组中
    while (!feof(pf))
    {
        fgets(fline, 1024, pf);
        if (strstr(fline, key) != NULL && exit == 1)
            strcpy(fline, "");
        if (strstr(fline, key) != NULL && exit == 0)
        { //判断key是否存在
            exit = 1;
            sprintf(fline, "%s = %s\n", key, value);
        }

        printf("fline = %s\n", fline);
        strcat(ftemp, fline);
    }
    if (exit != 1)
    { //如果不存在则把key value写入到最后一行
        sprintf(fline, "%s = %s\n", key, value);
        strcat(ftemp, fline);
    }
    if (pf != NULL)
    {
        fclose(pf);
        pf = fopen(filename, "w+");
        fp = (char*)malloc(sizeof(char) * strlen(ftemp) + 1);
        strcpy(fp, ftemp);
        fp[strlen(fp) - 1] = EOF;
        fputs(fp, pf);
        if (fp != NULL)
        {
            free(fp);
            fp = NULL;
        }
        fclose(pf);
    }
end:
    if (pf != NULL)
        fclose(pf);
    //重新创建一个以filename命名的文件
    return reg;
}

void readConfig(const char* filename /*in*/, const char* key /*in*/, const char** value /*out*/)
{

    FILE* pf = NULL;
    char line[1024] = { 0 }, vtemp[1024] = { 0 };

    pf = fopen(filename, "r"); //以只读方式打开

    while (!feof(pf))
    {
        fgets(line, 1024, pf);
        getValue(line, key, vtemp);
        if (strlen(vtemp) != 0)
            break;
    }
    if (strlen(vtemp) != 0)
    {
        *value = (char*)malloc(sizeof(char) * strlen(vtemp) + 1);
        strcpy((char*)*value, vtemp);
    }
    else
        *value = NULL;
    if (pf != NULL)
        fclose(pf);
}