#ifndef __PNGWRITE_H__
#define __PNGWRITE_H__

typedef unsigned char BYTE;
typedef unsigned long ULONG;

enum colorType_e
{
	COLOR_GRAY,
	COLOR_RGB,
	COLOR_RGBA
};

bool WritePNG(int, int , unsigned char* , int , colorType_e , int , const char* );

#endif
