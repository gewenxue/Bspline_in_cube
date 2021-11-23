#include "StdAfx.h"
#include <windows.h>
#include "stdio.h"
#include "glew.h"
#include "glaux.h"
#include <string>
#include <cstdlib>
#include <glui.h>
#include <cmath>
#include <sstream>
#include <fstream>
#include <cstdio>
#include<iostream>
#include <glut.h>
#include <time.h>

#define GL_SILENCE_DEPRECATION  
#define CRTL_LOAD			0x00	//打开文件控件
#define CRTL_MOUSE			0x01	//鼠标绘制控件
#define CRTL_CLEAR			0x02	//清屏控件
#define CRTL_SAVE			0x03    //保存为bmp控件
#define CRTL_SHOW			0x04	//展示立方体

#define DRAW_BY_FILE		0x00	//打开文件绘制
#define DRAW_BY_MOUSE		0x01	//鼠标控制绘制
#define DRAW_NONE			0x02	//不绘制
#define PI 3.1415926535897

using namespace std;


HGLRC hRC=NULL; //窗口着色描述表句柄
HDC hDC=NULL;//Opengl渲染描述表句柄
HWND hWnd=NULL;//保存我们的窗口句柄
HINSTANCE hInstance;//保存程序的实例
LRESULT CALLBACK WndProc(HWND,UINT,WPARAM,LPARAM);//WndProc的定义，为了creategl window（）引用

bool mouseRightIsDown = false;		//右键是否按下
int DrawPolygons = 0;				//是否绘制封闭多边形
int density;						//采样点数量
int window_x, window_y;				//窗口坐标
int window_name;					//窗口名字
int draw_state = DRAW_NONE;			//绘制状态
int num=0;
bool active=TRUE;
bool fullscreen=TRUE;
bool keys[256];

GLfloat rquad;
GLfloat xrot;
GLfloat yrot;
GLfloat zrot;
GLuint texture[6];//存储6个纹理
bool mouseDown = false;
float xdiff = 0.0f;
float ydiff = 0.0f;

//点
typedef struct Point {
	double x, y;
	Point() {}
	Point(double x, double y) :x(x), y(y) {}
};

//对象
class Obj {
public:
	int degree;				//B样条曲线的次数
	int cnt_num;			//控制点的数量
	vector<double> u;		//节点矢量
	vector<Point> points;	//控制点集合

	//读取txt文件
	void ReadFile(string filename);

	Obj() {
		degree = 0;
		cnt_num = 0;
		u.clear();
		points.clear();
	}
};
Obj obj;

//读取txt文件
void Obj::ReadFile(string filename) {
	fstream fpFile;
	fpFile.open(filename, ios::in); //打开文件
	if (!fpFile.is_open())	//打开失败返回
		return;

	points.clear();	//清空
	u.clear();

	string line;

	//读入次数
	line = "";
	getline(fpFile, line);
	degree = atoi(line.c_str());	//string转int
	//读入控制点数量
	line = "";
	getline(fpFile, line);
	cnt_num = atoi(line.c_str());
	//读入节点矢量
	line = "";
	getline(fpFile, line);
	string str = "";
	line += " ";
	for (unsigned i = 0; i < line.length(); i++) {
		if (line[i] != ' ')
			str += line[i];
		else {
			double ui;
			istringstream iss(str);		//string转double
			iss >> ui;
			u.push_back(ui);
			str = "";
		}
	}
	//读入控制点坐标
	for (int i = 0; i < cnt_num; i++) {
		line = "";
		string str = "";
		vector<string> p;
		getline(fpFile, line);
		line += " ";
		for (unsigned i = 0; i < line.length(); i++) {
			if (line[i] != ' ')
				str += line[i];
			else {
				p.push_back(str);
				str = "";
			}
		}
		Point point;
		istringstream iss_x(p[0]);
		istringstream iss_y(p[1]);
		iss_x >> point.x;
		iss_y >> point.y;
		points.push_back(point);
	}

	fpFile.close();		//关闭文件
}

//加载模型
void loadFile() {
	//调用系统对话框
	OPENFILENAME  fname;
	ZeroMemory(&fname, sizeof(fname));
	char strfile[200] = "*.txt";
	char szFilter[] = TEXT("TXT Files(*.txt)\0");
	fname.lStructSize = sizeof(OPENFILENAME);
	fname.hwndOwner = NULL;
	fname.hInstance = NULL;
	fname.lpstrFilter = szFilter;
	fname.lpstrCustomFilter = NULL;
	fname.nFilterIndex = 0;
	fname.nMaxCustFilter = 0;
	fname.lpstrFile = strfile;
	fname.nMaxFile = 200;
	fname.lpstrFileTitle = NULL;
	fname.nMaxFileTitle = 0;
	fname.lpstrTitle = NULL;
	fname.Flags = OFN_HIDEREADONLY | OFN_CREATEPROMPT;
	fname.nFileOffset = 0;
	fname.nFileExtension = 0;
	fname.lpstrDefExt = 0;
	fname.lCustData = NULL;
	fname.lpfnHook = NULL;
	fname.lpTemplateName = NULL;
	fname.lpstrInitialDir = NULL;
	HDC hDC = wglGetCurrentDC();
	HGLRC hRC = wglGetCurrentContext();
	GetOpenFileName(&fname);
	wglMakeCurrent(hDC, hRC);

	obj.ReadFile(fname.lpstrFile); //读入模型文件
}

