#include "stdafx.h"

#include "screenshot.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>
#include <Windows.h>

#include <Vfw.h>
#include <mmsystem.h>
#include <memory>
#include <wingdi.h>
#pragma comment(lib, "Vfw32.lib")
#pragma comment(lib, "winmm.lib")

// 全局变量
namespace globalvars
{
	// 录像缓冲区
	std::vector<HBITMAP> ScreenShotStack;
	// 当前帧数
	uint nFrames = 0u;
	// 像素信息缓冲
	std::vector<byte> buf;

	// AVI视频流信息
	AVISTREAMINFO avsi;
	// AVI音频流信息
	AVISTREAMINFO aasi;
	// AVI文件指针
	PAVIFILE pfile;
	// AVI视频流
	PAVISTREAM ps;
	// AVI音频流
	PAVISTREAM pas;
	// AVI压缩流
	PAVISTREAM pcoms;

	// AVI压缩选项
	AVICOMPRESSOPTIONS CompressOption;

	// 位图信息指针
	BITMAPINFO *pBInfo;

	// 临时设备上下文
	HDC tmphDC;

	LONG AudioStreamPos;
	WAVEFORMATEX AudioFormat;
	DWORD AudioBufferSize = 22050UL;
	HWAVEIN	hMic;
	UINT AudioDeviceID = WAVE_MAPPER;
	DWORD adwAudioFormat = WAVE_FORMAT_2M16;

	// 立体声
	std::unique_ptr<WAVEHDR> pWaveHeader[2];
	std::vector<BYTE> pAudioBuffer[2];

	bool RecordAudio = false;

	// fps
	uint fps = 5u;
}

HBITMAP GetScreenShot(HDC hDCscr, HDC hDCout, RECT const& scrrect, RECT const& outrect, bool bKeepcursor)
{
	HBITMAP mbmp = CreateCompatibleBitmap(hDCscr, outrect.right - outrect.left, outrect.bottom - outrect.top);
	if (mbmp == nullptr)
	{
		throw _T("无法创建位图");
	}
	if (SelectObject(hDCout, mbmp) == nullptr)
	{
		throw _T("无法选中对象");
	}
	BITMAP mbitmap;
	if (!GetObject(mbmp, sizeof(BITMAP), &mbitmap))
	{
		throw _T("无法获得位图信息");
	}
	if (!StretchBlt(hDCout, 0, 0, mbitmap.bmWidth, mbitmap.bmHeight, hDCscr, 0, 0, scrrect.right - scrrect.left, scrrect.bottom - scrrect.top, SRCCOPY))
	{
		throw _T("位块转换或传送到目标设备上下文失败");
	}

	if (bKeepcursor)
	{
		POINT cpos;
		if (!GetCursorPos(&cpos))
		{
			throw _T("无法获得鼠标位置");
		}
		HICON cicon = GetCursor();
		if (!DrawIcon(hDCout, cpos.x, cpos.y, cicon))
		{
			throw _T("绘制鼠标时发生错误");
		}
	}

	return mbmp;
}

uint& GetFPS()
{
	return globalvars::fps;
}

uint GetnFrames()
{
	return globalvars::nFrames;
}

void PushScreenShotStack(HBITMAP const& ss)
{
	globalvars::ScreenShotStack.push_back(ss);
}

bool CanRecordAudio(DWORD format)
{
	WAVEINCAPS twic;
	waveInGetDevCaps(globalvars::AudioDeviceID, &twic, sizeof(twic));

	return ((twic.dwFormats & format) != (DWORD)NULL);
}

bool CreateAudioStream()
{
	globalvars::aasi.fccType = streamtypeAUDIO;
	globalvars::aasi.dwScale = globalvars::AudioFormat.nChannels;
	globalvars::aasi.dwRate = globalvars::AudioFormat.nSamplesPerSec;
	globalvars::aasi.dwSampleSize = globalvars::AudioFormat.nBlockAlign;
	globalvars::aasi.dwSuggestedBufferSize = globalvars::AudioFormat.nSamplesPerSec;

	return (AVIFileCreateStream(globalvars::pfile, &globalvars::pas, &globalvars::aasi) == AVIERR_OK);
}

