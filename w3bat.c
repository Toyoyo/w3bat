/*  W3Battery
 *  Copyright (c) 2022 Toyoyo
 *
 *  This program is free software. It comes without any warranty, to
 *  the extent permitted by applicable law. You can redistribute it
 *  and/or modify it under the terms of the Do What the Fuck You Want
 *  to Public License, Version 2, as published by Sam Hocevar. See
 *  http://www.wtfpl.net/ for more details.
*/

#include <windows.h>
#include <dos.h>
#include <stdio.h>
#include <ole2.h>

#define W3BATVERSION "0.1.0"

/* Global variable, since I'm lazy */
HINSTANCE g_hInstance = NULL;
HBRUSH hBrushMain;
HBRUSH hBrushGreen;
HBRUSH hBrushBlack;
HBRUSH hBrushYellow;
HBRUSH hBrushRed;
HPEN hPenWhite;
HPEN hPenBlack;
HWND hWnd;
HWND hACStatus;
HFONT s_hFont;
char sTitle[5];
char sACStatus[13];
int iCharge=0;
int iStatus=0;
int iCharging=0;
static const char MainWndClass[] = "W3Bat";

/* Function declarations */

/* AH=53h AL=00h BX=0000h -> INT15h
 * 16-bit real-mode call
 * AH: APM Major version
 * AL: APM Minor version */
void APMVersion(int* major, int* minor) {
  union REGS r;

  r.h.ah = 0x53;
  r.h.al = 0x00;
  r.w.bx = 0x0000;
  int86(0x15, &r, &r);
  *major = r.h.ah;
  *minor = r.h.al;

  if(r.w.cflag & INTR_CF) *minor=0;
}

/* AH=53h AL=0Ah BX=0001h -> INT15h
 * 16-bit real-mode call *
 * BX=80XXh could be used for multiple batteries
 * BH: AC line status
 * CL: Remaining battery life (percent)
 * CH: Battery flag, charging state is bit 3 */
void GetBattery(int* Charge, int* ACStatus, int* Charging) {
  union REGS r;
  unsigned mask;

  r.h.ah = 0x53;
  r.h.al = 0x0A;
  r.w.bx = 0x0001;
  int86(0x15, &r, &r);

  *Charging = r.h.ch;
  *Charge = r.h.cl;
  *ACStatus = r.h.bh;

  if(*Charge > 100) *Charge=100;
}

void ClientResize(HWND hWnd, int nWidth, int nHeight)
{
  /* Thanks, random reddit post */
  RECT rcClient, rcWind;
  POINT ptDiff;
  GetClientRect(hWnd, &rcClient);
  GetWindowRect(hWnd, &rcWind);
  ptDiff.x = (rcWind.right - rcWind.left) - rcClient.right;
  ptDiff.y = (rcWind.bottom - rcWind.top) - rcClient.bottom;
  MoveWindow(hWnd,rcWind.left, rcWind.top, nWidth + ptDiff.x, nHeight + ptDiff.y, TRUE);
}

void UpdateStatus() {
  GetBattery(&iCharge, &iStatus, &iCharging);
}

/* Actual battery indicator drawing
 * Also, title update with percentage */
void UpdateWin() {
  HDC hdc;
  int lCharge;

  lCharge=(iCharging >> 3) & 1;

  snprintf(&sTitle, 5, "%d%%", iCharge);
  if(iStatus==1) {
    if(lCharge==1) {
    snprintf(&sACStatus, 13, "AC: Charging");
    } else {
    snprintf(&sACStatus, 7, "AC: On");
    }
  } else {
    snprintf(&sACStatus, 8, "AC: Off");
  }

  SetWindowText(hWnd, sTitle);

  if(!IsIconic(hWnd)) {
    SetWindowText(hACStatus, sACStatus);
    hdc = GetDC(hWnd);

    SelectObject(hdc, hBrushMain);
    SelectObject(hdc, hPenWhite);
    Rectangle(hdc, -1, -1, 200, 18);

    SelectObject(hdc, hBrushBlack);
    SelectObject(hdc, hPenBlack);
    Rectangle(hdc, 1, 1, 100, 16);

    SelectObject(hdc, hBrushGreen);
    if(iCharge < 50) SelectObject(hdc, hBrushYellow);
    if(iCharge < 25) SelectObject(hdc, hBrushRed);
    Rectangle(hdc, 1, 1, iCharge, 16);
    ReleaseDC(hWnd, hdc);
  }
}

void ShowAboutDlg() {
  char sAboutMsg[256];
  int iMajor;
  int iMinor;

  APMVersion(&iMajor, &iMinor);
  snprintf(&sAboutMsg, 255, "W3Battery %s\r\n(c) 2022 Toyoyo\r\n\r\nBIOS APM Version: %d.%d", W3BATVERSION, iMajor, iMinor);
  MessageBox(NULL, sAboutMsg, "About", MB_OK|MB_ICONINFORMATION|MB_TASKMODAL);
}

/* APM BIOS Call only on time event */
void CALLBACK Timer0Proc(HWND hWnd, unsigned int msg, unsigned int idTimer, DWORD dwTime)
{
  UpdateStatus();
  UpdateWin();
}

