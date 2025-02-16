#include <windows.h>
#include <vector>
#include <sstream>
#include <fstream>
#include <random>


using namespace std;

LRESULT CALLBACK WinProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// Глобальные переменные для взаимодействия с состоянием игры
int N = 3;
bool isSizeParamSet = false;
vector<vector<int>> stateMatrix(N, vector<int>(N, 0));
COLORREF markingColor = RGB(255, 0, 0);  // Изначальный цвет сетки (красный)
int markingColorChangeSpeed = 5;  // Скорость изменения цвета
COLORREF bgColor = RGB(51, 129, 255); // Начальный цвет фона
HBRUSH bgBrush = CreateSolidBrush(bgColor);
const char* saveFile = "state.txt";

// Функция для изменения цвета плавно
void ChangeGridColor(bool increase) {
	// Извлекаем компоненты цвета
	int r = GetRValue(markingColor);
	int g = GetGValue(markingColor);
	int b = GetBValue(markingColor);

	// Изменяем компоненты цвета
	if (increase) {
		r = (r + markingColorChangeSpeed) % 256;
	}
	else {
		r = (r - markingColorChangeSpeed + 256) % 256;
	}

	// Обновляем цвет
	markingColor = RGB(r, g, b);
}

void SetRandomBgColor() {
	static random_device rd;
	static mt19937 gen(rd());
	static uniform_int_distribution<int> dist(0, 255);
	bgColor = RGB(dist(gen), dist(gen), dist(gen));
}

// Сохранение состояния
void SaveState(HWND hwnd) {
	ofstream file(saveFile);
	if (!file) return;
	RECT rect;
	GetWindowRect(hwnd, &rect);
	file << N << '\n';
	file << (rect.right - rect.left) << ' ' << (rect.bottom - rect.top) << '\n';
	file << bgColor << '\n';
	file << markingColor << '\n';
	for (int i = 0; i < N; ++i) {
		for (int j = 0; j < N; ++j) {
			file << stateMatrix[i][j] << ' ';
		}
		file << '\n';
	}
}

// Загрузка состояния
void LoadState(HWND hwnd) {
	ifstream file(saveFile);
	if (!file) return;
	int width = 320, height = 240; // Значения по умолчанию
	int savedN;
	file >> savedN;
	file >> width >> height;
	file >> bgColor;
	file >> markingColor;
	if (!isSizeParamSet || savedN == N) {
		N = savedN;
		stateMatrix.assign(N, vector<int>(N, 0)); // Обновляем матрицу
		for (int i = 0; i < N; ++i)
			for (int j = 0; j < N; ++j)
				file >> stateMatrix[i][j];
	}
	DeleteObject(bgBrush);
	bgBrush = CreateSolidBrush(bgColor);
	SetClassLongPtr(hwnd, GCLP_HBRBACKGROUND, (LONG_PTR)bgBrush);
	SetWindowPos(hwnd, NULL, 0, 0, width, height, SWP_NOMOVE | SWP_NOZORDER);
	InvalidateRect(hwnd, NULL, TRUE);
}

void updateState(HWND hwnd, int x, int y, int type) {
	RECT rect;
	GetClientRect(GetForegroundWindow(), &rect);
	int cellWidth = rect.right / N;
	int cellHeight = rect.bottom / N;

	int col = x / cellWidth;
	int row = y / cellHeight;

	if (row >= 0 && row < N && col >= 0 && col < N) {
		if (stateMatrix[row][col] != type) {
			stateMatrix[row][col] = type; // 1 - крест, 2 - окружность

			RECT cellRect = {
				col * cellWidth, row * cellHeight,
				(col + 1) * cellWidth, (row + 1) * cellHeight
			};

			InvalidateRect(hwnd, &cellRect, TRUE);
		}
	}
}

void ClearState() {
	for (int i = 0; i < N; ++i)
		for (int j = 0; j < N; ++j)
			stateMatrix[i][j] = 0;
	markingColor = RGB(255, 0, 0);
	DeleteObject(bgBrush);
	bgBrush = CreateSolidBrush(RGB(51, 129, 255));
}

// Создание разметки для поля
void DrawMarking(HWND hwnd, HDC hdc) {
	if (N <= 1) return; // Защита от некорректных значений
	RECT rect;
	GetClientRect(hwnd, &rect);

	int width = rect.right;
	int height = rect.bottom;
	int cellWidth = width / N;
	int cellHeight = height / N;

	HPEN hPen = CreatePen(PS_SOLID, 5, markingColor); // Красные линии
	HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);

	// Вертикальные линии
	for (int i = 1; i < N; ++i) {
		int x = i * cellWidth;
		MoveToEx(hdc, x, 0, NULL);
		LineTo(hdc, x, height);
	}

	// Горизонтальные линии
	for (int i = 1; i < N; ++i) {
		int y = i * cellHeight;
		MoveToEx(hdc, 0, y, NULL);
		LineTo(hdc, width, y);
	}

	SelectObject(hdc, hOldPen);
	DeleteObject(hPen);
}

