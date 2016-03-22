#ifndef BMP_H_INCLUDED
#define BMP_H_INCLUDED

typedef unsigned short WORD;
typedef unsigned long DWORD;
typedef long LONG;
typedef unsigned char BYTE;

#pragma pack(1)
typedef struct tagBITMAPFILEHEADER//14b
{
  WORD bfType;//2b λͼ�ļ�����
  DWORD bfSize;//4b λͼ�ļ���С
  WORD bfReserved1;//2b λͼ�ļ�������
  WORD bfReserved2;//2b λͼ�ļ�������
  DWORD bfOffbits;//4b λͼ������ʼ��ַƫ��
}BITMAPFILEHEADER,*PBITMAPFILEHEADER;
#pragma pack()

#pragma pack(1)
typedef struct tagBITMAPINFOHEADER//40b
{
  DWORD biSize;//4b ���ṹ���С
  LONG biWidth;//4b λͼ�Ŀ��
  LONG biHeight;//4b λͼ�ĸ߶�
  WORD biPlanes;//2b Ŀ���豸��˵��λ���� ��Ϊ1
  WORD biBitCount;//2b ÿ�����������λ�� 1-˫ɫ 4-16ɫ 8-256ɫ 24-���ɫ
  DWORD biCompression;//4b λͼ��ѹ������ ��Ϊ0-��ѹ�� 1-BI_RLE8ѹ�� 2-BI_RLE4ѹ��
  DWORD biSizeImage;//4b λͼ��С
  LONG biXPelsPerMeter;//4b λͼˮƽ�ֱ���
  LONG biYPelsPerMeter;//4b λͼ��ֱ�ֱ���
  DWORD biClrUsed;//4b λͼʵ��ʹ�õ���ɫ���е���ɫ��
  DWORD biClrImportant;//4b λͼ��ʾ��������Ҫ����ɫ��
}BITMAPINFOHEADER,*PBITMAPINFOHEADER;
#pragma pack()

#pragma pack(1)
typedef struct tagRGBQUAD
{
	BYTE rgbBlue;//1b ��ɫ����
	BYTE rgbGreen;//1b ��ɫ����
	BYTE rgbRed;//1b ��ɫ����
	BYTE rgbReserved;//1b ���� ��Ϊ0
}RGBQUAD;// biBitCount=1,4,8ʱ �ֱ���2,16,256������;=24ʱ��û����ɫ����
#pragma pack()

#pragma pack(1)
typedef struct tagBITMAPIMAGE
{
  BITMAPFILEHEADER bmiHeader;// λͼ��Ϣͷ
  RGBQUAD bmiColors[1];// ��ɫ��
}BITMAPIMAGE;
#pragma pack()

#endif