#include<stdio.h>
#include<stdlib.h>
#include<malloc.h>
#include<io.h>
#include<iostream>
#include<algorithm>
#include<string.h>
#include<string>
#include<vector>
#include "rgb2gray.h"//处理bmp头部的头文件
using namespace std;
#define MAXN 10000//最大图片数
#define TRAINSET1 16//选取生成特征脸的训练集子集大小
#define NDIM 10//映射后向量的维度
#define CLASSNUM 20//类的数量
FILE *fpSrcBmpfile;//读入原图片
FILE *fpDestBmpfile;//输出的恢复图片
typedef struct EIGEN
{
	double value;
	double vector[10000];
};//特征向量和特征值
double threshold=100;//欧几里德距离的阈值
int imgs=0;//实际图片数
char name[20][30]={"MouWenlong","WangTianming","LinXingyu","WangWeizi","WangFeng","WeiWan","ShenYeji","ZhiTiancheng","GuoXiujia"};//每一类的名称
BYTE facevector[MAXN][100][100];//存放整个训练集每张图片的初始向量
double facemapv[MAXN][NDIM];//存放每张图片投影后的向量
int faceclass[MAXN];//每张图片的类别
BYTE matrix1[MAXN][10000];//训练特征空间的矩阵
int matrix2[TRAINSET1][10000];//减去平均值后的矩阵 即中心化处理
int cov[TRAINSET1][TRAINSET1];//得到的协方差矩阵
int set1[TRAINSET1];//选取的用来生存特征脸的训练集子集
int ave_face[10000];//平均值
int cla[CLASSNUM][1000];//记录第i类中的图片对应在facemap中的下标
double classcenter[CLASSNUM][NDIM];//每一类的类中心坐标
double eigenvector1[TRAINSET1][TRAINSET1];//matlab计算得到的特征向量
EIGEN eigen[TRAINSET1];//matlab计算得到的特征值
int pimg[100][100];//临时存储待输出图片的矩阵
int histogram[256];//灰度直方图
int src[800][800],dst[100][100];//图片大小放缩变换的源矩阵和目标矩阵
bool ifscan[1500][1500];//记录该像素是否已经被识别
int skinsum1[1500][1500],skinsum[1500][1500],skin[1500][1500];//记录肤色像素的数量
struct testimgs
{
	int w,h;
	BYTE r[1500][1500];
	BYTE g[1500][1500];
	BYTE b[1500][1500];
	BYTE gray[1500][1500];
}testimg;//存储待测试图片

void GetBmpHeader(PBITMAPFILEHEADER,PBITMAPINFOHEADER);
void ChangeBmpHeader(PBITMAPFILEHEADER,PBITMAPINFOHEADER,WORD);
void SetBmpHeader(const PBITMAPFILEHEADER,const PBITMAPINFOHEADER);
void SetRGBQUAD();//处理bmp头部的三个函数
int rgb2gray(const char*,int);//读入bmp图片并灰度化
void getfiles(string,string,vector<string>&);//得到文件夹中的所有文件的路径及名称
void selectimgs();//随机选择  暂不使用
void calc_eigenvector();//计算特征向量
int getclassnum(const char*);//根据文件名得到其类别

bool cmp1(EIGEN,EIGEN);//快排比较函数
void calc_mapvector();//计算投影后的向量
void calc_classcenter();//计算类中心
int print_img(int);//输出图片
void get_aveface();//计算每个人平均脸 暂不使用
void adjust_classcenter();//调整类中心
double dis_facemap_with_center(int,int);//训练集图片与类中心的距离
void get_histogram();//计算灰度直方图
void histogram_equalize();//灰度直方图均衡化

double dis_sldwindow_with_center(double*,int);//滑动窗口中图片与类中心的距离
int read_testimgs();//读入测试图片
void sliding_window(int);//滑动窗口检测识别
bool quick_skincolor_detect(int,int,int);//快速肤色检测
double rgb2ycbcr(int,int,int);//rgb空间转化到ycbcr空间
int bilinear(double,double,int,int,int,int);//双线性插值法
void transfer(int**,int**,int);//图片大小的放缩变换