//矩阵相乘
double getRatio(double t, double a, double b, double c, double d) {
	return a * pow(t, 3) + b * pow(t, 2) + c * t + d;
}

//两点间距离
double caculateSquarDistance(Point a, Point b) {
	return (a.x - b.x)*(a.x - b.x) + (a.y - b.y)*(a.y - b.y);
}

//鼠标点击距离函数
int getIndexNearByMouse(double x, double y) {
	double precision = 200;     //精确度
	int index = -1;
	double Min;

	for (int i = 0; i < obj.cnt_num; i++) {
		double dis = caculateSquarDistance(obj.points[i], Point(x, y));

		if (dis < precision) {
			if (index == -1) {
				index = i;
				Min = dis;
			}
			else if (dis < Min) {
				index = i;
				Min = dis;
			}
		}
	}
	return index;
}

Point compute_point(double a) {
	int m = obj.cnt_num + obj.degree;	//行
	int n = obj.degree + 1;		//列
	vector<double> u = obj.u;
	vector<vector<double> > B_spline(m, vector<double>(n, 0));

	// 将 0 - 1 映射到对应取值范围上
	double umin = u[obj.degree];
	double umax = u[obj.cnt_num];
	a = a * (umax - umin) + umin;

	// 计算每一列
	for (int j = 0; j < n; j++) {
		// 计算每一行
		for (int i = 0; i < m; i++) {
			if (i + j > m - 1)
				break;

			if (j == 0) 
				B_spline[i][j] = (a >= u[i] && a < u[i + 1]) ? 1 : 0;

			else {
				double A = (a - u[i]) * B_spline[i][j - 1];
				double B = u[i + j] - u[i];
				double C = (u[i + j + 1] - a) * B_spline[i + 1][j - 1];
				double D = u[i + j + 1] - u[i + 1];

				if (A == 0 && B == 0 && C != 0 && D != 0)
					B_spline[i][j] = C / D;
				else if (A != 0 && B != 0 && C == 0 && D == 0)
					B_spline[i][j] = A / B;
				else if (A == 0 && B == 0 && C == 0 && D == 0)
					B_spline[i][j] = 0;
				else
					B_spline[i][j] = A / B + C / D;

			}
		}
	}

	Point p(0, 0);

	for (int i = 0; i < obj.cnt_num; i++) {
		p.x += obj.points[i].x * B_spline[i][obj.degree];
		p.y += obj.points[i].y * B_spline[i][obj.degree];
	}

	return p;
}


//三次均匀b样条曲线函数
void Bezier(Point a, Point b, Point c, Point d) {
	int n = density;
	double derta = 1.0 / n;	//采样点密度
	glPointSize(2);
	glColor3d(0, 0, 0);
	glBegin(GL_LINE_STRIP);
	for (int i = 0; i <= n; i++) {
		double t = derta * i;
		double ratio[4];
		ratio[0] = getRatio(t, -1, 3, -3, 1);
		ratio[1] = getRatio(t, 3, -6, 0, 4);
		ratio[2] = getRatio(t, -3, 3, 3, 1);
		ratio[3] = getRatio(t, 1, 0, 0, 0);
		double x = 0, y = 0;
		x += ratio[0] * a.x + ratio[1] * b.x + ratio[2] * c.x + ratio[3] * d.x;
		y += ratio[0] * a.y + ratio[1] * b.y + ratio[2] * c.y + ratio[3] * d.y;
		x /= 6.0;
		y /= 6.0;
		glVertex2d(x, y);
	}
	glEnd();
}


