#include <cstdlib>
#include <ctime>
#include "BASE.H"
#include "BOARD.H"
#include "HELPER.H"
#include "SEARCH.H"
#define SIDE_TAG getSideTag

Gui GUI;
Search SEARCH;
Position POS;
ZobStruct ZOBRIST;


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	int i;
	MSG msg;
	WNDCLASSEX wce;

	srand(time(NULL));
	initZOBRIST();
	GUI.hInst = hInstance;
	LoadBook();
	GUI.isFlipped = false;
	StartUp();

	GUI.bmpBoard = LoadResBmp(IDB_BOARD);
	GUI.bmpSelected = LoadResBmp(IDB_SELECTED);
	for (i=PIECE_KING; i<=PIECE_PAWN; ++i) {
		GUI.bmpPieces[getSideTag(0) + i] = LoadResBmp(IDB_RK + i);
		GUI.bmpPieces[getSideTag(1) + i] = LoadResBmp(IDB_BK + i);
	}

	wce.cbSize = sizeof(WNDCLASSEX);
	wce.style = 0;
	wce.lpfnWndProc = (WNDPROC) WndProc;
	wce.cbClsExtra = wce.cbWndExtra = 0;
	wce.hInstance = hInstance;
	wce.hIcon = (HICON) LoadImage(hInstance, MAKEINTRESOURCE(IDI_APPICON), IMAGE_ICON, 32,32,LR_SHARED);
	wce.hCursor = (HCURSOR) LoadImage(NULL, IDC_ARROW, IMAGE_CURSOR, 0, 0, LR_DEFAULTSIZE | LR_SHARED);
	wce.hbrBackground = (HBRUSH) (COLOR_BTNFACE + 1);
	wce.lpszMenuName = MAKEINTRESOURCE(IDM_MAINMENU);
	wce.lpszClassName = "SimpleXQ";
	wce.hIconSm = (HICON) LoadImage(hInstance, MAKEINTRESOURCE(IDI_APPICON), IMAGE_ICON, 16,16,LR_SHARED);
	RegisterClassEx(&wce);

	GUI.hWnd = CreateWindow("SimpleXQ","SimpleXQ",WINDOW_STYLES,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, hInstance, NULL);
	if (GUI.hWnd == NULL) return 0;
	ShowWindow(GUI.hWnd, nCmdShow);
	UpdateWindow(GUI.hWnd);

	while(GetMessage(&msg, NULL, 0 , 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return msg.wParam;
}	