int main()
{
	unsigned int seed;
	char rootpath[256];
	printf("Input the file path\n");//读入训练集文件夹路径
	gets(rootpath);
	vector<string> files;
	getfiles(rootpath,"bmp",files);//扫描所有bmp文件
	int size=files.size();
	imgs=0;
	for (int i=0;i<size;i++)
	{
		int cnum=getclassnum(files[i].c_str());
		rgb2gray(files[i].c_str(),cnum);//计算得到类标号
	}
	printf("Input a positive integer seed value\n");
	scanf("%d",&seed);
	srand(seed);
	//selectimgs();
	get_aveface();
	calc_eigenvector();//计算特征向量
	
	calc_mapvector();//计算基变换后图片的新坐标向量
	calc_classcenter();//计算类中心坐标
	
	adjust_classcenter();//调整类中心坐标
	
	read_testimgs();
	//sliding_window(130);
	sliding_window(600);//不同大小的窗口扫描
	sliding_window(100);
	sliding_window(200);

	system("pause");
}

int bilinear(double x,double y,int a11,int a12,int a21,int a22)
{
	double deltax=x-int(x);
	double deltay=y-int(y);
	return a11 * (1 - deltax) * (1 - deltay) + a21 * deltax * (1 - deltay) + a12 * (1 - deltax) * deltay + a22 * deltax * deltay;
}

void transfer(int size)//size表示原图大小  目标图大小为100*100
{
	int h=size,w=size;
	for (int i=0;i<100;i++)
	for (int j=0;j<100;j++)
	{
		double x=i*(h-1)*1.0/99;
		double y=j*(w-1)*1.0/99;
		int fx=x;
		int fy=y;
		dst[i][j]=bilinear(x,y,src[fx][fy],src[fx][fy+1],src[fx+1][fy],src[fx+1][fy+1]);
	}
}

double rgb2cr(int r,int g,int b)
{
	double cr=0.5*r-0.418688*g-0.081312*b+128;
	return cr;
}

double rgb2cb(int r,int g,int b)
{
	double cb=-0.168736*r-0.331264*g+0.5*b+128;
	return cb;
}

bool quick_skincolor_detect(int h,int w)//肤色检测
{
	double cr;
	double cb;
	cr=rgb2cr(testimg.r[h][w],testimg.g[h][w],testimg.b[h][w]);
	cb=rgb2cb(testimg.r[h][w],testimg.g[h][w],testimg.b[h][w]);
	if (cr>=130 && cr<=160 )
	if (cb>=87 && cb<=127) return true;
	return false;
}

