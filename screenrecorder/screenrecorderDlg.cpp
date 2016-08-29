
// screenrecorderDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "screenrecorder.h"
#include "screenrecorderDlg.h"
#include "afxdialogex.h"

#include "resource.h"

#include "screenshot.h"
#include <mmsystem.h>

#include <string>
#include <sstream>
#include <vector>
#include <io.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

typedef std::basic_string<TCHAR> tstring;

// 全局变量
namespace globalvars
{
	HDC hDCscr = GetDC(GetDesktopWindow());
	RECT scrrect;
	HDC hDCtmp = CreateCompatibleDC(hDCscr);

	bool bKeepCursor = true;
	uint FrameCache = 10u;
	uint tFrame = 0u;

	RecStatus rstatus = Stop;
}

// 其他函数

/*
	判断选择文件所属目录是否合法且存在
	参数说明：
	filename：	文件名
	返回值：	bool型，true为合法且存在
*/
bool isFileAvailable(tstring const& filename)
{
	return _taccess_s(filename.substr(0, filename.find_last_of(_T('\\'))).data(), 0) == 0;
}

// CscreenrecorderDlg 对话框



CscreenrecorderDlg::CscreenrecorderDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD, pParent)
	, m_TopMost(FALSE)
	, m_RecordCursor(FALSE)
	, m_Lbnframe(_T("0"))
{
	m_hIcon = AfxGetApp()->LoadIcon(IDI_ICOCIRNO);
}

void CscreenrecorderDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Check(pDX, IDC_CKTOPMOST, m_TopMost);
	DDX_Check(pDX, IDC_CKCURSOR, m_RecordCursor);
	DDX_Control(pDX, IDC_EDFPS, m_EditFPS);
	DDX_Control(pDX, IDC_EDIT1, m_EditFilename);
	DDX_Control(pDX, IDC_BNSTART, m_Bnstart);
	DDX_Control(pDX, IDC_BNPAUSE, m_Bnpause);
	DDX_Control(pDX, IDC_BNEND, m_Bnend);
	DDX_Control(pDX, IDC_CKCURSOR, m_Ckcursor);
	DDX_Text(pDX, IDC_LBNFRAME, m_Lbnframe);
	DDX_Control(pDX, IDC_BNSELECTFILE, m_Bnselectfile);
	DDX_Control(pDX, IDC_EDITFRAMECACHE, m_EditFrameCache);
	DDX_Control(pDX, IDC_CKRECORDAUDIO, m_Ckrecordaudio);
}

BEGIN_MESSAGE_MAP(CscreenrecorderDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_MESSAGE(WM_HOTKEY, &CscreenrecorderDlg::OnHotKey)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_BNSTART, &CscreenrecorderDlg::OnBnClickedBnstart)
	ON_BN_CLICKED(IDC_CKTOPMOST, &CscreenrecorderDlg::OnBnClickedCktopmost)
	ON_WM_TIMER()
	ON_BN_CLICKED(IDC_BNSELECTFILE, &CscreenrecorderDlg::OnBnClickedBnselectfile)
	ON_BN_CLICKED(IDC_BNPAUSE, &CscreenrecorderDlg::OnBnClickedBnpause)
	ON_BN_CLICKED(IDC_BNEND, &CscreenrecorderDlg::OnBnClickedBnend)
	ON_MESSAGE(MM_WIM_DATA, &CscreenrecorderDlg::OnWimData)
	ON_MESSAGE(MM_WIM_OPEN, &CscreenrecorderDlg::OnWimOpen)
	ON_MESSAGE(MM_WIM_CLOSE, &CscreenrecorderDlg::OnWimClose)
END_MESSAGE_MAP()


// CscreenrecorderDlg 消息处理程序

BOOL CscreenrecorderDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	skinppLoadSkinFromRes(AfxGetInstanceHandle(), MAKEINTRESOURCEA(IDR_SKIN1), "SKIN", "blue.ssk");
	m_EditFPS.SetWindowText(_T("5"));
	m_EditFrameCache.SetWindowText(_T("10"));
	m_RecordCursor = TRUE;
	m_Bnpause.EnableWindow(FALSE);
	m_Bnend.EnableWindow(FALSE);

	UpdateData(FALSE);

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CscreenrecorderDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CscreenrecorderDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