//绘图函数
void myGlutDisplay() {
	if (draw_state == DRAW_NONE)	//状态为不绘制时，直接返回
		return;

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);    //清除颜色缓存和深度缓存

	//画点
	glPointSize(3.0);
	glColor3d(1.0, 0.0, 0.0);
	glBegin(GL_POINTS);
	for (int i = 0; i < obj.cnt_num; i++)
		glVertex2d(obj.points[i].x, obj.points[i].y);
	glEnd();

	//画直线
	glLineWidth(2);
	glColor3d(1.0, 0.0, 0.0);
	if (DrawPolygons == 0)		//判断是否绘制封闭多边形
		glBegin(GL_LINE_STRIP);
	else if (DrawPolygons == 1)
		glBegin(GL_LINE_LOOP);
	
	for (int i = 0; i < obj.cnt_num; i++)
		glVertex2d(obj.points[i].x, obj.points[i].y);
	
	glEnd();
	
	//画曲线
	glLineWidth(2);
	glPointSize(2);
	glColor3d(0, 0, 0);

	//鼠标点击绘制
	if (draw_state == DRAW_BY_MOUSE) {
		if (obj.cnt_num >= 4)
		for (int i = 0; i < obj.cnt_num - 3; i++) {
			Bezier(obj.points[i], obj.points[i + 1], obj.points[i + 2], obj.points[i + 3]);
		}
	}

	//打开文件绘制
	else if (draw_state == DRAW_BY_FILE) {
		if (obj.cnt_num >= 4) {
			glBegin(GL_LINE_STRIP);
			for (double i = 0; i <= 1; i += (1.0 / density)) {
				Point p = compute_point(i);
				glVertex2d(p.x, p.y);
			}
			glEnd();
		}
	}
	glFlush();
}

//键盘输入函数
void myGlutKeyboard(unsigned char key, int x, int y) {
	//ESC
	if (key == 27)
		exit(0);

	//退格键或删除键
	if (key == 8 || key == 127) {
		int index = getIndexNearByMouse(x - window_x / 2, window_y / 2 - y);
		if (index == -1)
			return;
		obj.points.erase(obj.points.begin() + index);
		obj.cnt_num--;
		glutPostRedisplay();
	}
}

//鼠标点击函数
void myGlutMouse(int button, int state, int x, int y) {

	//左键放置
	if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {	
		if (draw_state == DRAW_BY_FILE || draw_state == DRAW_NONE)	//只有当处于鼠标点击绘制状态下才能增加结点
			return;
		Point p(x - window_x / 2, window_y / 2 - y);
		obj.points.push_back(p);
		obj.cnt_num++;
		glutPostRedisplay();
	}

	//右键移动
	if (button == GLUT_RIGHT_BUTTON && state == GLUT_DOWN)
		mouseRightIsDown = true;
	if (button == GLUT_RIGHT_BUTTON && state == GLUT_UP)
		mouseRightIsDown = false;
}

//鼠标移动函数
void myGlutMotion(int x, int y) {

	//按住右键移动点
	if (mouseRightIsDown) {
		int index = getIndexNearByMouse(x - window_x / 2, window_y / 2 - y);
		if (index == -1)
			return;
		obj.points[index].x = x - window_x / 2;
		obj.points[index].y = window_y / 2 - y;
		glutPostRedisplay();
	}
}

