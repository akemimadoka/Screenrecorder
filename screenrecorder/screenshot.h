#pragma once

typedef unsigned int uint;

/*
	获得一个屏幕快照
	参数说明：
	hDCscr：	要截图的设备上下文
	hDCout：	作为保存截图的设备上下文
	scrrect：	截图屏幕范围
	outrect：	截图的矩形
	bKeepcursor：在截图中保留光标，默认为保留

	返回值：	截图的位图句柄
*/
HBITMAP GetScreenShot(HDC hDCscr, HDC hDCout, RECT const& scrrect, RECT const& outrect, bool bKeepcursor = true);

/*
	初始化AVI流等
	参数说明：
	hDCout：	临时设备上下文，这里指截图所用的设备上下文
	hWnd：		接受音频相关信息的窗口句柄
	filename：	avi文件名
	quality：	avi的质量
	recordaudio：是否录制声音
*/
void InitAVI(HDC hDCout, HWND hWnd, LPCTSTR filename = _T("tmp.avi"), DWORD quality = 7500UL, bool recordaudio = false);

/*
	保存位图集到AVI文件
*/
void SaveAVI();

/*
	完成AVI流的操作
*/
void FinishAVI();

/*
	获得FPS
*/
uint& GetFPS();

/*
	获得当前帧数
*/
uint GetnFrames();

/*
	压入屏幕截图栈
	参数说明：
	ss：	位图句柄
*/
void PushScreenShotStack(HBITMAP ss);

/*
	准备打开录音
*/
void WimOpen();

/*
	准备关闭录音
*/
void WimClose();

/*
	处理录音数据
	参数说明：
	wParam：	wParam
	lParam：	lParam
*/
void WimData(WPARAM wParam, LPARAM lParam);