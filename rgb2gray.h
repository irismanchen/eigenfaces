#ifndef BMP_H_INCLUDED
#define BMP_H_INCLUDED

typedef unsigned short WORD;
typedef unsigned long DWORD;
typedef long LONG;
typedef unsigned char BYTE;

#pragma pack(1)
typedef struct tagBITMAPFILEHEADER//14b
{
  WORD bfType;//2b 位图文件类型
  DWORD bfSize;//4b 位图文件大小
  WORD bfReserved1;//2b 位图文件保留字
  WORD bfReserved2;//2b 位图文件保留字
  DWORD bfOffbits;//4b 位图数据起始地址偏移
}BITMAPFILEHEADER,*PBITMAPFILEHEADER;
#pragma pack()

#pragma pack(1)
typedef struct tagBITMAPINFOHEADER//40b
{
  DWORD biSize;//4b 本结构体大小
  LONG biWidth;//4b 位图的宽度
  LONG biHeight;//4b 位图的高度
  WORD biPlanes;//2b 目标设备的说明位面数 总为1
  WORD biBitCount;//2b 每个像素所需的位数 1-双色 4-16色 8-256色 24-真彩色
  DWORD biCompression;//4b 位图的压缩类型 总为0-不压缩 1-BI_RLE8压缩 2-BI_RLE4压缩
  DWORD biSizeImage;//4b 位图大小
  LONG biXPelsPerMeter;//4b 位图水平分辨率
  LONG biYPelsPerMeter;//4b 位图垂直分辨率
  DWORD biClrUsed;//4b 位图实际使用的颜色表中的颜色数
  DWORD biClrImportant;//4b 位图显示过程中重要的颜色数
}BITMAPINFOHEADER,*PBITMAPINFOHEADER;
#pragma pack()

#pragma pack(1)
typedef struct tagRGBQUAD
{
	BYTE rgbBlue;//1b 蓝色亮度
	BYTE rgbGreen;//1b 绿色亮度
	BYTE rgbRed;//1b 红色亮度
	BYTE rgbReserved;//1b 保留 总为0
}RGBQUAD;// biBitCount=1,4,8时 分别有2,16,256个表项;=24时，没有颜色表项
#pragma pack()

#pragma pack(1)
typedef struct tagBITMAPIMAGE
{
  BITMAPFILEHEADER bmiHeader;// 位图信息头
  RGBQUAD bmiColors[1];// 颜色表
}BITMAPIMAGE;
#pragma pack()

#endif