//画面大小重构函数
void myGlutReshape(int w, int h) {
	window_x = w;
	window_y = h;
	glViewport(0, 0, w, h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(-w / 2, w / 2, -h / 2, h / 2);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

//空闲回调函数
void myGlutIdle() {
	if (glutGetWindow() != window_name)
		glutSetWindow(window_name);

	glutPostRedisplay();
}

//清空屏幕
void gluiClear() {
	glClearColor(1.0, 1.0, 1.0, 0.0);//设置清除颜色
	glClear(GL_COLOR_BUFFER_BIT);//把窗口清除为当前颜色
	glClearDepth(1.0);//指定深度缓冲区中每个像素需要的值
	glClear(GL_DEPTH_BUFFER_BIT);//清除深度缓冲区

	Obj newobj;
	obj = newobj;

	glutPostRedisplay();
}

//WriteBitmapFile
//根据bitmapData的（RGB）数据，保存bitmap
//filename是要保存到物理硬盘的文件名（包括路径）
BOOL WriteBitmapFile(char * filename,int width,int height,unsigned char * bitmapData)
{
 //填充BITMAPFILEHEADER
 BITMAPFILEHEADER bitmapFileHeader;
 memset(&bitmapFileHeader,0,sizeof(BITMAPFILEHEADER));
 bitmapFileHeader.bfSize = sizeof(BITMAPFILEHEADER);
 bitmapFileHeader.bfType = 0x4d42; //BM
 bitmapFileHeader.bfOffBits =sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
 
 //填充BITMAPINFOHEADER
 BITMAPINFOHEADER bitmapInfoHeader;
 memset(&bitmapInfoHeader,0,sizeof(BITMAPINFOHEADER));
 bitmapInfoHeader.biSize = sizeof(BITMAPINFOHEADER);
 bitmapInfoHeader.biWidth = width;
 bitmapInfoHeader.biHeight = height;
 bitmapInfoHeader.biPlanes = 1;
 bitmapInfoHeader.biBitCount = 24;
 bitmapInfoHeader.biCompression = BI_RGB;
 bitmapInfoHeader.biSizeImage = width * abs(height) * 3;
 
 //
 FILE * filePtr;   //连接要保存的bitmap文件用
 unsigned char tempRGB; //临时色素
 int imageIdx;
 
 //交换R、B的像素位置,bitmap的文件放置的是BGR,内存的是RGB
 for (imageIdx = 0;imageIdx < bitmapInfoHeader.biSizeImage;imageIdx +=3)
 {
  tempRGB = bitmapData[imageIdx];
  bitmapData[imageIdx] = bitmapData[imageIdx + 2];
  bitmapData[imageIdx + 2] = tempRGB;
 }
 
 filePtr = fopen(filename,"wb");
 if (NULL == filePtr)
 {
  return FALSE;
 }
 
 fwrite(&bitmapFileHeader,sizeof(BITMAPFILEHEADER),1,filePtr);
 
 fwrite(&bitmapInfoHeader,sizeof(BITMAPINFOHEADER),1,filePtr);
 
 fwrite(bitmapData,bitmapInfoHeader.biSizeImage,1,filePtr);
 
 fclose(filePtr);
 return TRUE;
}

//SaveScreenShot
//保存窗口客户端的截图
//窗口大小* 600
void SaveScreenShot()
{
 int clnWidth,clnHeight; //client width and height
 static void * screenData;
 RECT rc;
 int len = 800 * 600 * 3;
 screenData = malloc(len);
 memset(screenData,0,len);
 glReadPixels(0, 0, 800, 600, GL_RGB, GL_UNSIGNED_BYTE, screenData);

 char lpstrFilename[256] = {0};
 sprintf_s(lpstrFilename,sizeof(lpstrFilename),"./Data/pic%d.bmp",num);
 num++;
 
 WriteBitmapFile(lpstrFilename,800,600,(unsigned char*)screenData);
 
 free(screenData);
 
}


AUX_RGBImageRec *LoadBMP(char* Filename)
{
	FILE* File=NULL;
	if(!Filename)
	{
		return NULL;
	}
	File=fopen(Filename,"r");
	if(File)
	{
		fclose(File);
		return auxDIBImageLoad(Filename);
	}
	return NULL;
}

//载入位图并转换纹理
int LoadGLTextures()
{
	int Status=FALSE;
	AUX_RGBImageRec* TextureImage[6];/////NUMBER
	memset(TextureImage,0,sizeof(void*)*6);
	//if(TextureImage[0]=LoadBMP("./testxuanzhuan/Data/pic0.bmp"))
	if((TextureImage[0]=LoadBMP("./Data/pic0.bmp"))&&(TextureImage[1]=LoadBMP("./Data/pic1.bmp"))&&(TextureImage[2]=LoadBMP("./Data/pic2.bmp"))&&(TextureImage[3]=LoadBMP("./Data/pic3.bmp"))&&(TextureImage[4]=LoadBMP("./Data/pic4.bmp"))&&(TextureImage[5]=LoadBMP("./Data/pic5.bmp")))
	{
		Status=TRUE;
		glGenTextures(6,&texture[0]);
		// 使用来自位图数据生成 的典型纹理
		for(int i=0;i<6;i++)
		{
			glBindTexture(GL_TEXTURE_2D,texture[i]);
			glTexImage2D(GL_TEXTURE_2D, 0, 3, TextureImage[i]->sizeX, TextureImage[i]->sizeY, 0, 
	GL_RGB, GL_UNSIGNED_BYTE, TextureImage[i]->data);
			glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR); // 线形滤波
			glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR); // 线形滤波
		}
	}
	for(int i=0;i<6;i++)
	{
		if (TextureImage[i])        // 纹理是否存在
		{
			if (TextureImage[i]->data)      // 纹理图像是否存在
			{
				free(TextureImage[i]->data);     // 释放纹理图像占用的内存
			}

			free(TextureImage[i]);       // 释放图像结构
  
		 }
	}
	return Status; // 返回 Status

}
//add
GLvoid ReSizeGLSence(GLsizei width,GLsizei height)//重置opengl窗口大小
{
    if(height==0)
    {
        height=1;
    }
    glViewport(0,0,width,height);
	glMatrixMode(GL_PROJECTION);//选择投影矩阵
	glLoadIdentity();//重置投影矩阵
    gluPerspective(45.0f,(GLfloat)width/(GLfloat)height,0.1f,100.0f);//设置视口大小add........................
    glMatrixMode(GL_MODELVIEW);//选择模型观察矩阵add
    glLoadIdentity();//重置模型观察矩阵add
}


