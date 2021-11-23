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
#define CRTL_LOAD			0x00	//���ļ��ؼ�
#define CRTL_MOUSE			0x01	//�����ƿؼ�
#define CRTL_CLEAR			0x02	//�����ؼ�
#define CRTL_SAVE			0x03    //����Ϊbmp�ؼ�
#define CRTL_SHOW			0x04	//չʾ������

#define DRAW_BY_FILE		0x00	//���ļ�����
#define DRAW_BY_MOUSE		0x01	//�����ƻ���
#define DRAW_NONE			0x02	//������
#define PI 3.1415926535897

using namespace std;


HGLRC hRC=NULL; //������ɫ��������
HDC hDC=NULL;//Opengl��Ⱦ��������
HWND hWnd=NULL;//�������ǵĴ��ھ��
HINSTANCE hInstance;//��������ʵ��
LRESULT CALLBACK WndProc(HWND,UINT,WPARAM,LPARAM);//WndProc�Ķ��壬Ϊ��creategl window��������

bool mouseRightIsDown = false;		//�Ҽ��Ƿ���
int DrawPolygons = 0;				//�Ƿ���Ʒ�ն����
int density;						//����������
int window_x, window_y;				//��������
int window_name;					//��������
int draw_state = DRAW_NONE;			//����״̬
int num=0;
bool active=TRUE;
bool fullscreen=TRUE;
bool keys[256];

GLfloat rquad;
GLfloat xrot;
GLfloat yrot;
GLfloat zrot;
GLuint texture[6];//�洢6������
bool mouseDown = false;
float xdiff = 0.0f;
float ydiff = 0.0f;

//��
typedef struct Point {
	double x, y;
	Point() {}
	Point(double x, double y) :x(x), y(y) {}
};

//����
class Obj {
public:
	int degree;				//B�������ߵĴ���
	int cnt_num;			//���Ƶ������
	vector<double> u;		//�ڵ�ʸ��
	vector<Point> points;	//���Ƶ㼯��

	//��ȡtxt�ļ�
	void ReadFile(string filename);

	Obj() {
		degree = 0;
		cnt_num = 0;
		u.clear();
		points.clear();
	}
};
Obj obj;

//��ȡtxt�ļ�
void Obj::ReadFile(string filename) {
	fstream fpFile;
	fpFile.open(filename, ios::in); //���ļ�
	if (!fpFile.is_open())	//��ʧ�ܷ���
		return;

	points.clear();	//���
	u.clear();

	string line;

	//�������
	line = "";
	getline(fpFile, line);
	degree = atoi(line.c_str());	//stringתint
	//������Ƶ�����
	line = "";
	getline(fpFile, line);
	cnt_num = atoi(line.c_str());
	//����ڵ�ʸ��
	line = "";
	getline(fpFile, line);
	string str = "";
	line += " ";
	for (unsigned i = 0; i < line.length(); i++) {
		if (line[i] != ' ')
			str += line[i];
		else {
			double ui;
			istringstream iss(str);		//stringתdouble
			iss >> ui;
			u.push_back(ui);
			str = "";
		}
	}
	//������Ƶ�����
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

	fpFile.close();		//�ر��ļ�
}

//����ģ��
void loadFile() {
	//����ϵͳ�Ի���
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

	obj.ReadFile(fname.lpstrFile); //����ģ���ļ�
}

//�������
double getRatio(double t, double a, double b, double c, double d) {
	return a * pow(t, 3) + b * pow(t, 2) + c * t + d;
}

//��������
double caculateSquarDistance(Point a, Point b) {
	return (a.x - b.x)*(a.x - b.x) + (a.y - b.y)*(a.y - b.y);
}

