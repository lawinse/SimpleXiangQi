#ifndef GUI_H
#define GUI_H

#include <windows.h>
struct Gui {
	HINSTANCE hInst;
	HWND hWnd;
	HDC hdc, hdcTmp;
	HBITMAP bmpBoard, bmpSelected, bmpPieces[24];
	int sqSelected, mvLast;
	bool isFlipped, isGameOver;
};

extern Gui GUI;

static void TransparentBlt2(HDC hdcDest, int nXOriginDest,
	int nYOriginDest, int nWidthDest, int nHeightDest,
    HDC hdcSrc, int nXOriginSrc, int nYOriginSrc,
    int nWidthSrc, int nHeightSrc, UINT crTransparent);
void DrawTransBmp(HDC hdc, HDC hdcTmp, int xx, int yy, HBITMAP bmp);

static void DrawBoard(HDC hdc);
void  PlaySound(int nResId);
static void MessageBoxMute(LPCSTR txt);

const bool DRAW_SELECTED = true;
static void DrawSquare(int sq, bool isSelected = false);
static void Response();
static void ClickSquare(int sq);
void StartUp();
LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
HBITMAP LoadResBmp(int nResId);

#endif