//add
GLvoid KillGLWindow(GLvoid)//正常销毁窗口
{
    if(fullscreen){
        ChangeDisplaySettings(NULL,0);
        ShowCursor(TRUE);
    }
    if(hRC){
        if(!wglMakeCurrent(NULL,NULL))
        {
            MessageBox(NULL,"释放DC或RC失败。","关闭错误",MB_OK|MB_ICONINFORMATION);
        }
        if(!wglDeleteContext(hRC))
        {
            MessageBox(NULL,"释放RC失败.","关闭错误",MB_OK|MB_ICONINFORMATION);
        }
        hRC=NULL;
    }
    if(hDC&&!ReleaseDC(hWnd,hDC))
    {
        MessageBox(NULL,"释放DC失败。","关闭错误",MB_OK|MB_ICONINFORMATION);
        hDC=NULL;
    }
    if(hWnd&&!DestroyWindow(hWnd))
    {
        MessageBox(NULL,"释放窗口句柄失败。","关闭错误",MB_OK|MB_ICONINFORMATION);
        hWnd=NULL;
    }
    if(!UnregisterClass("OpenG",hInstance))
    {
        MessageBox(NULL,"不能注销窗口类。","关闭错误",MB_OK|MB_ICONINFORMATION);
        hInstance=NULL;
    }
}

//≥? ????≠≤?
int InitGL(GLvoid) {
	
	if (!LoadGLTextures()) // 调用纹理载入子例程
	{
		std::cout << "!LoadGLTextures"  << std::endl;
		return FALSE; // 如果未能载入，返回FALSE
	}
	glEnable(GL_TEXTURE_2D); // 启用纹理映射
    glShadeModel(GL_SMOOTH);//启用阴影平滑add
    glClearColor(1, 1, 1, 1);//白色背景
	glClear(GL_COLOR_BUFFER_BIT);
    glClearDepth(1.0f);//设置深度缓存add
    glEnable(GL_DEPTH_TEST);//启用深度测试add
    glDepthFunc(GL_LEQUAL);//所作深度测试类型add
    glHint(GL_PERSPECTIVE_CORRECTION_HINT,GL_NICEST);//告诉系统对透视进行修正add
	return TRUE;
}

//add创建窗口
BOOL CreateGLWindow(char* title,int width,int height,int bits,bool fullscreenflag)
{
    GLuint PixelFormat;
    WNDCLASS wc;
    DWORD dwExStyle;
    DWORD dwStyle;
    RECT WindowRect;
    WindowRect.left=(long)0;
    WindowRect.right=(long)width;
    WindowRect.top=(long)0;
    WindowRect.bottom=(long)height;
    fullscreen=fullscreenflag;
    hInstance=GetModuleHandle(NULL);
    wc.style=CS_HREDRAW|CS_VREDRAW|CS_OWNDC;
    wc.lpfnWndProc=(WNDPROC)WndProc;
    wc.cbClsExtra=0;
    wc.cbWndExtra=0;
    wc.hInstance=hInstance;
    wc.hIcon=LoadIcon(NULL,IDI_WINLOGO);
    wc.hCursor=LoadCursor(NULL,IDC_ARROW);
    wc.hbrBackground=NULL;
    wc.lpszMenuName=NULL;
    wc.lpszClassName="OpenG";
    if(!RegisterClass(&wc))
    {
        MessageBox(NULL,"注册窗口失败","错误",MB_OK|MB_ICONEXCLAMATION);
        return FALSE;
    }
    if(fullscreen)
    {
        DEVMODE dmScreenSettings;
        memset(&dmScreenSettings,0,sizeof(dmScreenSettings));
        dmScreenSettings.dmSize=sizeof(dmScreenSettings);
        dmScreenSettings.dmPelsWidth=width;
        dmScreenSettings.dmPelsHeight=height;
        dmScreenSettings.dmBitsPerPel=bits;
        dmScreenSettings.dmFields=DM_BITSPERPEL|DM_PELSWIDTH|DM_PELSHEIGHT;
        if(ChangeDisplaySettings(&dmScreenSettings,CDS_FULLSCREEN)!=DISP_CHANGE_SUCCESSFUL)
        {
            if(MessageBox(NULL,"全屏模式在当前显卡上设置失败！\n使用窗口模式？","NeHeG",MB_YESNO|MB_ICONEXCLAMATION)==IDYES)
            {
                fullscreen=FALSE;
            }
            else
            {
                MessageBox(NULL,"程序将被关闭","错误",MB_OK|MB_ICONSTOP);
                return FALSE;
            }
        }
    }
    if(fullscreen)
    {
        dwExStyle=WS_EX_APPWINDOW;
        dwStyle=WS_POPUP;
        ShowCursor(FALSE);
    }
    else
    {
        dwExStyle=WS_EX_APPWINDOW|WS_EX_WINDOWEDGE;
        dwStyle=WS_OVERLAPPEDWINDOW;
    }
    AdjustWindowRectEx(&WindowRect,dwStyle,FALSE,dwExStyle);
    if(!(hWnd=CreateWindowEx(dwExStyle,"OPenG",title,WS_CLIPSIBLINGS|WS_CLIPCHILDREN|dwStyle,0,0,WindowRect.right-WindowRect.left,WindowRect.bottom-WindowRect.top,NULL,NULL,hInstance,NULL)))
    {
        KillGLWindow();
        MessageBox(NULL,"不能创建一个窗口设备描述表","错误",MB_OK|MB_ICONEXCLAMATION);
        return FALSE;
    }
    // add描述像素格式
    static PIXELFORMATDESCRIPTOR pfd=
    {
        sizeof(PIXELFORMATDESCRIPTOR),
        1,
        PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL|PFD_DOUBLEBUFFER,
        PFD_TYPE_RGBA,
        bits,
        0,0,0,0,0,0,
        0,
        0,
        0,
        0,0,0,0,
        16,
        0,
        0,
        PFD_MAIN_PLANE,
        0,
        0,0,0
    };
    if(!(hDC=GetDC(hWnd)))
    {
        KillGLWindow();
        MessageBox(NULL,"不能创建一种相匹配的像素格式","错误",MB_OK|MB_ICONEXCLAMATION);
        return FALSE;
    }
    if(!(PixelFormat=ChoosePixelFormat(hDC,&pfd)))
    {
        KillGLWindow();
        MessageBox(NULL,"不能设置像素格式","错误",MB_OK|MB_ICONEXCLAMATION);
        return FALSE;
    }
    if(!SetPixelFormat(hDC,PixelFormat,&pfd))
    {
        KillGLWindow();
        MessageBox(NULL,"不能设置像素格式","错误",MB_OK|MB_ICONEXCLAMATION);
        return FALSE;
    }
    if(!(hRC=wglCreateContext(hDC)))
    {
        KillGLWindow();
        MessageBox(NULL,"不能创建OpenGL渲染描述表","错误",MB_OK|MB_ICONEXCLAMATION);
        return FALSE;
    }
    if(!wglMakeCurrent(hDC,hRC))
    {
        KillGLWindow();
        MessageBox(NULL,"不能激活当前的OpenGL渲染描述表","错误",MB_OK|MB_ICONEXCLAMATION);
        return FALSE;
    }
    ShowWindow(hWnd,SW_SHOW);
    SetForegroundWindow(hWnd);
    SetFocus(hWnd);
    ReSizeGLSence(width,height);
    if(!InitGL()){
        KillGLWindow();
        MessageBox(NULL,"Initialization Field.","ERROR",MB_OK|MB_ICONEXCLAMATION);
        return FALSE;
    }
    return TRUE;
}