//��������뺯��
int getIndexNearByMouse(double x, double y) {
	double precision = 200;     //��ȷ��
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
	int m = obj.cnt_num + obj.degree;	//��
	int n = obj.degree + 1;		//��
	vector<double> u = obj.u;
	vector<vector<double> > B_spline(m, vector<double>(n, 0));

	// �� 0 - 1 ӳ�䵽��Ӧȡֵ��Χ��
	double umin = u[obj.degree];
	double umax = u[obj.cnt_num];
	a = a * (umax - umin) + umin;

	// ����ÿһ��
	for (int j = 0; j < n; j++) {
		// ����ÿһ��
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


//���ξ���b�������ߺ���
void Bezier(Point a, Point b, Point c, Point d) {
	int n = density;
	double derta = 1.0 / n;	//�������ܶ�
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


//��ͼ����
void myGlutDisplay() {
	if (draw_state == DRAW_NONE)	//״̬Ϊ������ʱ��ֱ�ӷ���
		return;

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);    //�����ɫ�������Ȼ���

	//����
	glPointSize(3.0);
	glColor3d(1.0, 0.0, 0.0);
	glBegin(GL_POINTS);
	for (int i = 0; i < obj.cnt_num; i++)
		glVertex2d(obj.points[i].x, obj.points[i].y);
	glEnd();

	//��ֱ��
	glLineWidth(2);
	glColor3d(1.0, 0.0, 0.0);
	if (DrawPolygons == 0)		//�ж��Ƿ���Ʒ�ն����
		glBegin(GL_LINE_STRIP);
	else if (DrawPolygons == 1)
		glBegin(GL_LINE_LOOP);
	
	for (int i = 0; i < obj.cnt_num; i++)
		glVertex2d(obj.points[i].x, obj.points[i].y);
	
	glEnd();
	
	//������
	glLineWidth(2);
	glPointSize(2);
	glColor3d(0, 0, 0);

	//���������
	if (draw_state == DRAW_BY_MOUSE) {
		if (obj.cnt_num >= 4)
		for (int i = 0; i < obj.cnt_num - 3; i++) {
			Bezier(obj.points[i], obj.points[i + 1], obj.points[i + 2], obj.points[i + 3]);
		}
	}

	//���ļ�����
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

//�������뺯��
void myGlutKeyboard(unsigned char key, int x, int y) {
	//ESC
	if (key == 27)
		exit(0);

	//�˸����ɾ����
	if (key == 8 || key == 127) {
		int index = getIndexNearByMouse(x - window_x / 2, window_y / 2 - y);
		if (index == -1)
			return;
		obj.points.erase(obj.points.begin() + index);
		obj.cnt_num--;
		glutPostRedisplay();
	}
}

//���������
void myGlutMouse(int button, int state, int x, int y) {

	//�������
	if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {	
		if (draw_state == DRAW_BY_FILE || draw_state == DRAW_NONE)	//ֻ�е��������������״̬�²������ӽ��
			return;
		Point p(x - window_x / 2, window_y / 2 - y);
		obj.points.push_back(p);
		obj.cnt_num++;
		glutPostRedisplay();
	}

	//�Ҽ��ƶ�
	if (button == GLUT_RIGHT_BUTTON && state == GLUT_DOWN)
		mouseRightIsDown = true;
	if (button == GLUT_RIGHT_BUTTON && state == GLUT_UP)
		mouseRightIsDown = false;
}

//����ƶ�����
void myGlutMotion(int x, int y) {

	//��ס�Ҽ��ƶ���
	if (mouseRightIsDown) {
		int index = getIndexNearByMouse(x - window_x / 2, window_y / 2 - y);
		if (index == -1)
			return;
		obj.points[index].x = x - window_x / 2;
		obj.points[index].y = window_y / 2 - y;
		glutPostRedisplay();
	}
}

//�����С�ع�����
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

//���лص�����
void myGlutIdle() {
	if (glutGetWindow() != window_name)
		glutSetWindow(window_name);

	glutPostRedisplay();
}

//�����Ļ
void gluiClear() {
	glClearColor(1.0, 1.0, 1.0, 0.0);//���������ɫ
	glClear(GL_COLOR_BUFFER_BIT);//�Ѵ������Ϊ��ǰ��ɫ
	glClearDepth(1.0);//ָ����Ȼ�������ÿ��������Ҫ��ֵ
	glClear(GL_DEPTH_BUFFER_BIT);//�����Ȼ�����

	Obj newobj;
	obj = newobj;

	glutPostRedisplay();
}

//WriteBitmapFile
//����bitmapData�ģ�RGB�����ݣ�����bitmap
//filename��Ҫ���浽����Ӳ�̵��ļ���������·����
BOOL WriteBitmapFile(char * filename,int width,int height,unsigned char * bitmapData)
{
 //���BITMAPFILEHEADER
 BITMAPFILEHEADER bitmapFileHeader;
 memset(&bitmapFileHeader,0,sizeof(BITMAPFILEHEADER));
 bitmapFileHeader.bfSize = sizeof(BITMAPFILEHEADER);
 bitmapFileHeader.bfType = 0x4d42; //BM
 bitmapFileHeader.bfOffBits =sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
 
 //���BITMAPINFOHEADER
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
 FILE * filePtr;   //����Ҫ�����bitmap�ļ���
 unsigned char tempRGB; //��ʱɫ��
 int imageIdx;
 
 //����R��B������λ��,bitmap���ļ����õ���BGR,�ڴ����RGB
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
//���洰�ڿͻ��˵Ľ�ͼ
//���ڴ�С* 600
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

//����λͼ��ת������
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
		// ʹ������λͼ�������� �ĵ�������
		for(int i=0;i<6;i++)
		{
			glBindTexture(GL_TEXTURE_2D,texture[i]);
			glTexImage2D(GL_TEXTURE_2D, 0, 3, TextureImage[i]->sizeX, TextureImage[i]->sizeY, 0, 
	GL_RGB, GL_UNSIGNED_BYTE, TextureImage[i]->data);
			glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR); // �����˲�
			glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR); // �����˲�
		}
	}
	for(int i=0;i<6;i++)
	{
		if (TextureImage[i])        // �����Ƿ����
		{
			if (TextureImage[i]->data)      // ����ͼ���Ƿ����
			{
				free(TextureImage[i]->data);     // �ͷ�����ͼ��ռ�õ��ڴ�
			}

			free(TextureImage[i]);       // �ͷ�ͼ��ṹ
  
		 }
	}
	return Status; // ���� Status

}
//add
GLvoid ReSizeGLSence(GLsizei width,GLsizei height)//����opengl���ڴ�С
{
    if(height==0)
    {
        height=1;
    }
    glViewport(0,0,width,height);
	glMatrixMode(GL_PROJECTION);//ѡ��ͶӰ����
	glLoadIdentity();//����ͶӰ����
    gluPerspective(45.0f,(GLfloat)width/(GLfloat)height,0.1f,100.0f);//�����ӿڴ�Сadd........................
    glMatrixMode(GL_MODELVIEW);//ѡ��ģ�͹۲����add
    glLoadIdentity();//����ģ�͹۲����add
}


