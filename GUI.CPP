#include "BASE.H"
#include "BOARD.H"
#include "SEARCH.H"


void TransparentBlt2(HDC hdcDest, int nXOriginDest, int nYOriginDest, int nWidthDest, int nHeightDest,
    HDC hdcSrc, int nXOriginSrc, int nYOriginSrc, int nWidthSrc, int nHeightSrc, UINT crTransparent) {
  HDC hImageDC, hMaskDC;
  HBITMAP hOldImageBMP, hImageBMP, hOldMaskBMP, hMaskBMP;

  hImageBMP = CreateCompatibleBitmap(hdcDest, nWidthDest, nHeightDest);
  hMaskBMP = CreateBitmap(nWidthDest, nHeightDest, 1, 1, NULL);
  hImageDC = CreateCompatibleDC(hdcDest);
  hMaskDC = CreateCompatibleDC(hdcDest);
  hOldImageBMP = (HBITMAP) SelectObject(hImageDC, hImageBMP);
  hOldMaskBMP = (HBITMAP) SelectObject(hMaskDC, hMaskBMP);

  if (nWidthDest == nWidthSrc && nHeightDest == nHeightSrc) {
    BitBlt(hImageDC, 0, 0, nWidthDest, nHeightDest,
        hdcSrc, nXOriginSrc, nYOriginSrc, SRCCOPY);
  } else {
    StretchBlt(hImageDC, 0, 0, nWidthDest, nHeightDest,
        hdcSrc, nXOriginSrc, nYOriginSrc, nWidthSrc, nHeightSrc, SRCCOPY);
  }
  SetBkColor(hImageDC, crTransparent);
  BitBlt(hMaskDC, 0, 0, nWidthDest, nHeightDest, hImageDC, 0, 0, SRCCOPY);
  SetBkColor(hImageDC, RGB(0,0,0));
  SetTextColor(hImageDC, RGB(255,255,255));
  BitBlt(hImageDC, 0, 0, nWidthDest, nHeightDest, hMaskDC, 0, 0, SRCAND);
  SetBkColor(hdcDest, RGB(255,255,255));
  SetTextColor(hdcDest, RGB(0,0,0));
  BitBlt(hdcDest, nXOriginDest, nYOriginDest, nWidthDest, nHeightDest,
      hMaskDC, 0, 0, SRCAND);
  BitBlt(hdcDest, nXOriginDest, nYOriginDest, nWidthDest, nHeightDest,
      hImageDC, 0, 0, SRCPAINT);

  SelectObject(hImageDC, hOldImageBMP);
  DeleteDC(hImageDC);
  SelectObject(hMaskDC, hOldMaskBMP);
  DeleteDC(hMaskDC);
  DeleteObject(hImageBMP);
  DeleteObject(hMaskBMP);
}

inline void DrawTransBmp(HDC hdc, HDC hdcTmp, int xx, int yy, HBITMAP bmp) {
  SelectObject(hdcTmp, bmp);
  TransparentBlt2(hdc, xx, yy, SQUARE_SIZE, SQUARE_SIZE, hdcTmp, 0, 0, SQUARE_SIZE, SQUARE_SIZE, MASK_COLOR);
}

void DrawBoard(HDC hdc) {
	int x,y,xx,yy,sq,pc;
	HDC hdcTmp;

	hdcTmp = CreateCompatibleDC(hdc);
	SelectObject(hdcTmp, GUI.bmpBoard);
	BitBlt(hdc, 0, 0, BOARD_WIDTH, BOARD_HEIGHT, hdcTmp, 0, 0, SRCCOPY);

	for (x = FILE_LEFT; x <= FILE_RIGHT; ++x) {
		for (y = RANK_TOP; y <= RANK_BOTTOM; ++y) {
			if (GUI.isFlipped) {
				xx = BOARD_EDGE + (flipFile(x) - FILE_LEFT) * SQUARE_SIZE;
				yy = BOARD_EDGE + (flipRank(y) - RANK_TOP) * SQUARE_SIZE;
			} else {
				xx = BOARD_EDGE + (x - FILE_LEFT) * SQUARE_SIZE;
				yy = BOARD_EDGE + (y - RANK_TOP) * SQUARE_SIZE;
			}
			sq = getCoordXY(x,y);
			pc = POS.getSquareAt(sq);
			if (pc != 0) {
				DrawTransBmp(hdc, hdcTmp, xx, yy, GUI.bmpPieces[pc]);
			}
			if (sq == GUI.sqSelected || sq == getMoveSrc(GUI.mvLast) || sq == getMoveDst(GUI.mvLast)) {
				DrawTransBmp(hdc, hdcTmp, xx, yy, GUI.bmpSelected);
			}
		}
	}
	DeleteDC(hdcTmp);
}

