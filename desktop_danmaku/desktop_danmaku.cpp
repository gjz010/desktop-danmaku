// desktop_danmaku.cpp : Defines the entry point for the application.
//
#include "stdafx.h"
#include "desktop_danmaku.h"
#include <set>
#include <string>
#include <objidl.h>
#include <gdiplus.h>
#include <io.h>
#include <fcntl.h>
#include <thread>
#include <cstdio>
#include <cstring>
#include <random>
#include <mutex>
#include <algorithm>
#include <conio.h>
#include <WinSock2.h>
#include <queue>
//cpp_redis.lib
//tacopie.lib
#pragma comment (lib,"Gdiplus.lib")
#pragma comment(lib, "ws2_32.lib")
#define MAX_LOADSTRING 100
#define PORT 6789
#define MAX_DANMAKU 50
#define IP "127.0.0.1"
bool updating = false;
struct Danmaku {
	std::wstring content;
	Gdiplus::Brush* brush;
	int length;
	int x;
	int y;
	int idx;
	bool operator<(const Danmaku& d) const{
		return x > d.x;
	}
};
std::priority_queue<int, std::vector<int>, std::greater<int>> pq;
int max_id = 0;
std::random_device rd;
std::mutex dl_mutex;
std::vector<Danmaku> danmaku_list;
// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int wheight;
int wwidth;
Gdiplus::Color list[9] = {
	Gdiplus::Color(0xff,0x30,0x30),
	Gdiplus::Color::Yellow,
	Gdiplus::Color::Orange,
	Gdiplus::Color(0x66,0xcc,0xff),
	Gdiplus::Color::Green,
	Gdiplus::Color::Pink,
	Gdiplus::Color::LightGreen,
	Gdiplus::Color::Blue,
	Gdiplus::Color::Gold
};
void PushDanmaku(std::wstring& content, Gdiplus::Color color, bool special) {
	std::lock_guard<std::mutex> guard(dl_mutex);
	Gdiplus::Font font(L"黑体", 32, Gdiplus::FontStyleBold);
	int title = -1;
	if (special) {
		if (pq.size() == 0) {
			title = max_id++;
		}
		else {
			title = pq.top();
			pq.pop();
		}
	}
	Danmaku dm{ content,new Gdiplus::SolidBrush((special?list[0]:color)),0,wwidth,rd() % wheight ,title};

	danmaku_list.push_back(dm);
	updating = true;
}

BOOL StringToWString(const std::string &str, std::wstring &wstr)
{
	int nLen = (int)str.length();
	wstr.resize(nLen, L' ');
	int nResult = MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)str.c_str(), -1, (LPWSTR)wstr.c_str(), nLen);
	if (nResult == 0)
	{
		return FALSE;
	}
	return TRUE;
}