double dis_with_mapv(double* newcor,int k)//调试用
{
	double s=0;
	for (int i=0;i<NDIM;i++)
	s+=(newcor[i]-facemapv[k][i])*(newcor[i]-facemapv[k][i]);
	return sqrt(s);
}
void sliding_window(int size)//滑动窗口检测识别
{
	memset(ifscan,false,sizeof(ifscan));
	cout<<testimg.h<<" "<<testimg.w<<endl;
	for (int h=0;h<testimg.h;h++)
	for (int w=0;w<testimg.w;w++)
	{
		if (quick_skincolor_detect(h,w)==true) skin[h][w]=1;
		else skin[h][w]=0;
	}
	memset(skinsum1,0,sizeof(skinsum1));
	memset(skinsum,0,sizeof(skinsum));
	for (int h=0;h<testimg.h;h++)
	{
		skinsum1[h][0]=skin[h][0];
		for (int w=1;w<testimg.w;w++)
		skinsum1[h][w]=skinsum1[h][w-1]+skin[h][w];
	}
	for (int w=0;w<testimg.w;w++)
	{
		skinsum[0][w]=skinsum1[0][w];
		for (int h=1;h<testimg.h;h++)
		skinsum[h][w]=skinsum[h-1][w]+skinsum1[h][w];
	}//以上通过前缀和求方阵内的肤色像素数量

	for (int h=1;h+size<testimg.h;h++)
	for (int w=1;w+size<testimg.w;w++)
	if (!ifscan[h][w])
	{
		if (skinsum[h+size-1][w+size-1]-skinsum[h+size-1][w-1]-skinsum[h-1][w+size-1]+skinsum[h-1][w-1]<size*size*0.3)//肤色像素过少则排除
			continue;
		bool check=false;
		for (int x=0;x<size;x++)
		for (int y=0;y<size;y++)
		{
			src[x][y]=testimg.gray[x+h][y+w];
			if (ifscan[x+h][y+w]) {check=true;break;}
		}//避免两个识别窗口重叠
		if (check) continue;

		transfer(size);
		double newcor[NDIM];//存放投影后的坐标
		for (int k=0;k<NDIM;k++)//计算投影后的坐标
		{
			newcor[k]=0;
			for (int x=0;x<100;x++)
			for (int y=0;y<100;y++)
				newcor[k]+=dst[x][y]*eigen[k].vector[x*100+y];
		}
		double delta=0;//存放像素差值和
		double restore[100][100];//存放重建图片
		for (int x=0;x<100;x++)//重建图片
		for (int y=0;y<100;y++)
		{
			restore[x][y]=0;
			for (int k=0;k<NDIM;k++)
			{
				restore[x][y]+=eigen[k].vector[x*100+y]*newcor[k];
			}

			delta+=abs(restore[x][y]-dst[x][y]);
		}
		
	
		if (delta>580000) continue;//如果差值过大则判定不是人脸
		cout<<delta<<endl;
		threshold=4500;
		int ansk;
		for (int k=0;k<8;k++)//判定离哪个类中心最近
		{
			cout<<dis_sldwindow_with_center(newcor,k)<<endl;
		if (dis_sldwindow_with_center(newcor,k)<threshold)
		{
			threshold=dis_sldwindow_with_center(newcor,k);
			ansk=k;
		}
		}
		system("pause");
	
		if (threshold<=4000)//输出识别到的结果
		{
			printf("Find Person %s!\n",name[ansk]);
			printf("coor_x: %d , coor_y: %d \n",h,w);
			cout<<delta<<endl;
			for (int x=h;x<h+size;x++)
			for (int y=w;y<w+size;y++)
				ifscan[x][y]=true;
		}
		

	}
}

double dis_sldwindow_with_center(double* newcor,int k)
{
	double s=0;
	for (int i=0;i<NDIM;i++)
	s+=(newcor[i]-classcenter[k][i])*(newcor[i]-classcenter[k][i]);
	return sqrt(s);
}

