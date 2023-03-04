#ifndef CHARCONVERT_H
#define CHARCONVERT_H
#include<string>
#include<Windows.h>
class CharConvert {
public:
	static std::string GBKToUTF8(const char* str_GBK);
	static std::string UTF8ToGBK(const char* str_UTF8);
};


#endif