inline void PlaySound(int nResId) {
	PlaySound(MAKEINTRESOURCE(nResId), GUI.hInst, SND_ASYNC | SND_NOWAIT | SND_RESOURCE);
}

void MessageBoxMute(LPCSTR txt) {
	MSGBOXPARAMS mbp;
	mbp.cbSize = sizeof(MSGBOXPARAMS);
	mbp.hwndOwner = GUI.hWnd;
	mbp.hInstance = NULL;
	mbp.lpszText = txt;
	mbp.lpszCaption = "SimpleXQ";
	mbp.dwStyle = MB_USERICON;
	mbp.lpszIcon = MAKEINTRESOURCE(IDI_INFORMATION);
	mbp.dwContextHelpId = 0;
	mbp.lpfnMsgBoxCallback = NULL;
	mbp.dwLanguageId = 0;

	if (MessageBoxIndirect(&mbp) == 0) {
		mbp.hInstance = GUI.hInst;
		mbp.lpszIcon = MAKEINTRESOURCE(IDI_APPICON);
		MessageBoxIndirect(&mbp);
	}
}


void DrawSquare(int sq, bool isSelected) {
	int sqFlipped, xx, yy, pc;

	sqFlipped = GUI.isFlipped ? flipSquare(sq) : sq;
	xx = BOARD_EDGE + (getFileX(sqFlipped) - FILE_LEFT) * SQUARE_SIZE;
	yy = BOARD_EDGE + (getRankY(sqFlipped) - RANK_TOP) * SQUARE_SIZE;
	SelectObject(GUI.hdcTmp, GUI.bmpBoard);
	BitBlt(GUI.hdc,xx,yy,SQUARE_SIZE,SQUARE_SIZE,GUI.hdcTmp,xx,yy,SRCCOPY);
	pc = POS.getSquareAt(sq);
	if (pc != 0)
		DrawTransBmp(GUI.hdc, GUI.hdcTmp, xx, yy, GUI.bmpPieces[pc]);
	if (isSelected)
		DrawTransBmp(GUI.hdc, GUI.hdcTmp, xx, yy, GUI.bmpSelected);
}

void Response() {
	int valueRep;
	SetCursor((HCURSOR) LoadImage(NULL, IDC_WAIT, IMAGE_CURSOR, 0, 0, LR_DEFAULTSIZE | LR_SHARED));
	SearchMain();
	SetCursor((HCURSOR) LoadImage(NULL, IDC_ARROW, IMAGE_CURSOR, 0, 0, LR_DEFAULTSIZE | LR_SHARED));
	POS.makeMove(SEARCH.mvResult);

	DrawSquare(getMoveSrc(GUI.mvLast));
	DrawSquare(getMoveDst(GUI.mvLast));

	GUI.mvLast = SEARCH.mvResult;
	DrawSquare(getMoveSrc(GUI.mvLast), DRAW_SELECTED);
	DrawSquare(getMoveDst(GUI.mvLast), DRAW_SELECTED);

	valueRep = POS.repStatus(3);
	if (POS.isMate()) {
		PlaySound(IDR_LOSS);
		MessageBoxMute("Try again!");
		GUI.isGameOver = true;
	} else if (valueRep > 0) {
		valueRep = POS.repValue(valueRep);
		PlaySound(valueRep < -WIN_VALUE ? IDR_LOSS : valueRep > WIN_VALUE ? IDR_WIN : IDR_DRAW);
		MessageBoxMute(valueRep < -WIN_VALUE ? "LOSS" : valueRep > WIN_VALUE ? "WIN" : "DRAE");
		GUI.isGameOver = true;
	} else if (POS.getMoveNum() > 100) {
		PlaySound(IDR_DRAW);
		MessageBoxMute("DRAW!!!");
		GUI.isGameOver = true;
	} else {
		PlaySound(POS.isLastInCheck() ? IDR_CHECK2 : POS.isLastCaptured() ? IDR_CAPTURE2 : IDR_MOVE2);
		if (POS.isLastCaptured()) POS.clearHist();
	}
}