LRESULT CALLBACK MainWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  switch (msg)
  {
    case WM_CREATE:
    {
      hACStatus = CreateWindowEx(0,"Static","AC: ???", WS_CHILD|WS_VISIBLE, 0, 18, 110, 16, hWnd,(HMENU)5000,g_hInstance,0);
      SendMessage(hACStatus, WM_SETFONT, (WPARAM)s_hFont, (LPARAM)MAKELONG(TRUE, 0));
      UpdateStatus();
      return 0;
    }

    case WM_COMMAND:
    {
      return DefWindowProc(hWnd, msg, wParam, lParam);
    }
    case WM_SIZE:
    {
      if(LOWORD(wParam) == 0) {
        UpdateWin();
      }
    }

    case WM_ERASEBKGND:
    {
      UpdateWin();
      return 0;
    }
    case WM_KEYUP:
    {
        if(LOWORD(wParam) == 0x70) ShowAboutDlg();
    }

    case WM_CTLCOLOR:
    {
      int param=lParam;
      if(param == hACStatus) {
        return (HBRUSH)hBrushMain;
      }

      SetBkMode((HDC)wParam,TRANSPARENT);
      return DefWindowProc(hWnd, msg, wParam, lParam);
    }

    case WM_DESTROY:
    {
      KillTimer(hWnd, 1001);
      PostQuitMessage(0);
    }
    default:
      return DefWindowProc(hWnd, msg, wParam, lParam);
  }
  return 0;
}

/* Register a class for our main window */
BOOL RegisterMainWindowClass()
{
  WNDCLASS wc = {0};

  wc.lpfnWndProc   = &MainWndProc;
  wc.hInstance     = g_hInstance;
  wc.lpszClassName = MainWndClass;
  wc.hbrBackground = hBrushMain;
  wc.hIcon = LoadIcon(g_hInstance, MAKEINTRESOURCE(1));
  wc.hCursor       = LoadCursor(NULL, IDC_ARROW);

  return (RegisterClass(&wc)) ? TRUE : FALSE;
}

/* Create an instance of our main window */
HWND CreateMainWindow()
{
  int iXPos = GetPrivateProfileInt("W3Bat", "x", 0, ".\\w3bat.ini");
  int iYPos = GetPrivateProfileInt("W3Bat", "y", 0, ".\\w3bat.ini");

  hWnd = CreateWindowEx(0, MainWndClass, "???%", WS_OVERLAPPED | WS_SYSMENU | WS_CAPTION | WS_MINIMIZEBOX, iXPos, iYPos,
                        103, 60, NULL, NULL, g_hInstance, NULL);
  ClientResize(hWnd, 101, 32);

  return hWnd;
}

int PASCAL WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
  int iMajor;
  int iMinor;
  int CfgFontSize;
  char CfgFont[1024];

  HWND hWnd;
  MSG msg;

  hBrushMain = CreateSolidBrush(RGB(255,255,255));
  hBrushGreen = CreateSolidBrush(RGB(0,205,0));
  hBrushBlack = CreateSolidBrush(RGB(0,0,0));
  hBrushYellow = CreateSolidBrush(RGB(205,205,0));
  hBrushRed = CreateSolidBrush(RGB(205,0,0));
  hPenWhite = CreatePen(PS_SOLID, 0, RGB(255,255,255));
  hPenBlack = CreatePen(PS_SOLID, 0, RGB(0,0,0));

  GetPrivateProfileString("W3Bat", "font", "Courier", CfgFont, 1023, ".\\w3bat.ini");
  CfgFontSize = GetPrivateProfileInt("W3Bat", "size", 10, ".\\w3bat.ini");
  s_hFont = CreateFont(CfgFontSize, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, CfgFont);

  g_hInstance = hInstance;

  /* z3dc compatibility */
  OleInitialize(NULL);

  APMVersion(&iMajor, &iMinor);
  if(iMajor == 0) {
    MessageBox(NULL, "APM Not supported.", "Error", MB_ICONHAND | MB_OK);
    goto cleanup;
  }

  /* Register our main window class */
  if (!hPrevInstance)
  {
    if (!RegisterMainWindowClass())
    {
      MessageBox(NULL, "Error registering main window class.", "Error", MB_ICONHAND | MB_OK);
      return 0;
    }
  }

  /* Create our main window */
  if (!(hWnd = CreateMainWindow()))
  {
    MessageBox(NULL, "Error creating main window.", "Error", MB_ICONHAND | MB_OK);
    return 0;
  }

  SetTimer(hWnd, 1001, 1000, (TIMERPROC) Timer0Proc);

  /* Sh!ow main window and force a paint */
  ShowWindow(hWnd, nCmdShow);
  UpdateWindow(hWnd);

  while(GetMessage(&msg, NULL, 0, 0) > 0)
  {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

cleanup:
  DeleteObject(hBrushMain);
  DeleteObject(hBrushGreen);
  DeleteObject(hBrushBlack);
  DeleteObject(hBrushYellow);
  DeleteObject(hBrushRed);
  DeleteObject(hPenWhite);
  DeleteObject(hPenBlack);
  DeleteObject(s_hFont);
  UnregisterClass("W3Bat", g_hInstance);

  return(0);
}
