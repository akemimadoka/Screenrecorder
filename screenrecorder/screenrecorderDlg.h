
// screenrecorderDlg.h : ͷ�ļ�
//

#pragma once
#include "afxwin.h"

enum MyHotkey
{
	StartRec = 1001,
	PauseRec = 1002,
	EndRec = 1003
};

enum MyTimer
{
	Rec = 1
};

enum RecStatus
{
	Record,
	Pause,
	Stop
};

// CscreenrecorderDlg �Ի���
class CscreenrecorderDlg : public CDialogEx
{
// ����
public:
	CscreenrecorderDlg(CWnd* pParent = NULL);	// ��׼���캯��

// �Ի�������
	enum { IDD = IDD_MFCTEST2_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV ֧��


// ʵ��
protected:
	HICON m_hIcon;

	// ���ɵ���Ϣӳ�亯��
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();

	DECLARE_MESSAGE_MAP()
public:
	afx_msg LRESULT OnHotKey(WPARAM wParam, LPARAM lParam);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	afx_msg void OnBnClickedBnstart();
	BOOL m_TopMost;
	afx_msg void OnBnClickedCktopmost();
	BOOL m_RecordCursor;
	CEdit m_EditFPS;
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnBnClickedBnselectfile();
	CEdit m_EditFilename;
	afx_msg void OnBnClickedBnpause();
	CButton m_Bnstart;
	CButton m_Bnpause;
	CButton m_Bnend;
	CButton m_Ckcursor;
	afx_msg void OnBnClickedBnend();
	CString m_Lbnframe;
	CButton m_Bnselectfile;
	CEdit m_EditFrameCache;
protected:
	afx_msg LRESULT OnWimData(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnWimOpen(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnWimClose(WPARAM wParam, LPARAM lParam);
public:
	CButton m_Ckrecordaudio;
};