int read_testimgs()//读入测试图片
{
	LONG w,h;
	BYTE r,g,b,gray;
	BYTE count24,count8;
	BYTE bmpnul=0;
	printf("Please input the path of img:\n");
	char SrcfilePath[256];
	gets(SrcfilePath);
	BITMAPFILEHEADER bmfh;
	BITMAPINFOHEADER bmih;

	BYTE *data;
	
	memset(&bmfh,0,sizeof(BITMAPFILEHEADER));
	memset(&bmih,0,sizeof(BITMAPINFOHEADER));
	data=(BYTE *)malloc(3*sizeof(BYTE));
	if (!data) 
	{
		printf("malloc error\n");
		return -1;
	}

	printf("handle the file: %s\n",SrcfilePath);
	gets(SrcfilePath);

	if ((fpSrcBmpfile=fopen(SrcfilePath,"rb"))==NULL)
	{
		printf("Path Error\n");
		free(data);
		return -1;
	}
	rewind(fpSrcBmpfile);
	GetBmpHeader(&bmfh,&bmih);

	if (bmfh.bfType!=0x4D42)
	{
	    printf("Not Bmp file!\n");
		free(data);
		return -1;
	}

	if (bmih.biBitCount!=24)
	{
	    printf("Not 24bit Bmp!\n");
		free(data);
		return -1;
	}

	if (bmih.biCompression!=0)
	{
		printf("Not BI_RGB type!\n");
		free(data);
		return -1;
	}
	
	ChangeBmpHeader(&bmfh,&bmih,8);
	SetBmpHeader(&bmfh,&bmih);
	SetRGBQUAD();

	count24=(4-(bmih.biWidth*3)%4)%4;//24位bmp的对齐
	count8=(4-(bmih.biWidth)%4)%4;//8位bmp的对齐

	testimg.h=bmih.biHeight;
	testimg.w=bmih.biWidth;
	for(h=bmih.biHeight-1;h>=0;h--)
	{
		for(w=0;w<bmih.biWidth;w++)
		{
			fread(data,3,1,fpSrcBmpfile);
			if (feof(fpSrcBmpfile))
			{
				printf("Read Pixel data failed\n");
				free(data);
				return -1;
			}
			b=*data;
			g=*(data+1);
			r=*(data+2);
			gray=(299*r+587*g+114*b)/1000;
			testimg.b[h][w]=b;
			testimg.g[h][w]=g;
			testimg.r[h][w]=r;
			testimg.gray[h][w]=gray;
		}
		fseek(fpSrcBmpfile,count24,SEEK_CUR);
	}
	free(data);
	fclose(fpSrcBmpfile);
	cout<<"fuck"<<endl;
}

double dis_facemap_with_center(int k,int cl)
{
	double s=0;
	for (int i=0;i<NDIM;i++)
	s+=(facemapv[k][i]-classcenter[cl][i])*(facemapv[k][i]-classcenter[cl][i]);
	return sqrt(s);
}

void adjust_classcenter()//调整类中心 迭代把距离中心远的向量排除
{
	bool adjust[CLASSNUM];
	bool infacemap[MAXN];
	double tmpcenter[NDIM];
	memset(infacemap,true,sizeof(infacemap));
	memset(adjust,true,sizeof(adjust));
	threshold=3000;
	for (int i=0;i<8;i++)
	{
		int ct;
		while(adjust[i])
		{
			int count=0;
			bool ad=false;
			memset(tmpcenter,0,sizeof(tmpcenter));
			for (int j=1;j<=cla[i][0];j++)
			{
				int k=cla[i][j];
				if (infacemap[k] && dis_facemap_with_center(k,i)>threshold)
				{
					ad=true;;
					infacemap[k]=false;
				}else
				if (infacemap[k])
				{
					count++;
					for (int t=0;t<NDIM;t++)
					tmpcenter[t]+=facemapv[k][t];
				}
			}
			if (ad)
			{
				for (int t=0;t<NDIM;t++)
				classcenter[i][t]=tmpcenter[t]/count;
				ct=count;
				
			}
			else adjust[i]=false;
		}
		cout<<ct<<endl;
	}

	for (int i=0;i<8;i++)
	{
		for (int j=0;j<NDIM;j++)
		{
			cout<<classcenter[i][j]<<" ";
		}
		cout<<endl;
	}
	cout<<endl;
	for (int i=0;i<8;i++)
	{
		cout<<dis_facemap_with_center(cla[1][2],i)<<endl;
	}
}