void ClickSquare(int sq) {
	int pc, mv, valueRep;
	GUI.hdc = GetDC(GUI.hWnd);
	GUI.hdcTmp = CreateCompatibleDC(GUI.hdc);
	sq = GUI.isFlipped ? flipSquare(sq) : sq;
	pc = POS.getSquareAt(sq);

	if ((pc & getSideTag(POS.getSdPlayer())) != 0) {
		if (GUI.sqSelected != 0) {
			DrawSquare(GUI.sqSelected);
		}
		GUI.sqSelected = sq;
		DrawSquare(sq, DRAW_SELECTED);
		if (GUI.mvLast != 0) {
			DrawSquare(getMoveSrc(GUI.mvLast));
			DrawSquare(getMoveDst(GUI.mvLast));
		}
		PlaySound(IDR_CLICK);
	} else if (GUI.sqSelected != 0 && !GUI.isGameOver) {
		mv = getMove(GUI.sqSelected,sq);
		if (POS.isLegalMove(mv)) {
			if (POS.makeMove(mv)) {
				GUI.mvLast = mv;
				DrawSquare(GUI.sqSelected, DRAW_SELECTED);
				DrawSquare(sq, DRAW_SELECTED);
				GUI.sqSelected = 0;

				valueRep = POS.repStatus(3);
				if (POS.isMate()) {
					PlaySound(IDR_WIN);
					MessageBoxMute("WIN");
					GUI.isGameOver = true;
				} else if (valueRep > 0) {
					valueRep = POS.repValue(valueRep);
					PlaySound(valueRep > WIN_VALUE ? IDR_LOSS : valueRep < -WIN_VALUE ? IDR_WIN : IDR_DRAW);
					MessageBoxMute(valueRep > WIN_VALUE ? "LOSS" : valueRep < -WIN_VALUE ? "WIN" : "DRAW");
					GUI.isGameOver = true;
				} else if (POS.getMoveNum() > 100) {
					PlaySound(IDR_DRAW);
					MessageBoxMute("DRAW!!");
					GUI.isGameOver = true;
				} else {
					PlaySound(POS.isLastInCheck() ? IDR_CHECK : POS.isLastCaptured() ? IDR_CAPTURE : IDR_MOVE);
					if (POS.isLastCaptured()) POS.clearHist();
					Response();
				}
			} else {
				PlaySound(IDR_ILLEGAL);
			}
		}
	}
	DeleteDC(GUI.hdcTmp);
	ReleaseDC(GUI.hWnd, GUI.hdc);
}

void StartUp() {
	POS.startUp();
	GUI.sqSelected = GUI.mvLast = 0;
	GUI.isGameOver = false;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	int x,y;
	HDC hdc;
	RECT rect;
	PAINTSTRUCT ps;
	MSGBOXPARAMS mbp;
	switch (uMsg) {
	case WM_CREATE:
		GetWindowRect(hWnd, &rect);
		x = rect.left;
		y = rect.top;
		rect.right = rect.left + BOARD_WIDTH;
		rect.bottom = rect.top + BOARD_HEIGHT;
		AdjustWindowRect(&rect, WINDOW_STYLES, true);
		MoveWindow(hWnd, x, y, rect.right-rect.left, rect.bottom-rect.top, true);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDM_FILE_RED:
		case IDM_FILE_BLACK:
			GUI.isFlipped = (LOWORD(wParam) == IDM_FILE_BLACK);
			StartUp();
			hdc = GetDC(GUI.hWnd);
			DrawBoard(hdc);
			if (GUI.isFlipped) {
				GUI.hdc = hdc;
				GUI.hdcTmp = CreateCompatibleDC(GUI.hdc);
				Response();
				DeleteDC(GUI.hdcTmp);
			}
			ReleaseDC(GUI.hWnd, hdc);
			break;
		case IDM_FILE_EXIT:
			DestroyWindow(GUI.hWnd);
			break;
		case IDM_HELP_HOME:
			break;
		case IDM_HELP_ABOUT:
			MessageBeep(MB_ICONINFORMATION);
			mbp.cbSize = sizeof(MSGBOXPARAMS);
			mbp.hwndOwner = hWnd;
			mbp.hInstance = GUI.hInst;
			mbp.lpszText = "about";
			mbp.lpszCaption = "ABOUT SimpleXQ";
			mbp.dwStyle = MB_USERICON;
			mbp.lpszIcon = MAKEINTRESOURCE(IDI_APPICON);
			mbp.dwContextHelpId = 0;
			mbp.lpfnMsgBoxCallback = NULL;
			mbp.dwLanguageId = 0;
			MessageBoxIndirect(&mbp);
			break;
		}	
		break;
	case WM_PAINT:
		hdc = BeginPaint(GUI.hWnd, &ps);
		DrawBoard(hdc);
		EndPaint(GUI.hWnd, &ps);
		break;
	case WM_LBUTTONDOWN:
		x = FILE_LEFT + (LOWORD(lParam) - BOARD_EDGE) / SQUARE_SIZE;
		y = RANK_TOP + (HIWORD(lParam) - BOARD_EDGE) / SQUARE_SIZE;
		if (x >=  FILE_LEFT && x<= FILE_RIGHT && y >= RANK_TOP && y <= RANK_BOTTOM) {
			ClickSquare(getCoordXY(x,y));
		}
		break;
	default:
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}
	return false;
}

HBITMAP LoadResBmp(int nResId) {
	return (HBITMAP) LoadImage(GUI.hInst, MAKEINTRESOURCE(nResId), IMAGE_BITMAP, 0, 0, LR_DEFAULTSIZE | LR_SHARED);
}