bool CreateAudioBuffer(DWORD size)
{
	if (size <= 0UL)
	{
		return false;
	}

	if (globalvars::hMic == NULL)
	{
		return false;
	}

	for (uint i = 0; i < 2; ++i)
	{
		globalvars::pWaveHeader[i] = std::make_unique<WAVEHDR>();
		globalvars::pAudioBuffer[i].resize(size * globalvars::AudioFormat.nBlockAlign);

		memset(globalvars::pWaveHeader[i].get(), 0, sizeof(WAVEHDR));
		globalvars::pWaveHeader[i]->lpData = reinterpret_cast<LPSTR>(globalvars::pAudioBuffer[i].data());
		globalvars::pWaveHeader[i]->dwBufferLength = size * globalvars::AudioFormat.nBlockAlign;

		if (waveInPrepareHeader(globalvars::hMic, globalvars::pWaveHeader[i].get(), sizeof(WAVEHDR)) != MMSYSERR_NOERROR)
		{
			globalvars::pWaveHeader[i].reset();
			globalvars::pAudioBuffer[i].clear();
			throw _T("无法准备音频头");
		}

		if (waveInAddBuffer(globalvars::hMic,globalvars::pWaveHeader[i].get(), sizeof(WAVEHDR)) != MMSYSERR_NOERROR)
		{
			globalvars::pWaveHeader[i].reset();
			globalvars::pAudioBuffer[i].clear();
			throw _T("无法在缓存头部添加音频头");
		}
	}

	return true;
}

bool DeleteAudioBuffer()
{
	for (uint i = 0; i < 2; ++i)
	{
		if (globalvars::pWaveHeader[i] != nullptr)
		{
			globalvars::pWaveHeader[i].reset();
		}
		else
		{
			return false;
		}
	}

	return true;
}

// 使用默认设置初始化AVI流
// 可能以后会提供配置文件
void InitAVI(HDC hDCout, HWND hWnd, LPCTSTR filename, DWORD quality, bool recordaudio)
{
	AVIFileInit();
	AVIFileOpen(&globalvars::pfile, filename, OF_WRITE | OF_CREATE, NULL);
	memset(&globalvars::avsi, 0, sizeof(globalvars::avsi));
	memset(&globalvars::aasi, 0, sizeof(globalvars::aasi));

	globalvars::avsi.fccType = streamtypeVIDEO;
	globalvars::avsi.fccHandler = 0;
	globalvars::avsi.dwScale = 1;
	globalvars::avsi.dwRate = globalvars::fps; // fps

	memset(&globalvars::CompressOption, 0, sizeof(globalvars::CompressOption));
	globalvars::CompressOption.fccType = streamtypeVIDEO;
	globalvars::CompressOption.fccHandler = mmioStringToFOURCC(_T("MSVC"), 0);
	globalvars::CompressOption.dwQuality = quality;
	globalvars::CompressOption.dwBytesPerSecond = 0;
	globalvars::CompressOption.dwFlags = AVICOMPRESSF_VALID | AVICOMPRESSF_KEYFRAMES;
	globalvars::CompressOption.lpFormat = 0;
	globalvars::CompressOption.cbFormat = 0;
	globalvars::CompressOption.dwInterleaveEvery = 0;

	globalvars::RecordAudio = recordaudio;
	if (globalvars::RecordAudio)
	{
		memset(&globalvars::AudioFormat, 0, sizeof globalvars::AudioFormat);
		globalvars::AudioFormat.wFormatTag = WAVE_FORMAT_PCM;
		globalvars::AudioFormat.wBitsPerSample = 8;
		globalvars::AudioFormat.nChannels = 2;
		globalvars::AudioFormat.nSamplesPerSec = globalvars::AudioBufferSize;
		globalvars::AudioFormat.nBlockAlign = globalvars::AudioFormat.nChannels * (globalvars::AudioFormat.wBitsPerSample / 8);
		globalvars::AudioFormat.nAvgBytesPerSec = globalvars::AudioFormat.nSamplesPerSec * globalvars::AudioFormat.nBlockAlign;

		if (CanRecordAudio(globalvars::adwAudioFormat))
		{
			if (waveInOpen(&globalvars::hMic, globalvars::AudioDeviceID, &globalvars::AudioFormat, (DWORD)hWnd, NULL, CALLBACK_WINDOW) != MMSYSERR_NOERROR)
			{
				throw _T("无法打开设备");
			}

			if (!CreateAudioStream())
			{
				throw _T("无法创建音频流");
			}

			if (AVIStreamSetFormat(globalvars::pas, 0, &globalvars::AudioFormat, sizeof(WAVEFORMATEX) + globalvars::AudioFormat.cbSize) != AVIERR_OK)
			{
				throw _T("无法设置音频流格式");
			}

			waveInStart(globalvars::hMic);
		}
	}

	globalvars::nFrames = 0u;
	globalvars::tmphDC = hDCout;
}