afx_msg LRESULT CscreenrecorderDlg::OnHotKey(WPARAM wParam, LPARAM lParam)
{
	switch (wParam)
	{
	case StartRec:
		if (globalvars::rstatus == Stop && m_Bnstart.IsWindowEnabled())
		{
			OnBnClickedBnstart();
		}
		break;
	case PauseRec:
		if (globalvars::rstatus != Stop && m_Bnpause.IsWindowEnabled())
		{
			OnBnClickedBnpause();
		}
		break;
	case EndRec:
		if (globalvars::rstatus != Stop && m_Bnend.IsWindowEnabled())
		{
			OnBnClickedBnend();
		}
		break;
	default:
		break;
	}

	return NULL;
}

int CscreenrecorderDlg::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CDialogEx::OnCreate(lpCreateStruct) == -1)
		return -1;

	RegisterHotKey(m_hWnd, StartRec, 0, VK_F2);
	RegisterHotKey(m_hWnd, PauseRec, 0, VK_F3);
	RegisterHotKey(m_hWnd, EndRec, 0, VK_F4);

	return 0;
}


void CscreenrecorderDlg::OnDestroy()
{
	CDialogEx::OnDestroy();

	UnregisterHotKey(m_hWnd, StartRec);
	UnregisterHotKey(m_hWnd, PauseRec);
	UnregisterHotKey(m_hWnd, EndRec);
}

void CscreenrecorderDlg::OnBnClickedBnstart()
{
	UpdateData();
	CString text;
	m_EditFPS.GetWindowTextW(text);
	int tmpint = _ttoi(text);
	if (tmpint <= 0)
	{
		MessageBox(_T("FPS值必须为正数"), _T("错误"), MB_OK | MB_ICONERROR);
		return;
	}
	GetFPS() = static_cast<uint>(tmpint);

	m_EditFrameCache.GetWindowTextW(text);
	tmpint = _ttoi(text);
	if (tmpint <= 0)
	{
		MessageBox(_T("帧缓存值必须为正数"), _T("错误"), MB_OK | MB_ICONERROR);
		return;
	}
	globalvars::FrameCache = static_cast<uint>(tmpint);

	m_EditFilename.GetWindowText(text);
	if (!isFileAvailable(static_cast<tstring>(text)))
	{
		MessageBox(_T("文件路径无效"), _T("错误"), MB_OK | MB_ICONERROR);
		return;
	}

	GetDesktopWindow()->GetClientRect(&globalvars::scrrect);

	globalvars::bKeepCursor = m_RecordCursor == TRUE;

	m_EditFilename.EnableWindow(FALSE);
	m_EditFPS.EnableWindow(FALSE);
	m_EditFrameCache.EnableWindow(FALSE);
	m_Bnstart.EnableWindow(FALSE);
	m_Bnpause.EnableWindow(TRUE);
	m_Bnend.EnableWindow(TRUE);
	m_Bnselectfile.EnableWindow(FALSE);
	m_Ckcursor.EnableWindow(FALSE);
	m_Ckrecordaudio.EnableWindow(FALSE);

	globalvars::rstatus = Record;
	globalvars::tFrame = 0u;

	// 默认压缩系数是7500，暂时不允许修改
	// 范围为0到10000，10000为不压缩
	// 7500足够了【确信
	InitAVI(globalvars::hDCtmp, m_hWnd, text, 7500ul, m_Ckrecordaudio.GetCheck() > 0);

	SetTimer(Rec, static_cast<UINT>(1000.0f / GetFPS()), NULL);
}