//add
GLvoid KillGLWindow(GLvoid)//�������ٴ���
{
    if(fullscreen){
        ChangeDisplaySettings(NULL,0);
        ShowCursor(TRUE);
    }
    if(hRC){
        if(!wglMakeCurrent(NULL,NULL))
        {
            MessageBox(NULL,"�ͷ�DC��RCʧ�ܡ�","�رմ���",MB_OK|MB_ICONINFORMATION);
        }
        if(!wglDeleteContext(hRC))
        {
            MessageBox(NULL,"�ͷ�RCʧ��.","�رմ���",MB_OK|MB_ICONINFORMATION);
        }
        hRC=NULL;
    }
    if(hDC&&!ReleaseDC(hWnd,hDC))
    {
        MessageBox(NULL,"�ͷ�DCʧ�ܡ�","�رմ���",MB_OK|MB_ICONINFORMATION);
        hDC=NULL;
    }
    if(hWnd&&!DestroyWindow(hWnd))
    {
        MessageBox(NULL,"�ͷŴ��ھ��ʧ�ܡ�","�رմ���",MB_OK|MB_ICONINFORMATION);
        hWnd=NULL;
    }
    if(!UnregisterClass("OpenG",hInstance))
    {
        MessageBox(NULL,"����ע�������ࡣ","�رմ���",MB_OK|MB_ICONINFORMATION);
        hInstance=NULL;
    }
}