void SaveAVI()
{
	size_t panelsize;

	for (auto&& x : globalvars::ScreenShotStack)
	{
		BITMAP bmp;
		GetObject(x, sizeof(BITMAP), &bmp);

		if (globalvars::nFrames == 0)
		{
			if (bmp.bmBitsPixel < 16)
			{
				panelsize = static_cast<size_t>(pow(2, bmp.bmBitsPixel*sizeof(RGBQUAD)));
			}
			globalvars::pBInfo = static_cast<BITMAPINFO*>(malloc(sizeof(BITMAPINFO) + panelsize));
			memset(globalvars::pBInfo, 0, sizeof(globalvars::pBInfo));

			globalvars::pBInfo->bmiHeader.biBitCount = bmp.bmBitsPixel;
			globalvars::pBInfo->bmiHeader.biClrImportant = 0;
			globalvars::pBInfo->bmiHeader.biCompression = BI_RGB;
			globalvars::pBInfo->bmiHeader.biWidth = bmp.bmWidth;
			globalvars::pBInfo->bmiHeader.biHeight = bmp.bmHeight;
			globalvars::pBInfo->bmiHeader.biPlanes = bmp.bmPlanes;
			globalvars::pBInfo->bmiHeader.biSize = sizeof(BITMAPINFO);
			globalvars::pBInfo->bmiHeader.biSizeImage = bmp.bmWidthBytes * bmp.bmHeight;
			globalvars::pBInfo->bmiHeader.biXPelsPerMeter = 0;
			globalvars::pBInfo->bmiHeader.biYPelsPerMeter = 0;

			globalvars::buf.clear();
			globalvars::buf.resize(globalvars::pBInfo->bmiHeader.biSizeImage);

			globalvars::avsi.dwSuggestedBufferSize = bmp.bmWidthBytes * bmp.bmHeight;
			SetRect(&globalvars::avsi.rcFrame, 0, 0, bmp.bmWidth, bmp.bmHeight);

			AVIFileCreateStream(globalvars::pfile, &globalvars::ps, &globalvars::avsi);
			AVIMakeCompressedStream(&globalvars::pcoms, globalvars::ps, &globalvars::CompressOption, NULL);
		}

		GetDIBits(globalvars::tmphDC, x, 0, bmp.bmHeight, globalvars::buf.data(), globalvars::pBInfo, DIB_RGB_COLORS);

		AVIStreamSetFormat(globalvars::pcoms, globalvars::nFrames, &globalvars::pBInfo->bmiHeader, sizeof(globalvars::pBInfo->bmiHeader));
		AVIStreamWrite(globalvars::pcoms, globalvars::nFrames, 1, globalvars::buf.data(), globalvars::pBInfo->bmiHeader.biSizeImage, AVIIF_KEYFRAME, NULL, NULL);

		DeleteObject(x);
		++globalvars::nFrames;
	}

	globalvars::ScreenShotStack.clear();
}

void FinishAVI()
{
	if (globalvars::RecordAudio)
	{
		waveInStop(globalvars::hMic);
		waveInReset(globalvars::hMic);
		waveInClose(globalvars::hMic);
	}

	AVIStreamClose(globalvars::pcoms);
	AVIStreamClose(globalvars::ps);
	if (globalvars::RecordAudio)
	{
		AVIStreamClose(globalvars::pas);
	}

//	AVIStreamRelease(globalvars::pcoms);
//	AVIStreamRelease(globalvars::ps);
//	AVIStreamRelease(globalvars::pas);

	if (globalvars::pfile)
	{
		AVIFileRelease(globalvars::pfile);
	}

	if (globalvars::RecordAudio && globalvars::hMic != nullptr)
	{
		waveInReset(globalvars::hMic);
		waveInClose(globalvars::hMic);

		DeleteAudioBuffer();
	}

	globalvars::nFrames = 0u;
	AVIFileExit();
	free(globalvars::pBInfo);
	globalvars::buf.clear();
}

void WimOpen()
{
	CreateAudioBuffer(globalvars::AudioBufferSize);
}

void WimClose()
{
	DeleteAudioBuffer();
}

void WimData(WPARAM wParam, LPARAM lParam)
{
	auto hwi = reinterpret_cast<HWAVEIN>(wParam);
	auto pHdr = reinterpret_cast<LPWAVEHDR>(lParam);
	UINT Samples;

	if (pHdr == nullptr)
	{
		throw _T("音频头为空");
	}

	Samples = pHdr->dwBytesRecorded;
	if (waveInUnprepareHeader(hwi, pHdr, sizeof(WAVEHDR)) != MMSYSERR_NOERROR)
	{
		throw _T("不能取消音频头准备");
	}

	if (AVIStreamWrite(globalvars::pas, globalvars::AudioStreamPos, Samples, pHdr->lpData, pHdr->dwBytesRecorded, AVIIF_KEYFRAME, NULL, NULL) == MMSYSERR_NOERROR)
	{
		globalvars::AudioStreamPos += pHdr->dwBytesRecorded;
	}
	else
	{
		throw _T("无法写入avi流");
	}

	if (waveInPrepareHeader(hwi, pHdr, sizeof(WAVEHDR)) != MMSYSERR_NOERROR)
	{
		throw _T("无法准备音频头");
	}

	if (waveInAddBuffer(hwi, pHdr, sizeof(WAVEHDR)) != MMSYSERR_NOERROR)
	{
		throw _T("无法在缓存头部添加音频头");
	}
}