void NetworkHandler(SOCKET client) {
	_cprintf("Socket connected!\n");
	char buf[1024];
	std::string str = "";
	bool read_num = true;
	bool special = false;
	int length = 0;
	while (true) {
		memset(buf, 0, sizeof(buf));
		int ret = recv(client, buf, sizeof(buf), 0);
		if (ret <= 0) break;
		_cprintf("Reading length of %d\n",ret);
		for (int i = 0; i < ret; i++) {
			if (read_num) {
				_cprintf("Processing %c(%d)\n",buf[i],buf[i]);

				if (buf[i] == ' ' || buf[i]=='*') {
					_cprintf("Reading a message of length %d\n",length);
					if (length == 0) {
						_cprintf("Empty string detected!\n");
						read_num = true;
						continue;
					}
					special = buf[i] == '*';
					//_cprintf("%d\n",std::wstring(L"野兽先辈").size());
					read_num = false;
					continue;
				}
				else {
					if (buf[i]<'0' || buf[i]>'9') {
						_cprintf("Wrong number!Reloading...\n");
						length = 0;
						continue;
					}else
					length = 10 * length + buf[i] - '0';
				}
			}
			else {
				length--;
				str.push_back(buf[i]);
				if (length == 0) {
					read_num = true;
					_cprintf("%s\n",str.c_str());
					//The yajue here.
					std::wstring wstr;
					
					_cprintf("Got! %d\n", StringToWString(str, wstr));
					_cprintf("%d->%d\n", str.size(),wstr.size());
					send(client, "D", 1, 0);
					PushDanmaku(wstr, list[rd()%8+1],special);
					str = "";
				}
			}
		}

	}
}
void NetworkThread() {
	WSADATA wsa;
	SOCKET server;
	WSAStartup(MAKEWORD(2, 2), &wsa);
	server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	SOCKADDR_IN addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(PORT);
	addr.sin_addr.S_un.S_addr = INADDR_ANY;
	bind(server, (sockaddr *)&addr, sizeof(sockaddr));
	listen(server, 10);
	sockaddr_in addrClient;
	int addrClientLen = sizeof(sockaddr_in);
	while (TRUE) {
		SOCKET sClient;
		//_cprintf("Socket connected!\n");
		sClient = accept(server, (sockaddr*)&addrClient, &addrClientLen);
		new std::thread([&] {
			NetworkHandler(sClient);
		});
	}
	closesocket(server);
	WSACleanup();
}
//cpp_redis::redis_subscriber sub;
int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

	
    // TODO: Place code here.
	AllocConsole();


	std::thread network(&NetworkThread);

	/*
	sub->subscribe("desktop_danmaku", [](const std::string& chan, const std::string& msg) {
		_cprintf("Got!\n");
		std::wstring wstr;
		StringToWString(chan, wstr);
		PushDanmaku(wstr, Gdiplus::Color(rand() % 255, rand() % 255, rand() % 255));
	});
	*/
    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_DESKTOP_DANMAKU, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_DESKTOP_DANMAKU));

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_DESKTOP_DANMAKU));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_DESKTOP_DANMAKU);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
char buffer[2048];
RECT r;
ULONG_PTR gdiToken;

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Store instance handle in our global variable
   //SetProcessDPIAware();
   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW| WS_EX_TOPMOST,
      0, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }
   SystemParametersInfo(SPI_GETWORKAREA, 0, &r, 0);
   sprintf(buffer,"%d %d %d %d", r.top, r.bottom, r.left, r.right);
   WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),buffer,strlen(buffer),NULL,NULL);
   Gdiplus::GdiplusStartupInput gsi(NULL, false, false);
   Gdiplus::GdiplusStartup(&gdiToken, &gsi, NULL);
  
   SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, r.right, r.bottom, SWP_NOMOVE);
   wwidth = r.right;
   wheight = r.bottom;
   SetWindowLong(hWnd, GWL_STYLE, 0);
   LONG nRet = ::GetWindowLong(hWnd, GWL_EXSTYLE);
   nRet |= WS_EX_LAYERED;
   ::SetWindowLong(hWnd, GWL_EXSTYLE, nRet);
   ::SetLayeredWindowAttributes(hWnd, 0, 128, LWA_ALPHA);
   SetLayeredWindowAttributes(hWnd,RGB(255, 255, 255), 160, LWA_COLORKEY|LWA_ALPHA);
   SetMenu(hWnd, NULL);
   ::SetForegroundWindow(hWnd);
   ::LockSetForegroundWindow(1);
   
   //Danmaku d = { L"HHH",20,RGB(0,255,0),20,20 };

   //PushDanmaku(std::wstring(L"说得太好了"), Gdiplus::Color(0, 255, 0));
   //PushDanmaku(std::wstring(L"HelloKugou 矛盾"), Gdiplus::Color(255, 0, 0));
   
   //danmaku_list.insert(d);
   UpdateWindow(hWnd);
   ShowWindow(hWnd, nCmdShow);
   //graphics = new Gdiplus::Graphics(hWnd, false);
   

   return TRUE;
}
static void DrawLine(HDC hDC, int x0, int y0, int x1, int y1, int style, int width, COLORREF color)
{
	HPEN hPen = CreatePen(style, width, color);
	HPEN hOldPen = (HPEN)SelectObject(hDC, hPen);

	MoveToEx(hDC, x0, y0, NULL);
	LineTo(hDC, x1, y1);

	SelectObject(hDC, hOldPen);
	DeleteObject(hPen);
}
//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
int apt = 0;

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
	case WM_CREATE:
		SetTimer(hWnd, 1, 30, NULL);
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // Parse the menu selections:
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
	case WM_ERASEBKGND:
		return 1;
	case WM_PAINT:
	{
		if (danmaku_list.empty()) updating = false;
		std::lock_guard<std::mutex> guard(dl_mutex);
		RECT rc;
		GetClientRect(hWnd, &rc);
		SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, rc.right, rc.bottom, SWP_NOMOVE);
		//HDC memory
		Gdiplus::Bitmap bmp(int(rc.right), int(rc.bottom));
		Gdiplus::Graphics buffer(&bmp);
		buffer.Clear(Gdiplus::Color::White);

		Gdiplus::FontFamily fontFamily(L"黑体");
		Gdiplus::Font myfont(&fontFamily, 48, Gdiplus::FontStyleBold, Gdiplus::UnitPixel);
		Gdiplus::StringFormat stringformat;

		stringformat.SetAlignment(Gdiplus::StringAlignmentNear);
		stringformat.SetLineAlignment(Gdiplus::StringAlignmentCenter);
		//buffer.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
		buffer.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);

		buffer.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAlias);
		std::vector<Danmaku> tmpd;
		int counter=0;
		std::for_each(danmaku_list.begin(), danmaku_list.end(), [&](Danmaku dm) {

			if (dm.length == 0) {
				Gdiplus::RectF r;
				Gdiplus::PointF p = { 0,0 };
				buffer.MeasureString(dm.content.c_str(), dm.content.length(), &myfont, p, &r);
				dm.length = r.GetRight();
			}
			if (counter == MAX_DANMAKU && dm.idx == -1) {
				tmpd.push_back(dm);
				return;
			}
			//
			dm.x -= 5;
			Gdiplus::PointF point(dm.x, dm.y);
			if (dm.idx != -1) {
				point.X = 0.5*(rc.right - dm.length);
				point.Y = 50 * dm.idx+30;

			}
			else {
				counter++;
			}
			Gdiplus::GraphicsPath path;
			path.AddString(dm.content.c_str(), -1, &fontFamily, Gdiplus::FontStyleBold, 48,
				point, &stringformat);
			Gdiplus::Pen pen(Gdiplus::Color(0, 0, 0), 3);
			buffer.DrawPath(&pen, &path);
			buffer.FillPath(dm.brush, &path);
			//buffer.DrawString
			if (dm.x + dm.length>0) tmpd.push_back(dm);
			else {
				delete dm.brush;
				if (dm.idx != -1) {
					pq.push(dm.idx);
				}
			}
		});
		danmaku_list = tmpd;
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hWnd, &ps);
		//BitBlt(hdc, 0, 0, rc.right, rc.bottom, buffer.GetHDC(), 0, 0,SRCPAINT);
		Gdiplus::Graphics graphics(hdc);
		Gdiplus::CachedBitmap cb(&bmp, &graphics);
		
		//graphics.Clear(Gdiplus::Color::White);
		graphics.DrawCachedBitmap(&cb, 0, 0);
		//graphics.DrawEllipse(&Gdiplus::Pen(Gdiplus::Color::Red, 1), Gdiplus::Rect(60, apt, 30, 30));
		EndPaint(hWnd, &ps);
		
 	}
		/*{
		
		
		

			buffer.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);


			
		

			//DrawLine(hdc, apt, 0, 1366, 768, PS_SOLID, 10, RGB(255, 0, 0));
			HDC hdc = BeginPaint(hWnd, &ps);
			//SendMessage(hWnd, WM_ERASEBKGND, (WPARAM)&hdc, 0);
			
			Gdiplus::Graphics graphics(hdc);
			Gdiplus::CachedBitmap cb(&bmp, &graphics);
			//graphics.DrawRectangle()
			//graphics.Clear(Gdiplus::Color::Transparent);
			//
			graphics.DrawCachedBitmap(&cb, 0, 0);
            // TODO: Add any drawing code that uses hdc here...
            EndPaint(hWnd, &ps);
        }
		*/

		break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
	case WM_TIMER:
		//apt += 5;
		/*
		if (rand() % 100 < 20) {
			PushDanmaku(std::wstring(L"苟利国家生死以"), Gdiplus::Color(rand() % 255, rand() % 255, rand() % 255));
		}
		*/ 
	{
		if (updating) {
			//delete guard;
			::RedrawWindow(hWnd, &r, 0, RDW_INVALIDATE | RDW_UPDATENOW);
		}
			
	}
				 break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