int print_img(int x)
{
	LONG w,h;
	BYTE r,g,b,gray;
	BYTE count24,count8;
	BYTE bmpnul=0;

	char SrcfilePath[256];
	BITMAPFILEHEADER bmfh;
	BITMAPINFOHEADER bmih;

	BYTE *data;
	SrcfilePath[0]='2';
	SrcfilePath[1]='.';
	SrcfilePath[2]='b';
	SrcfilePath[3]='m';
	SrcfilePath[4]='p';
	SrcfilePath[5]='\0';
	memset(&bmfh,0,sizeof(BITMAPFILEHEADER));
	memset(&bmih,0,sizeof(BITMAPINFOHEADER));
	data=(BYTE *)malloc(3*sizeof(BYTE));
	if (!data) 
	{
		printf("malloc error\n");
		return -1;
	}

	printf("handle the file: %s\n",SrcfilePath);

	if ((fpSrcBmpfile=fopen(SrcfilePath,"rb"))==NULL)
	{
		printf("Path Error\n");
		free(data);
		return -1;
	}

	rewind(fpSrcBmpfile);
	GetBmpHeader(&bmfh,&bmih);

	if (bmfh.bfType!=0x4D42)
	{
	    printf("Not Bmp file!\n");
		free(data);
		return -1;
	}

	if (bmih.biBitCount!=24)
	{
	    printf("Not 24bit Bmp!\n");
		free(data);
		return -1;
	}

	if (bmih.biCompression!=0)
	{
		printf("Not BI_RGB type!\n");
		free(data);
		return -1;
	}

	printf("Input the path of dstfile\n");
	char DestfilePath[256];

	DestfilePath[0]='a'+x;
	DestfilePath[1]='.';
	DestfilePath[2]='b';
	DestfilePath[3]='m';
	DestfilePath[4]='p';
	DestfilePath[5]='\0';
	if ((fpDestBmpfile=fopen(DestfilePath,"wb"))==NULL)
	{
		printf("Path Error!\n");
		free(data);
		return -1;
	}
	
	ChangeBmpHeader(&bmfh,&bmih,8);
	SetBmpHeader(&bmfh,&bmih);
	SetRGBQUAD();

	count24=(4-(bmih.biWidth*3)%4)%4;//24位bmp的对齐
	count8=(4-(bmih.biWidth)%4)%4;//8位bmp的对齐

	
	for(h=bmih.biHeight-1;h>=0;h--)
	{
		for(w=0;w<bmih.biWidth;w++)
		{
			fread(data,3,1,fpSrcBmpfile);
			if (feof(fpSrcBmpfile))
			{
				printf("Read Pixel data failed\n");
				free(data);
				return -1;
			}
			b=*data;
			g=*(data+1);
			r=*(data+2);
			gray=(299*r+587*g+114*b)/1000;
			//system("pause");
			gray=pimg[h][w];
			fwrite(&gray,sizeof(gray),1,fpDestBmpfile);
		}
		fseek(fpSrcBmpfile,count24,SEEK_CUR);
		fwrite(&bmpnul,1,count8,fpDestBmpfile);
	}
	printf("Convert Succeed\n");
	free(data);
	fclose(fpDestBmpfile);
	fclose(fpSrcBmpfile);

}

void get_aveface()
{
	memset(cla,0,sizeof(cla));
	for (int i=0;i<imgs;i++)
	{
		cla[faceclass[i]][0]++;
		cla[faceclass[i]][cla[faceclass[i]][0]]=i;
	}
}

void calc_mapvector()
{
	for (int i=0;i<imgs;i++)
	{
		for (int k=0;k<NDIM;k++)
		{
			facemapv[i][k]=0;
			for (int j=0;j<10000;j++)
				facemapv[i][k]+=eigen[k].vector[j]*matrix1[i][j];
		}
	}
	

	//重建图片并输出 调试用
	int max=0;
	int min=-256;
	for (int d=0;d<8;d++)
	{
	memset(pimg,0,sizeof(pimg));
	for (int i=0;i<100;i++)
	for (int j=0;j<100;j++)
	{
		for (int k=0;k<NDIM;k++)
			pimg[i][j]+=facemapv[cla[d][2]][k]*eigen[k].vector[i*100+j];//随便挑了一张恢复
		if (pimg[i][j]>max) max=pimg[i][j];
		if (pimg[i][j]<min) min=pimg[i][j];
		
	}

	for (int i=0;i<100;i++)
	for (int j=0;j<100;j++)
	{
		pimg[i][j]=(pimg[i][j]-min)*255/(max-min);
	}//归到0-255

	get_histogram();
	histogram_equalize();//灰度直方图均衡化

	int delta=0;
	for (int i=0;i<100;i++)
	for (int j=0;j<100;j++)
	{
		delta+=abs(facevector[cla[d][2]][i][j]-pimg[i][j]);
	}
	cout<<"delta:"<<delta<<endl;

	print_img(d*2);
	for (int i=0;i<100;i++)
	for (int j=0;j<100;j++)
		pimg[i][j]=matrix1[cla[d][2]][i*100+j];//输出原图作对比
	print_img(d*2+1);
	}
	system("pause");
	
}