//add 处理窗口消息
LRESULT CALLBACK WndProc(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
    switch (uMsg) {
        case WM_ACTIVATE:
        {
            if(!HIWORD(wParam))
            {
                active=TRUE;
            }
            else
            {
                active=FALSE;
            }
            return 0;
        }
        case WM_SYSCOMMAND:
        {
            switch (wParam) {
                case SC_SCREENSAVE:
                case SC_MONITORPOWER:
                return 0;
            }
            break;
        }
        case WM_CLOSE:
        {
            PostQuitMessage(0);
            return 0;
        }
        case WM_KEYDOWN:
        {
            keys[wParam]=TRUE;
            return 0;
        }
        case WM_KEYUP:
        {
            keys[wParam]=FALSE;
            return 0;
        }
        case WM_SIZE:
        {
            ReSizeGLSence(LOWORD(lParam),HIWORD(lParam));
            return 0;
        }
    }
    return DefWindowProc(hWnd,uMsg,wParam,lParam);
}

void mouse(int button, int state, int x, int y) {
	if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
		mouseDown = true;
		xdiff = x - yrot;
		ydiff = -y + xrot;
		std::cout << "xdiff:" << xdiff << "\tydiff" << ydiff << std::endl;
	}
	else
		mouseDown = false;
}
// 鼠标移动事件
void mouseMotion(int x, int y) {
	if (mouseDown) {
		yrot = x - xdiff;
		xrot = y + ydiff;
		std::cout << "yrot:" << yrot << "\txrot" << xrot << std::endl;

		glutPostRedisplay();
	}
}
//绘制
void  DrawGLScene(GLvoid)
{
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);//清除屏幕和深度缓存
    glLoadIdentity();//重置当前的模型观察矩阵
	glTranslatef(0.0f,0.0f,-5.0f);
	glRotatef(xrot,1.0f,0.0f,0.0f); // 绕X轴旋转
	glRotatef(yrot,0.0f,1.0f,0.0f); // 绕Y轴旋转
	//glRotatef(zrot,0.0f,0.0f,1.0f); // 绕Z轴旋转

	glBindTexture(GL_TEXTURE_2D,texture[0]);
    glBegin(GL_QUADS);
	// 前面
	glTexCoord2f(0.0f, 0.0f); glVertex3f(-1.0f, -1.0f, 1.0f); // 纹理和四边形的左下
	glTexCoord2f(1.0f, 0.0f); glVertex3f( 1.0f, -1.0f, 1.0f); // 纹理和四边形的右下
	glTexCoord2f(1.0f, 1.0f); glVertex3f( 1.0f, 1.0f, 1.0f); // 纹理和四边形的右上
	glTexCoord2f(0.0f, 1.0f); glVertex3f(-1.0f, 1.0f, 1.0f); // 纹理和四边形的左上
	glEnd();
	// 后面
	glBindTexture(GL_TEXTURE_2D,texture[1]);
	glBegin(GL_QUADS);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(-1.0f, -1.0f, -1.0f); // 纹理和四边形的右下
	glTexCoord2f(1.0f, 1.0f); glVertex3f(-1.0f, 1.0f, -1.0f); // 纹理和四边形的右上
	glTexCoord2f(0.0f, 1.0f); glVertex3f( 1.0f, 1.0f, -1.0f); // 纹理和四边形的左上
	glTexCoord2f(0.0f, 0.0f); glVertex3f( 1.0f, -1.0f, -1.0f); // 纹理和四边形的左下
	glEnd();
	// 顶面
	
	glBindTexture(GL_TEXTURE_2D,texture[2]);
	glBegin(GL_QUADS);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(-1.0f, 1.0f, -1.0f); // 纹理和四边形的左上
	glTexCoord2f(0.0f, 0.0f); glVertex3f(-1.0f, 1.0f, 1.0f); // 纹理和四边形的左下
	glTexCoord2f(1.0f, 0.0f); glVertex3f( 1.0f, 1.0f, 1.0f); // 纹理和四边形的右下
	glTexCoord2f(1.0f, 1.0f); glVertex3f( 1.0f, 1.0f, -1.0f); // 纹理和四边形的右上
	glEnd();
	// 底面
	
	glBindTexture(GL_TEXTURE_2D,texture[3]);
	glBegin(GL_QUADS);
	glTexCoord2f(1.0f, 1.0f); glVertex3f(-1.0f, -1.0f, -1.0f); // 纹理和四边形的右上
	glTexCoord2f(0.0f, 1.0f); glVertex3f( 1.0f, -1.0f, -1.0f); // 纹理和四边形的左上
	glTexCoord2f(0.0f, 0.0f); glVertex3f( 1.0f, -1.0f, 1.0f); // 纹理和四边形的左下
	glTexCoord2f(1.0f, 0.0f); glVertex3f(-1.0f, -1.0f, 1.0f); // 纹理和四边形的右下
	glEnd();
	// 右面
	
	glBindTexture(GL_TEXTURE_2D,texture[4]);
	glBegin(GL_QUADS);
	glTexCoord2f(1.0f, 0.0f); glVertex3f( 1.0f, -1.0f, -1.0f); // 纹理和四边形的右下
	glTexCoord2f(1.0f, 1.0f); glVertex3f( 1.0f, 1.0f, -1.0f); // 纹理和四边形的右上
	glTexCoord2f(0.0f, 1.0f); glVertex3f( 1.0f, 1.0f, 1.0f); // 纹理和四边形的左上
	glTexCoord2f(0.0f, 0.0f); glVertex3f( 1.0f, -1.0f, 1.0f); // 纹理和四边形的左下
	glEnd();
	// 左面
	
	glBindTexture(GL_TEXTURE_2D,texture[5]);
	glBegin(GL_QUADS);
	glTexCoord2f(0.0f, 0.0f); glVertex3f(-1.0f, -1.0f, -1.0f); // 纹理和四边形的左下
	glTexCoord2f(1.0f, 0.0f); glVertex3f(-1.0f, -1.0f, 1.0f); // 纹理和四边形的右下
	glTexCoord2f(1.0f, 1.0f); glVertex3f(-1.0f, 1.0f, 1.0f); // 纹理和四边形的右上
	glTexCoord2f(0.0f, 1.0f); glVertex3f(-1.0f, 1.0f, -1.0f); // 纹理和四边形的左上
	glEnd();

	glFlush();
	glutSwapBuffers();
}