void DrawMatrix(HWND hwnd, HDC hdc) {
	RECT rect;
	GetClientRect(hwnd, &rect);
	int cellWidth = rect.right / N;
	int cellHeight = rect.bottom / N;

	HPEN hCrossPen = CreatePen(PS_SOLID, 5, RGB(0, 255, 0)); // Крест (зелёный)
	HPEN hCirclePen = CreatePen(PS_SOLID, 5, RGB(255, 50, 255)); // Окружность (фиолетовая)
	HPEN hOldPen;

	for (int row = 0; row < N; ++row) {
		for (int col = 0; col < N; ++col) {
			int x1 = col * cellWidth;
			int y1 = row * cellHeight;
			int x2 = x1 + cellWidth;
			int y2 = y1 + cellHeight;
			int padding = min(cellWidth, cellHeight) / 4;

			if (stateMatrix[row][col] == 1) { // Крест
				hOldPen = (HPEN)SelectObject(hdc, hCrossPen);
				MoveToEx(hdc, x1 + padding, y1 + padding, NULL);
				LineTo(hdc, x2 - padding, y2 - padding);
				MoveToEx(hdc, x2 - padding, y1 + padding, NULL);
				LineTo(hdc, x1 + padding, y2 - padding);
				SelectObject(hdc, hOldPen);
			}
			else if (stateMatrix[row][col] == 2) { // Окружность
				hOldPen = (HPEN)SelectObject(hdc, hCirclePen);
				HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
				Ellipse(hdc, x1 + padding, y1 + padding, x2 - padding, y2 - padding);
				SelectObject(hdc, hOldBrush);
				SelectObject(hdc, hOldPen);
			}
		}
	}
	DeleteObject(hCrossPen);
	DeleteObject(hCirclePen);
}

void initState() {
	stateMatrix.assign(N, vector<int>(N, 0));
}

void parseCmdParams(LPWSTR cmd) {
	wstringstream wss(cmd);
	int newN;
	if (wss >> newN && newN > 1 && newN < 21) { // Ограничение на размер
		N = newN;
		isSizeParamSet = true;
	}
	initState();
}



int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrev, PWSTR pCmdLine, int showWin) {
	parseCmdParams(pCmdLine);
	WNDCLASSEX wc = { 0 };
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.lpfnWndProc = WinProc;
	wc.hInstance = hInstance;
	wc.lpszClassName = L"Tic-tac-toe";
	wc.hbrBackground = bgBrush;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);

	if (!RegisterClassEx(&wc)) {
		return -1;
	}

	HWND hwnd = CreateWindowW(
		L"Tic-tac-toe",
		L"Tic-tac-toe",
		WS_OVERLAPPEDWINDOW,
		100, 100, 320, 240,
		NULL, NULL, hInstance, NULL
	);

	if (!hwnd) return -1;

	ShowWindow(hwnd, showWin);
	UpdateWindow(hwnd);

	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return 0;
}

LRESULT CALLBACK WinProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	HDC hdc;
	PAINTSTRUCT ps;

	switch (uMsg) {
	case WM_CREATE:
		LoadState(hwnd);
		//CreateWinMenu(hwnd);
		return 0;

	case WM_COMMAND:
		return 0;

	case WM_LBUTTONDOWN:
		updateState(hwnd, LOWORD(lParam), HIWORD(lParam), 2);
		return 0;

	case WM_RBUTTONDOWN:
		updateState(hwnd, LOWORD(lParam), HIWORD(lParam), 1);
		return 0;

	case WM_MOUSEMOVE:
		return 0;

	case WM_LBUTTONUP:
		return 0;

	case WM_SIZE:
		InvalidateRect(hwnd, NULL, TRUE); // Запросить перерисовку окна
		return 0;

	case WM_PAINT:
		hdc = BeginPaint(hwnd, &ps);
		DrawMarking(hwnd, hdc);
		DrawMatrix(hwnd, hdc);
		EndPaint(hwnd, &ps);
		return 0;

	case WM_KEYDOWN: {
		if (wParam == VK_ESCAPE || (wParam == 'Q' && GetKeyState(VK_CONTROL) < 0)) {
			SaveState(hwnd);
			PostQuitMessage(0); // Закрываем окно
		}
		if ((wParam == 'L') && (GetKeyState(VK_CONTROL) < 0)) {
			ClearState();
			SetClassLongPtr(hwnd, GCLP_HBRBACKGROUND, (LONG_PTR)bgBrush);
			InvalidateRect(hwnd, NULL, TRUE);
		}
		if ((wParam == 'C') && (GetKeyState(VK_SHIFT) < 0)) {
			//system("notepad");
			ShellExecute(NULL, L"open", L"state.txt", NULL, NULL, SW_SHOWNORMAL);
		}
		if (wParam == VK_RETURN) {
			DeleteObject(bgBrush);
			SetRandomBgColor();
			bgBrush = CreateSolidBrush(bgColor); // Новый цвет фона
			SetClassLongPtr(hwnd, GCLP_HBRBACKGROUND, (LONG_PTR)bgBrush);
			InvalidateRect(hwnd, NULL, TRUE); // Перерисовать окно
		}
		return 0;
	}
	case WM_MOUSEWHEEL: {
		short zDelta = GET_WHEEL_DELTA_WPARAM(wParam);

		// Если колесо мыши прокручено вверх, увеличиваем цвет
		if (zDelta > 0) {
			ChangeGridColor(true);
		}
		// Если колесо мыши прокручено вниз, уменьшаем цвет
		else {
			ChangeGridColor(false);
		}

		InvalidateRect(hwnd, NULL, TRUE);  // Запрашиваем перерисовку
		return 0;
	}
	case WM_DESTROY:
		SaveState(hwnd);
		PostQuitMessage(0);
		return 0;

	default:
		return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}
}