void calc_classcenter()
{
	memset(classcenter,0,sizeof(classcenter));
	memset(cla,0,sizeof(cla));
	for (int i=0;i<imgs;i++)
	{
		cla[faceclass[i]][0]++;
		cla[faceclass[i]][cla[faceclass[i]][0]]=i;
		for (int j=0;j<NDIM;j++)
			classcenter[faceclass[i]][j]+=facemapv[i][j];
	}
	for (int i=0;i<8;i++)
	{
		for (int j=0;j<NDIM;j++)
		{
			classcenter[i][j]/=cla[i][0];
			cout<<classcenter[i][j]<<" ";
		}
		cout<<endl;
	}
	cout<<endl;
	system("pause");
}

bool cmp1(EIGEN a,EIGEN b)
{
	return a.value>b.value;
}

int getclassnum(const char *filename)
{
	int len=strlen(filename);
	while (filename[len]!='(') len--;
	int num=0;
	len++;
	while (filename[len]!=')')
	{
	   num=num*10+filename[len]-'0';
	   len++;
	}
	return num;
}

void selectimgs()
{
    for (int i=0;i<TRAINSET1;i++)
	set1[i]=abs(rand())%imgs;
}

void calc_eigenvector()//计算特征向量
{
	memset(ave_face,0,sizeof(ave_face));
	for (int i=0;i<8;i++)
	{
		set1[2*i]=cla[i][1];
		
		set1[2*i+1]=cla[i][2];
	}//选取生成特征脸的向量

	for (int i=0;i<TRAINSET1;i++)//将向量合并成矩阵
	{
		int s;
		s=0;
		for (int x=0;x<100;x++)
		for (int y=0;y<100;y++)
		{
			ave_face[s]+=facevector[set1[i]][x][y];
			matrix1[i][s]=facevector[set1[i]][x][y];
			s++;
		}
	}

	for (int i=0;i<10000;i++)
	{
		ave_face[i]/=TRAINSET1;
	}

	for (int i=0;i<TRAINSET1;i++)
	for (int s=0;s<10000;s++)
		matrix2[i][s]=matrix1[i][s]-ave_face[s];//中心化

	system("pause");
	memset(cov,0,sizeof(cov));
	for (int i=0;i<TRAINSET1;i++)
	for (int j=0;j<TRAINSET1;j++)
	{
		for (int k=0;k<10000;k++)
			cov[i][j]+=matrix2[i][k]*matrix2[j][k];
	}//计算协方差矩阵
	

	//利用matlab计算特征值和特征向量
	FILE* MatFile=fopen("matlabin.txt","w");
	for (int i=0;i<TRAINSET1;i++)
	{
		for (int j=0;j<TRAINSET1;j++)
			fprintf(MatFile,"%d ",cov[i][j]);
		fprintf(MatFile,"\n");
	}
	

	FILE* Eigenvt=fopen("eigenvector.txt","r");
	FILE* Eigenva=fopen("eigenvalue.txt","r");
	for (int i=0;i<TRAINSET1;i++)
	{
		for (int j=0;j<TRAINSET1;j++)
		{
			double x;
			fscanf(Eigenva,"%lf",&x);
			if (i==j)
			{
				eigen[i].value=x;
			}
		}
	}	
	for (int i=0;i<TRAINSET1;i++)
	{
		for (int j=0;j<TRAINSET1;j++)
		{
			double x;
			fscanf(Eigenvt,"%lf",&x);
			eigen[j].vector[i]=x;
		}
	}
	sort(eigen,eigen+TRAINSET1,cmp1);

	//得到高维度下的基底
	double tmpvector[10000];
	double len=0;
	for (int i=0;i<NDIM;i++)
	{
		memset(tmpvector,0,sizeof(tmpvector));
		for (int j=0;j<10000;j++)
		for (int k=0;k<TRAINSET1;k++)
			tmpvector[j]+=eigen[i].vector[k]*matrix2[k][j];
		for (int j=0;j<10000;j++)
		{
			len+=tmpvector[j]*tmpvector[j];
		}
		len=sqrt(len);
		for (int j=0;j<10000;j++)
		{
			eigen[i].vector[j]=tmpvector[j]/len;
		}
	}

	for (int i=0;i<imgs;i++)
	{
		for (int x=0;x<100;x++)
		for (int y=0;y<100;y++)
		{
			matrix1[i][x*100+y]=facevector[i][x][y];
		}
	}
}