void showCube()
{
	//glutInit(&argc, argv);

	//glutInitWindowPosition(50, 50);
	//glutInitWindowSize(500, 500);
	//glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);

	//initGlui();
	glutCreateWindow("showCube");
	InitGL();
	glutDisplayFunc(DrawGLScene);
	glutMouseFunc(mouse);
	glutMotionFunc(mouseMotion);
	glutReshapeFunc(ReSizeGLSence);

	glClearColor(0.93f, 0.93f, 0.93f, 0.0f);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glClearDepth(1.0f);

	glutMainLoop();
	//return 0;
}
//Glui状态选择
void gluiControl(int control) {
	switch (control) {
	case CRTL_LOAD:		//选择“打开”控件
		loadFile();		//加载文件
		draw_state = DRAW_BY_FILE;
		break;
	case CRTL_MOUSE:	//选择手绘控件
		draw_state = DRAW_BY_MOUSE;
		break;
	case CRTL_CLEAR:	//选择清屏控件
		gluiClear();	//清屏
		draw_state = DRAW_NONE;
		break;
	case CRTL_SAVE:	//选择保存控件
		SaveScreenShot();	//截屏
		draw_state = DRAW_NONE;
		break;
	case CRTL_SHOW:	//选择展示控件
		showCube();	//展示立方体
		draw_state = DRAW_NONE;
		break;
	default:
		break;
	}
}

