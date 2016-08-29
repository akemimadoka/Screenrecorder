#pragma once

typedef unsigned int uint;

/*
	���һ����Ļ����
	����˵����
	hDCscr��	Ҫ��ͼ���豸������
	hDCout��	��Ϊ�����ͼ���豸������
	scrrect��	��ͼ��Ļ��Χ
	outrect��	��ͼ�ľ���
	bKeepcursor���ڽ�ͼ�б�����꣬Ĭ��Ϊ����

	����ֵ��	��ͼ��λͼ���
*/
HBITMAP GetScreenShot(HDC hDCscr, HDC hDCout, RECT const& scrrect, RECT const& outrect, bool bKeepcursor = true);

/*
	��ʼ��AVI����
	����˵����
	hDCout��	��ʱ�豸�����ģ�����ָ��ͼ���õ��豸������
	hWnd��		������Ƶ�����Ϣ�Ĵ��ھ��
	filename��	avi�ļ���
	quality��	avi������
	recordaudio���Ƿ�¼������
*/
void InitAVI(HDC hDCout, HWND hWnd, LPCTSTR filename = _T("tmp.avi"), DWORD quality = 7500UL, bool recordaudio = false);

/*
	����λͼ����AVI�ļ�
*/
void SaveAVI();

/*
	���AVI���Ĳ���
*/
void FinishAVI();

/*
	���FPS
*/
uint& GetFPS();

/*
	��õ�ǰ֡��
*/
uint GetnFrames();

/*
	ѹ����Ļ��ͼջ
	����˵����
	ss��	λͼ���
*/
void PushScreenShotStack(HBITMAP ss);

/*
	׼����¼��
*/
void WimOpen();

/*
	׼���ر�¼��
*/
void WimClose();

/*
	����¼������
	����˵����
	wParam��	wParam
	lParam��	lParam
*/
void WimData(WPARAM wParam, LPARAM lParam);