//��? ????�١�?
int InitGL(GLvoid) {
	
	if (!LoadGLTextures()) // ������������������
	{
		std::cout << "!LoadGLTextures"  << std::endl;
		return FALSE; // ���δ�����룬����FALSE
	}
	glEnable(GL_TEXTURE_2D); // ��������ӳ��
    glShadeModel(GL_SMOOTH);//������Ӱƽ��add
    glClearColor(1, 1, 1, 1);//��ɫ����
	glClear(GL_COLOR_BUFFER_BIT);
    glClearDepth(1.0f);//������Ȼ���add
    glEnable(GL_DEPTH_TEST);//������Ȳ���add
    glDepthFunc(GL_LEQUAL);//������Ȳ�������add
    glHint(GL_PERSPECTIVE_CORRECTION_HINT,GL_NICEST);//����ϵͳ��͸�ӽ�������add
	return TRUE;
}

//add��������
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
        MessageBox(NULL,"ע�ᴰ��ʧ��","����",MB_OK|MB_ICONEXCLAMATION);
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
            if(MessageBox(NULL,"ȫ��ģʽ�ڵ�ǰ�Կ�������ʧ�ܣ�\nʹ�ô���ģʽ��","NeHeG",MB_YESNO|MB_ICONEXCLAMATION)==IDYES)
            {
                fullscreen=FALSE;
            }
            else
            {
                MessageBox(NULL,"���򽫱��ر�","����",MB_OK|MB_ICONSTOP);
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
        MessageBox(NULL,"���ܴ���һ�������豸������","����",MB_OK|MB_ICONEXCLAMATION);
        return FALSE;
    }
    // add�������ظ�ʽ
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
        MessageBox(NULL,"���ܴ���һ����ƥ������ظ�ʽ","����",MB_OK|MB_ICONEXCLAMATION);
        return FALSE;
    }
    if(!(PixelFormat=ChoosePixelFormat(hDC,&pfd)))
    {
        KillGLWindow();
        MessageBox(NULL,"�����������ظ�ʽ","����",MB_OK|MB_ICONEXCLAMATION);
        return FALSE;
    }
    if(!SetPixelFormat(hDC,PixelFormat,&pfd))
    {
        KillGLWindow();
        MessageBox(NULL,"�����������ظ�ʽ","����",MB_OK|MB_ICONEXCLAMATION);
        return FALSE;
    }
    if(!(hRC=wglCreateContext(hDC)))
    {
        KillGLWindow();
        MessageBox(NULL,"���ܴ���OpenGL��Ⱦ������","����",MB_OK|MB_ICONEXCLAMATION);
        return FALSE;
    }
    if(!wglMakeCurrent(hDC,hRC))
    {
        KillGLWindow();
        MessageBox(NULL,"���ܼ��ǰ��OpenGL��Ⱦ������","����",MB_OK|MB_ICONEXCLAMATION);
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

//add ��������Ϣ
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
// ����ƶ��¼�
void mouseMotion(int x, int y) {
	if (mouseDown) {
		yrot = x - xdiff;
		xrot = y + ydiff;
		std::cout << "yrot:" << yrot << "\txrot" << xrot << std::endl;

		glutPostRedisplay();
	}
}
//����
void  DrawGLScene(GLvoid)
{
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);//�����Ļ����Ȼ���
    glLoadIdentity();//���õ�ǰ��ģ�͹۲����
	glTranslatef(0.0f,0.0f,-5.0f);
	glRotatef(xrot,1.0f,0.0f,0.0f); // ��X����ת
	glRotatef(yrot,0.0f,1.0f,0.0f); // ��Y����ת
	//glRotatef(zrot,0.0f,0.0f,1.0f); // ��Z����ת

	glBindTexture(GL_TEXTURE_2D,texture[0]);
    glBegin(GL_QUADS);
	// ǰ��
	glTexCoord2f(0.0f, 0.0f); glVertex3f(-1.0f, -1.0f, 1.0f); // ������ı��ε�����
	glTexCoord2f(1.0f, 0.0f); glVertex3f( 1.0f, -1.0f, 1.0f); // ������ı��ε�����
	glTexCoord2f(1.0f, 1.0f); glVertex3f( 1.0f, 1.0f, 1.0f); // ������ı��ε�����
	glTexCoord2f(0.0f, 1.0f); glVertex3f(-1.0f, 1.0f, 1.0f); // ������ı��ε�����
	glEnd();
	// ����
	glBindTexture(GL_TEXTURE_2D,texture[1]);
	glBegin(GL_QUADS);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(-1.0f, -1.0f, -1.0f); // ������ı��ε�����
	glTexCoord2f(1.0f, 1.0f); glVertex3f(-1.0f, 1.0f, -1.0f); // ������ı��ε�����
	glTexCoord2f(0.0f, 1.0f); glVertex3f( 1.0f, 1.0f, -1.0f); // ������ı��ε�����
	glTexCoord2f(0.0f, 0.0f); glVertex3f( 1.0f, -1.0f, -1.0f); // ������ı��ε�����
	glEnd();
	// ����
	
	glBindTexture(GL_TEXTURE_2D,texture[2]);
	glBegin(GL_QUADS);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(-1.0f, 1.0f, -1.0f); // ������ı��ε�����
	glTexCoord2f(0.0f, 0.0f); glVertex3f(-1.0f, 1.0f, 1.0f); // ������ı��ε�����
	glTexCoord2f(1.0f, 0.0f); glVertex3f( 1.0f, 1.0f, 1.0f); // ������ı��ε�����
	glTexCoord2f(1.0f, 1.0f); glVertex3f( 1.0f, 1.0f, -1.0f); // ������ı��ε�����
	glEnd();
	// ����
	
	glBindTexture(GL_TEXTURE_2D,texture[3]);
	glBegin(GL_QUADS);
	glTexCoord2f(1.0f, 1.0f); glVertex3f(-1.0f, -1.0f, -1.0f); // ������ı��ε�����
	glTexCoord2f(0.0f, 1.0f); glVertex3f( 1.0f, -1.0f, -1.0f); // ������ı��ε�����
	glTexCoord2f(0.0f, 0.0f); glVertex3f( 1.0f, -1.0f, 1.0f); // ������ı��ε�����
	glTexCoord2f(1.0f, 0.0f); glVertex3f(-1.0f, -1.0f, 1.0f); // ������ı��ε�����
	glEnd();
	// ����
	
	glBindTexture(GL_TEXTURE_2D,texture[4]);
	glBegin(GL_QUADS);
	glTexCoord2f(1.0f, 0.0f); glVertex3f( 1.0f, -1.0f, -1.0f); // ������ı��ε�����
	glTexCoord2f(1.0f, 1.0f); glVertex3f( 1.0f, 1.0f, -1.0f); // ������ı��ε�����
	glTexCoord2f(0.0f, 1.0f); glVertex3f( 1.0f, 1.0f, 1.0f); // ������ı��ε�����
	glTexCoord2f(0.0f, 0.0f); glVertex3f( 1.0f, -1.0f, 1.0f); // ������ı��ε�����
	glEnd();
	// ����
	
	glBindTexture(GL_TEXTURE_2D,texture[5]);
	glBegin(GL_QUADS);
	glTexCoord2f(0.0f, 0.0f); glVertex3f(-1.0f, -1.0f, -1.0f); // ������ı��ε�����
	glTexCoord2f(1.0f, 0.0f); glVertex3f(-1.0f, -1.0f, 1.0f); // ������ı��ε�����
	glTexCoord2f(1.0f, 1.0f); glVertex3f(-1.0f, 1.0f, 1.0f); // ������ı��ε�����
	glTexCoord2f(0.0f, 1.0f); glVertex3f(-1.0f, 1.0f, -1.0f); // ������ı��ε�����
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
//Glui״̬ѡ��
void gluiControl(int control) {
	switch (control) {
	case CRTL_LOAD:		//ѡ�񡰴򿪡��ؼ�
		loadFile();		//�����ļ�
		draw_state = DRAW_BY_FILE;
		break;
	case CRTL_MOUSE:	//ѡ���ֻ�ؼ�
		draw_state = DRAW_BY_MOUSE;
		break;
	case CRTL_CLEAR:	//ѡ�������ؼ�
		gluiClear();	//����
		draw_state = DRAW_NONE;
		break;
	case CRTL_SAVE:	//ѡ�񱣴�ؼ�
		SaveScreenShot();	//����
		draw_state = DRAW_NONE;
		break;
	case CRTL_SHOW:	//ѡ��չʾ�ؼ�
		showCube();	//չʾ������
		draw_state = DRAW_NONE;
		break;
	default:
		break;
	}
}

//��ʼ������ʾ����Ļ����
void initWindow(int &argc, char *argv[], int width, int height, const char *title) {
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
	glutInitWindowPosition((GetSystemMetrics(SM_CXSCREEN) - width) >> 1, (GetSystemMetrics(SM_CYSCREEN) - height) >> 1);       //ָ������λ��
	glutInitWindowSize(width, height);       //ָ�����ڴ�С
	window_name = glutCreateWindow(title);

	glClearColor(1.0, 1.0, 1.0, 0.0);//���������ɫ
	glClear(GL_COLOR_BUFFER_BIT);//�Ѵ������Ϊ��ǰ��ɫ
	glClearDepth(1.0);//ָ����Ȼ�������ÿ��������Ҫ��ֵ
	glClear(GL_DEPTH_BUFFER_BIT);//�����Ȼ�����
	glShadeModel(GL_FLAT);
}

//��ʼ��glui����
void initGlui() {
	glutDisplayFunc(myGlutDisplay);		//��ͼ����
	glutReshapeFunc(myGlutReshape);		//�����С�ع�����
	glutMouseFunc(myGlutMouse);			//���������
	glutMotionFunc(myGlutMotion);		//����ƶ�����
	glutKeyboardFunc(myGlutKeyboard);	//���̵������
	glutIdleFunc(myGlutIdle); //ΪGLUIע��һ����׼��GLUT���лص���������ϵͳ���ڿ���ʱ,�ͻ���ø�ע��ĺ���

	//GLUI
	GLUI *glui = GLUI_Master.create_glui_subwindow(window_name, GLUI_SUBWINDOW_RIGHT); //�½��Ӵ��壬λ����������Ҳ� 
	new GLUI_StaticText(glui, "Bspline_in_cube"); //��GLUI���½�һ����̬�ı����������Ϊ��GLUI��

	new GLUI_Separator(glui); //�½��ָ���
	new GLUI_Button(glui, "Open", CRTL_LOAD, gluiControl); //�½���ť�ؼ��������ֱ�Ϊ���������塢���֡�ID���ص�����������ť������ʱ,���ᱻ����.
	new GLUI_Button(glui, "Draw", CRTL_MOUSE, gluiControl);	//�ֶ�����
	new GLUI_Button(glui, "Clear", CRTL_CLEAR, gluiControl);	//��ջ���
	new GLUI_Button(glui, "Save", CRTL_SAVE, gluiControl);//�½��˳���ť������ť������ʱ,�˳�����/����ΪbmpͼƬ
	new GLUI_Button(glui, "Show", CRTL_SHOW, gluiControl);//չʾ������


	new GLUI_Separator(glui);
	new GLUI_Checkbox(glui, "Draw polygons?", &DrawPolygons);	//���ƶ����
	(new GLUI_Spinner(glui, "Sampling point density", &density))->set_int_limits(1, 500);


	glui->set_main_gfx_window(window_name); //���Ӵ���glui��������main_window�󶨣�������glui�еĿؼ���ֵ�������ı䣬���glui���ڱ��ػ�
	glutIdleFunc(myGlutIdle);
}

int main(int argc, char *argv[]) {
	initWindow(argc, argv, 1000, 600, "Bspline_in_cube");
	
	cout << endl << "\t���Open��ť���ļ�����ģʽ" << endl;
	cout << endl << "\t���Draw��ť�����ƻ���ģʽ" << endl;
	cout << "\t������������ӿ��Ƶ㣬����Ҽ��ƶ����Ƶ�" << endl;
	cout << "\t�˸����������ɾ������delete��ɾ��������ڵĵ�" << endl;
	cout << endl << "\t���Clear��ť�����Ļ\t���л�����ģʽǰ���ȵ��Clear��ť" << endl;
	cout << endl << "\t���Save��ť��ȡ��ǰ���ڲ�����ΪbmpͼƬ" << endl;
	cout << endl << "\t���Show��ť��B�������Ƶ�����չʾ����������" << endl;
	cout << endl << "\t����Ƿ���Ʒ�ն���οɽ�ֱ�߷�ճɶ����" << endl;
	cout << "\t����density��С�����ڲ�������������Χ[1,500]" << endl;
	cout << endl;

	initGlui();

	glutMainLoop();
	return 0;
}
