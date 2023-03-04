#include"charconvert.h"


std::string CharConvert::GBKToUTF8(const char* str_GBK)
{
	int len = MultiByteToWideChar(CP_ACP, 0, str_GBK, -1, NULL, 0);
	wchar_t* wstr = new wchar_t[len + 1];
	memset(wstr, 0, len + 1);
	MultiByteToWideChar(CP_ACP, 0, str_GBK, -1, wstr, len);
	len = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, NULL, 0, NULL, NULL);
	char* str = new char[len + 1];
	memset(str, 0, len + 1);
	WideCharToMultiByte(CP_UTF8, 0, wstr, -1, str, len, NULL, NULL);
	std::string strTemp = str;
	if (wstr) delete[] wstr;
	if (str) delete[] str;
	return strTemp;
}
std::string CharConvert::UTF8ToGBK(const char* str_UTF8)
{
	int len = MultiByteToWideChar(CP_UTF8, 0, str_UTF8, -1, NULL, 0);
	wchar_t* wsz_GBK = new wchar_t[len + 1];
	memset(wsz_GBK, 0, len * 2 + 2);
	MultiByteToWideChar(CP_UTF8, 0, str_UTF8, -1, wsz_GBK, len);
	len = WideCharToMultiByte(CP_ACP, 0, wsz_GBK, -1, NULL, 0, NULL, NULL);
	char* sz_GBK = new char[len + 1];
	memset(sz_GBK, 0, len + 1);
	WideCharToMultiByte(CP_ACP, 0, wsz_GBK, -1, sz_GBK, len, NULL, NULL);
	std::string str_temp(sz_GBK);
	if (wsz_GBK) delete[] wsz_GBK;
	if (sz_GBK) delete[] sz_GBK;
	return str_temp;
}