void CscreenrecorderDlg::OnBnClickedCktopmost()
{
	UpdateData();
	if (m_TopMost)
	{
		SetWindowPos(FromHandle(HWND_TOPMOST), 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
	}
	else
	{
		SetWindowPos(FromHandle(HWND_NOTOPMOST), 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
	}
}


void CscreenrecorderDlg::OnTimer(UINT_PTR nIDEvent)
{
	try
	{
		if (nIDEvent == Rec)
		{
			switch (globalvars::rstatus)
			{
			case Pause:
				KillTimer(Rec);
				SaveAVI();
				m_Bnpause.SetWindowText(_T("继续录制(F3)"));
				m_Bnstart.EnableWindow(FALSE);
				m_Bnpause.EnableWindow(TRUE);
				m_Bnend.EnableWindow(TRUE);
				m_Ckcursor.EnableWindow(TRUE);
				break;
			case Stop:
			{
				KillTimer(Rec);
				SaveAVI();
				m_Bnpause.SetWindowText(_T("暂停录制(F3)"));
				FinishAVI();
				CString tmp;
				m_EditFilename.GetWindowTextW(tmp);
				MessageBox(_T("录制完毕！文件保存在\n") + tmp, _T("提示"), MB_OK | MB_ICONINFORMATION);
				m_Bnstart.EnableWindow(TRUE);
				m_Bnpause.EnableWindow(FALSE);
				m_Bnend.EnableWindow(FALSE);
				m_Bnselectfile.EnableWindow(TRUE);
				m_Ckcursor.EnableWindow(TRUE);
				m_Ckrecordaudio.EnableWindow(TRUE);
				m_EditFilename.EnableWindow(TRUE);
				m_EditFPS.EnableWindow(TRUE);
				m_EditFrameCache.EnableWindow(TRUE);
				break;
			}
			case Record:
			default:
			{
				PushScreenShotStack(GetScreenShot(globalvars::hDCscr, globalvars::hDCtmp, globalvars::scrrect, globalvars::scrrect, globalvars::bKeepCursor));
				++globalvars::tFrame;
				UpdateData();
				std::wstringstream ss;
				ss << globalvars::tFrame;
				m_Lbnframe = ss.str().data();

				UpdateData(FALSE);

				break;
			}
			}

			if (globalvars::tFrame % globalvars::FrameCache == 0u)
			{
				SaveAVI();
			}
		}
	}
	catch (std::exception& e)
	{
		auto err = GetLastError();
		CString msg;
		msg.Format(_T("发生错误：%hs\n错误号：%lu"), e.what(), err);
		MessageBox(msg, _T("错误"), MB_OK | MB_ICONERROR);
		throw;
	}
	catch (...)
	{
		auto err = GetLastError();
		CString msg;
		msg.Format(_T("未知错误\n错误号：%lu"), err);
		MessageBox(msg, _T("错误"), MB_OK | MB_ICONERROR);
		throw;
	}

	CDialogEx::OnTimer(nIDEvent);
}


void CscreenrecorderDlg::OnBnClickedBnselectfile()
{
	CFileDialog fd(FALSE, NULL, NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, _T("AVI文件(*.avi)|*.avi|所有文件(*.*)|*.*||"), AfxGetMainWnd());
	fd.GetOFN().lpstrTitle = _T("保存录像文件");
	fd.ApplyOFNToShellDialog();
	auto result = fd.DoModal();

	if (result == IDCANCEL)
	{
		return;
	}
	
	CString filename, fileext{ fd.GetFileExt() };
	filename.Format(_T("%s.%s"), fd.GetPathName().GetString(), fileext.IsEmpty() ? _T("avi") : fileext.GetString());
	m_EditFilename.SetWindowText(filename);
	
	UpdateData(FALSE);
}


void CscreenrecorderDlg::OnBnClickedBnpause()
{
	m_Bnstart.EnableWindow(FALSE);
	m_Bnpause.EnableWindow(FALSE);
	m_Bnend.EnableWindow(FALSE);

	if (globalvars::rstatus == Pause)
	{
		UpdateData();
		globalvars::rstatus = Record;
		globalvars::bKeepCursor = m_RecordCursor == TRUE;
		m_Bnpause.SetWindowText(_T("暂停录制(F3)"));
		m_Bnstart.EnableWindow(FALSE);
		m_Bnpause.EnableWindow(TRUE);
		m_Bnend.EnableWindow(TRUE);
		m_Ckcursor.EnableWindow(FALSE);
		SetTimer(Rec, static_cast<UINT>(1000.0f / GetFPS()), nullptr);
	}
	else
	{
		globalvars::rstatus = Pause;
	}
}


void CscreenrecorderDlg::OnBnClickedBnend()
{
	try
	{
		if (globalvars::tFrame > 0)
		{
			m_Bnstart.EnableWindow(FALSE);
			m_Bnpause.EnableWindow(FALSE);
			m_Bnend.EnableWindow(FALSE);

			if (globalvars::rstatus == Pause)
			{
				KillTimer(Rec);
				m_Bnpause.SetWindowText(_T("暂停录制(F3)"));
				SaveAVI();
				FinishAVI();
				CString tmp;
				m_EditFilename.GetWindowTextW(tmp);
				MessageBox(_T("录制完毕！文件保存在\n") + tmp, _T("提示"), MB_OK | MB_ICONINFORMATION);
				m_Bnstart.EnableWindow(TRUE);
				m_Bnpause.EnableWindow(FALSE);
				m_Bnend.EnableWindow(FALSE);
				m_Bnselectfile.EnableWindow(TRUE);
				m_Ckcursor.EnableWindow(TRUE);
				m_Ckrecordaudio.EnableWindow(TRUE);
				m_EditFilename.EnableWindow(TRUE);
				m_EditFPS.EnableWindow(TRUE);
				m_EditFrameCache.EnableWindow(TRUE);
			}
			globalvars::rstatus = Stop;
		}
	}
	catch (std::exception& e)
	{
		auto err = GetLastError();
		CString msg;
		msg.Format(_T("发生错误：%hs\n错误号：%lu"), e.what(), err);
		MessageBox(msg, _T("错误"), MB_OK | MB_ICONERROR);
		throw;
	}
	catch (...)
	{
		auto err = GetLastError();
		CString msg;
		msg.Format(_T("未知错误\n错误号：%lu"), err);
		MessageBox(msg, _T("错误"), MB_OK | MB_ICONERROR);
		throw;
	}
}


afx_msg LRESULT CscreenrecorderDlg::OnWimData(WPARAM wParam, LPARAM lParam)
{
	try
	{
		if (globalvars::rstatus == Record)
		{
			WimData(wParam, lParam);
		}
	}
	catch (std::exception& e)
	{
		auto err = GetLastError();
		CString msg;
		msg.Format(_T("发生错误：%hs\n错误号：%lu"), e.what(), err);
		MessageBox(msg, _T("错误"), MB_OK | MB_ICONERROR);
		throw;
	}
	catch (...)
	{
		auto err = GetLastError();
		CString msg;
		msg.Format(_T("未知错误\n错误号：%lu"), err);
		MessageBox(msg, _T("错误"), MB_OK | MB_ICONERROR);
		throw;
	}

	return 0;
}


afx_msg LRESULT CscreenrecorderDlg::OnWimOpen(WPARAM wParam, LPARAM lParam)
{
	try
	{
		WimOpen();
	}
	catch (std::exception& e)
	{
		auto err = GetLastError();
		CString msg;
		msg.Format(_T("发生错误：%hs\n错误号：%lu"), e.what(), err);
		MessageBox(msg, _T("错误"), MB_OK | MB_ICONERROR);
		throw;
	}
	catch (...)
	{
		auto err = GetLastError();
		CString msg;
		msg.Format(_T("未知错误\n错误号：%lu"), err);
		MessageBox(msg, _T("错误"), MB_OK | MB_ICONERROR);
		throw;
	}

	return 0;
}


afx_msg LRESULT CscreenrecorderDlg::OnWimClose(WPARAM wParam, LPARAM lParam)
{
	try
	{
		WimClose();
	}
	catch (std::exception& e)
	{
		auto err = GetLastError();
		CString msg;
		msg.Format(_T("发生错误：%hs\n错误号：%lu"), e.what(), err);
		MessageBox(msg, _T("错误"), MB_OK | MB_ICONERROR);
		throw;
	}
	catch (...)
	{
		auto err = GetLastError();
		CString msg;
		msg.Format(_T("未知错误\n错误号：%lu"), err);
		MessageBox(msg, _T("错误"), MB_OK | MB_ICONERROR);
		throw;
	}
	
	return 0;
}