void getfiles(string path,string extend,vector<string>& files)
{
	long hfile=0;
	struct _finddata_t fileinfo;
	string pathname,exdname;
	
	if (strcmp(extend.c_str(),"")!=0) exdname="\\*."+extend;
	else exdname="\\*";

	if ((hfile=_findfirst(pathname.assign(path).append(exdname).c_str(),&fileinfo))!=-1)
	{
		do
		{
			if ((fileinfo.attrib & _A_SUBDIR))
			{
				if (strcmp(fileinfo.name,".")!=0 && strcmp(fileinfo.name,"..")!=0)
					getfiles(pathname.assign(path).append("\\").append(fileinfo.name),extend,files);
			}else
			{
				if (strcmp(fileinfo.name,".")!=0 && strcmp(fileinfo.name,"..")!=0)
					files.push_back(pathname.assign(path).append("\\").append(fileinfo.name));
			}
		}while(_findnext(hfile,&fileinfo)==0);
		_findclose(hfile);
	}

}

void GetBmpHeader(PBITMAPFILEHEADER pbfheader,PBITMAPINFOHEADER pbiheader)
{
	fread(pbfheader,sizeof(BITMAPFILEHEADER),1,fpSrcBmpfile);
	fread(pbiheader,sizeof(BITMAPINFOHEADER),1,fpSrcBmpfile);
}

void ChangeBmpHeader(PBITMAPFILEHEADER pbfheader,PBITMAPINFOHEADER pbiheader,WORD wType)
{
	pbiheader->biBitCount=wType;

	pbiheader->biClrUsed=(wType==24)?0:256;
	pbfheader->bfOffbits=54+pbiheader->biClrUsed*sizeof(RGBQUAD);
	pbiheader->biSizeImage=((((pbiheader->biWidth*pbiheader->biBitCount)+31)& ~31 )/8)*pbiheader->biHeight;
	pbfheader->bfSize=pbfheader->bfOffbits+pbiheader->biSizeImage;
}

void SetBmpHeader(const PBITMAPFILEHEADER pbfheader,const PBITMAPINFOHEADER pbiheader)
{
	fwrite(pbfheader,sizeof(BITMAPFILEHEADER),1,fpDestBmpfile);
	fwrite(pbiheader,sizeof(BITMAPINFOHEADER),1,fpDestBmpfile);
}

void SetRGBQUAD()
{
	RGBQUAD rgbquad[256];
	for (int i=0;i<256;i++)
	{
		rgbquad[i].rgbBlue=i;
		rgbquad[i].rgbGreen=i;
		rgbquad[i].rgbRed=i;
		rgbquad[i].rgbReserved=0;
	}
	fwrite(rgbquad,256*sizeof(RGBQUAD),1,fpDestBmpfile);
}

void get_histogram()
{
	memset(histogram,0,sizeof(histogram));
	for (int x=0;x<100;x++)
	for (int y=0;y<100;y++)
	{
		histogram[pimg[x][y]]++;
	}
}