//初始化并显示到屏幕中央
void initWindow(int &argc, char *argv[], int width, int height, const char *title) {
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
	glutInitWindowPosition((GetSystemMetrics(SM_CXSCREEN) - width) >> 1, (GetSystemMetrics(SM_CYSCREEN) - height) >> 1);       //指定窗口位置
	glutInitWindowSize(width, height);       //指定窗口大小
	window_name = glutCreateWindow(title);

	glClearColor(1.0, 1.0, 1.0, 0.0);//设置清除颜色
	glClear(GL_COLOR_BUFFER_BIT);//把窗口清除为当前颜色
	glClearDepth(1.0);//指定深度缓冲区中每个像素需要的值
	glClear(GL_DEPTH_BUFFER_BIT);//清除深度缓冲区
	glShadeModel(GL_FLAT);
}

//初始化glui界面
void initGlui() {
	glutDisplayFunc(myGlutDisplay);		//绘图函数
	glutReshapeFunc(myGlutReshape);		//画面大小重构函数
	glutMouseFunc(myGlutMouse);			//鼠标点击函数
	glutMotionFunc(myGlutMotion);		//鼠标移动函数
	glutKeyboardFunc(myGlutKeyboard);	//键盘点击函数
	glutIdleFunc(myGlutIdle); //为GLUI注册一个标准的GLUT空闲回调函数，当系统处于空闲时,就会调用该注册的函数

	//GLUI
	GLUI *glui = GLUI_Master.create_glui_subwindow(window_name, GLUI_SUBWINDOW_RIGHT); //新建子窗体，位于主窗体的右部 
	new GLUI_StaticText(glui, "Bspline_in_cube"); //在GLUI下新建一个静态文本框，输出内容为“GLUI”

	new GLUI_Separator(glui); //新建分隔符
	new GLUI_Button(glui, "Open", CRTL_LOAD, gluiControl); //新建按钮控件，参数分别为：所属窗体、名字、ID、回调函数，当按钮被触发时,它会被调用.
	new GLUI_Button(glui, "Draw", CRTL_MOUSE, gluiControl);	//手动绘制
	new GLUI_Button(glui, "Clear", CRTL_CLEAR, gluiControl);	//清空画板
	new GLUI_Button(glui, "Save", CRTL_SAVE, gluiControl);//新建退出按钮，当按钮被触发时,退出程序/保存为bmp图片
	new GLUI_Button(glui, "Show", CRTL_SHOW, gluiControl);//展示立方体


	new GLUI_Separator(glui);
	new GLUI_Checkbox(glui, "Draw polygons?", &DrawPolygons);	//绘制多边形
	(new GLUI_Spinner(glui, "Sampling point density", &density))->set_int_limits(1, 500);


	glui->set_main_gfx_window(window_name); //将子窗体glui与主窗体main_window绑定，当窗体glui中的控件的值发生过改变，则该glui窗口被重绘
	glutIdleFunc(myGlutIdle);
}

int main(int argc, char *argv[]) {
	initWindow(argc, argv, 1000, 600, "Bspline_in_cube");
	
	cout << endl << "\t点击Open按钮打开文件绘制模式" << endl;
	cout << endl << "\t点击Draw按钮鼠标控制绘制模式" << endl;
	cout << "\t鼠标左键点击增加控制点，鼠标右键移动控制点" << endl;
	cout << "\t退格键（←）或删除键（delete）删除鼠标所在的点" << endl;
	cout << endl << "\t点击Clear按钮清空屏幕\t在切换绘制模式前需先点击Clear按钮" << endl;
	cout << endl << "\t点击Save按钮截取当前窗口并保存为bmp图片" << endl;
	cout << endl << "\t点击Show按钮将B样条绘制的名字展示在立方体上" << endl;
	cout << endl << "\t点击是否绘制封闭多边形可将直线封闭成多边形" << endl;
	cout << "\t调节density大小，调节采样点数量，范围[1,500]" << endl;
	cout << endl;

	initGlui();

	glutMainLoop();
	return 0;
}