void histogram_equalize()
{
	int lut[256];
	memset(lut,0,sizeof(lut));
	int sum=histogram[0];
	for (int i=1;i<256;i++)
	{
		sum+=histogram[i];
		lut[i]=sum*255/10000;
	}
	for (int x=0;x<100;x++)
	for (int y=0;y<100;y++)
	{
		pimg[x][y]=lut[pimg[x][y]];
	}
}

int rgb2gray(const char* SrcfilePath,int cnum)
{
	LONG w,h;
	BYTE r,g,b,gray;
	BYTE count24,count8;
	BYTE bmpnul=0;

	char DestfilePath[256];

	BITMAPFILEHEADER bmfh;
	BITMAPINFOHEADER bmih;

	BYTE *data;
	
	memset(&bmfh,0,sizeof(BITMAPFILEHEADER));
	memset(&bmih,0,sizeof(BITMAPINFOHEADER));
	data=(BYTE *)malloc(3*sizeof(BYTE));
	if (!data) 
	{
		printf("malloc error\n");
		return -1;
	}

	printf("handle the class %d file: %s\n",cnum,SrcfilePath);

	if ((fpSrcBmpfile=fopen(SrcfilePath,"rb"))==NULL)
	{
		printf("Path Error\n");
		free(data);
		return -1;
	}

	rewind(fpSrcBmpfile);
	GetBmpHeader(&bmfh,&bmih);

	if (bmfh.bfType!=0x4D42)
	{
	    printf("Not Bmp file!\n");
		free(data);
		return -1;
	}

	if (bmih.biBitCount!=24)
	{
	    printf("Not 24bit Bmp!\n");
		free(data);
		return -1;
	}

	if (bmih.biCompression!=0)
	{
		printf("Not BI_RGB type!\n");
		free(data);
		return -1;
	}

	DestfilePath[0]='1';
	DestfilePath[1]='.';
	DestfilePath[2]='b';
	DestfilePath[3]='m';
	DestfilePath[4]='p';
	DestfilePath[5]='\0';
	if ((fpDestBmpfile=fopen(DestfilePath,"wb"))==NULL)
	{
		printf("Path Error\n");
		free(data);
		return -1;
	}
	
	ChangeBmpHeader(&bmfh,&bmih,8);
	SetBmpHeader(&bmfh,&bmih);
	SetRGBQUAD();

	count24=(4-(bmih.biWidth*3)%4)%4;//24位bmp的对齐
	count8=(4-(bmih.biWidth)%4)%4;//8位bmp的对齐
	int sum=0;
	
	for(h=bmih.biHeight-1;h>=0;h--)
	{
		for(w=0;w<bmih.biWidth;w++)
		{
			fread(data,3,1,fpSrcBmpfile);
			if (feof(fpSrcBmpfile))
			{
				printf("Read Pixel data failed\n");
				free(data);
				return -1;
			}
			b=*data;
			g=*(data+1);
			r=*(data+2);
			gray=(299*r+587*g+114*b)/1000;
			//system("pause");
			facevector[imgs][h][w]=gray;
			sum+=gray;
			fwrite(&gray,sizeof(gray),1,fpDestBmpfile);
		}
		fseek(fpSrcBmpfile,count24,SEEK_CUR);
		fwrite(&bmpnul,1,count8,fpDestBmpfile);
	}
	/*
	sum/=10000;
	sum-=120;
	for (int i=0;i<100;i++)
	for (int j=0;j<100;j++)
	{
		facevector[imgs][i][j]-=sum;
	}
	cout<<sum<<endl;*/
	faceclass[imgs]=cnum;
	/*
	get_histogram(imgs);
	histogram_equalize(imgs);
	*/
	imgs++;
	printf("Convert Succeed\n");
	free(data);
	fclose(fpDestBmpfile);
	fclose(fpSrcBmpfile);
	return 0;
}