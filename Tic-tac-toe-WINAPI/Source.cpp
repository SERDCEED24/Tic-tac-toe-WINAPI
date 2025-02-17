#include <windows.h>  // ����������� ���������� Windows API ��� ������ � ������
#include <vector>     // ����������� ���������� ��� ������ � ���������
#include <random>     // ����������� ���������� ��� ��������� ��������� �����

using namespace std;  // ������������� ������������ ������������ ����

// ���������� ������� ��������� ��������� ����
LRESULT CALLBACK WinProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// ���������� ���������� ��� �������������� � ���������� ����
int N = 3;  // ������ �������� ���� (�� ��������� 3x3)
bool isSizeParamSet = false;  // ����, �����������, ��� �� ����� ������ ����� ��������� ��������� ������
vector<vector<int>> stateMatrix(N, vector<int>(N, 0));  // ������� ��������� �������� ����
COLORREF markingColor = RGB(255, 0, 0);  // ����������� ���� ����� (�������)
int markingColorChangeSpeed = 5;  // �������� ��������� ����� �����
COLORREF bgColor = RGB(51, 129, 255);  // ��������� ���� ����
HBRUSH bgBrush = CreateSolidBrush(bgColor);  // ����� ��� ������� ����
const LPCWSTR saveFile = L"state.bin"; // ��� �����, � ������� ����� ����������� ���������

// ������� ��� �������� ��������� ����� �����
void ChangeGridColor(bool increase) {
    // ��������� ���������� ����� (R, G, B)
    int r = GetRValue(markingColor);
    int g = GetGValue(markingColor);
    int b = GetBValue(markingColor);

    // �������� ���������� ����� � ����������� �� ����������� ���������
    if (increase) {
        r = (r + markingColorChangeSpeed) % 256;  // ����������� ������� ��������
    }
    else {
        r = (r - markingColorChangeSpeed + 256) % 256;  // ��������� ������� ��������
    }

    // ��������� ���� �����
    markingColor = RGB(r, g, b);
}

// ������� ��� ��������� ���������� ����� ����
void SetRandomBgColor() {
    static random_device rd;  // ��������� ��������� �����
    static mt19937 gen(rd());  // �������� ��������� ��������� �����
    static uniform_int_distribution<int> dist(0, 255);  // ������������� ��� RGB (0-255)
    bgColor = RGB(dist(gen), dist(gen), dist(gen));  // ��������� ���������� �����
}

void SaveState(HWND hwnd) {
    // ��������� ���� ��� ������ (������� ��� ��������������)
    HANDLE hFile = CreateFile(saveFile, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return; // ���������, ������� �� ������� ����

    RECT rect;
    GetWindowRect(hwnd, &rect); // �������� ���������� ����
    int width = rect.right - rect.left;  // ��������� ������ ����
    int height = rect.bottom - rect.top; // ��������� ������ ����

    DWORD written; // ���������� ��� �������� ���������� ���������� ������

    // ���������� � ���� ���������� ��������� N
    WriteFile(hFile, &N, sizeof(N), &written, NULL);
    // ���������� ������ ����
    WriteFile(hFile, &width, sizeof(width), &written, NULL);
    // ���������� ������ ����
    WriteFile(hFile, &height, sizeof(height), &written, NULL);
    // ���������� ���� ����
    WriteFile(hFile, &bgColor, sizeof(bgColor), &written, NULL);
    // ���������� ���� ���������
    WriteFile(hFile, &markingColor, sizeof(markingColor), &written, NULL);

    // ���������� ������� ��������� (N ����� �� N ���������)
    for (int i = 0; i < N; ++i) {
        WriteFile(hFile, stateMatrix[i].data(), N * sizeof(int), &written, NULL);
    }

    CloseHandle(hFile); // ��������� ����
}

void LoadState(HWND hwnd) {
    // ��������� ���� ��� ������
    HANDLE hFile = CreateFile(saveFile, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return; // ���������, ������� �� ������� ����

    int width = 320, height = 240; // �������� �� ��������� ��� �������� ����
    int savedN; // ��������� ���������� ��� �������� N �� �����
    DWORD read; // ���������� ��� �������� ���������� ����������� ������

    // ������ ���������� ��������� N �� �����
    ReadFile(hFile, &savedN, sizeof(savedN), &read, NULL);
    // ������ ������ ����
    ReadFile(hFile, &width, sizeof(width), &read, NULL);
    // ������ ������ ����
    ReadFile(hFile, &height, sizeof(height), &read, NULL);
    // ������ ���� ����
    ReadFile(hFile, &bgColor, sizeof(bgColor), &read, NULL);
    // ������ ���� ���������
    ReadFile(hFile, &markingColor, sizeof(markingColor), &read, NULL);

    // ���� ������ ������� �� ���������� ��� �� ��������� � �����������, ��������� ���
    if (!isSizeParamSet || savedN == N) {
        N = savedN;
        stateMatrix.assign(N, vector<int>(N, 0)); // ����������� ������� ���������
        // ������ ������� ��������� �� �����
        for (int i = 0; i < N; ++i) {
            ReadFile(hFile, stateMatrix[i].data(), N * sizeof(int), &read, NULL);
        }
    }

    CloseHandle(hFile); // ��������� ����

    // ������� ������ ����� ���� � ������� ����� � ����������� ������
    DeleteObject(bgBrush);
    bgBrush = CreateSolidBrush(bgColor);

    // ������������� ����� ���� ���� ��� ������ ����
    SetClassLongPtr(hwnd, GCLP_HBRBACKGROUND, (LONG_PTR)bgBrush);

    // ������ ������ ���� �� ����������� �� �����
    SetWindowPos(hwnd, NULL, 0, 0, width, height, SWP_NOMOVE | SWP_NOZORDER);

    // �������������� ����, ����� ��������� ���������
    InvalidateRect(hwnd, NULL, TRUE);
}



// ������� ��� ���������� ��������� �������� ����
void updateState(HWND hwnd, int x, int y, int type) {
    RECT rect;
    GetClientRect(GetForegroundWindow(), &rect);  // �������� ������� ���������� ������� ����
    int cellWidth = rect.right / N;  // ������ ����� ������
    int cellHeight = rect.bottom / N;  // ������ ����� ������

    int col = x / cellWidth;  // ���������� ������� ������
    int row = y / cellHeight;  // ���������� ������ ������

    if (row >= 0 && row < N && col >= 0 && col < N) {  // ���������, ��� ������ � �������� ����
        if (stateMatrix[row][col] != type) {  // ���� ��������� ������ ����������
            stateMatrix[row][col] = type;  // ��������� ��������� ������ (1 - �����, 2 - ����������)

            RECT cellRect = {
                col * cellWidth, row * cellHeight,
                (col + 1) * cellWidth, (row + 1) * cellHeight
            };  // ���������� ������� ������

            InvalidateRect(hwnd, &cellRect, TRUE);  // �������������� ������
        }
    }
}

// ������� ��� ������� ��������� ����
void ClearState() {
    for (int i = 0; i < N; ++i)
        for (int j = 0; j < N; ++j)
            stateMatrix[i][j] = 0;  // ������� ������� ���������
    markingColor = RGB(255, 0, 0);  // ���������� ���� �����
    DeleteObject(bgBrush);  // ������� ������ �����
    bgColor = RGB(51, 129, 255); // ������ �������� ���� �� ���������
    bgBrush = CreateSolidBrush(bgColor);  // ������� ����� ����� ��� ���� � ������ �� ���������
}

// ������� ��� ��������� �������� �������� ����
void DrawMarking(HWND hwnd, HDC hdc) {
    if (N <= 1) return;  // ������ �� ������������ ��������
    RECT rect;
    GetClientRect(hwnd, &rect);  // �������� ������� ���������� ������� ����

    int width = rect.right;  // ������ ���������� �������
    int height = rect.bottom;  // ������ ���������� �������
    int cellWidth = width / N;  // ������ ����� ������
    int cellHeight = height / N;  // ������ ����� ������

    HPEN hPen = CreatePen(PS_SOLID, 5, markingColor);  // ������� ���� ��� ��������� �����
    HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);  // �������� ���� � �������� ����������

    // ������ ������������ �����
    for (int i = 1; i < N; ++i) {
        int x = i * cellWidth;
        MoveToEx(hdc, x, 0, NULL);  // ������������ � ������ �����
        LineTo(hdc, x, height);  // ������ �����
    }

    // ������ �������������� �����
    for (int i = 1; i < N; ++i) {
        int y = i * cellHeight;
        MoveToEx(hdc, 0, y, NULL);  // ������������ � ������ �����
        LineTo(hdc, width, y);  // ������ �����
    }

    SelectObject(hdc, hOldPen);  // ��������������� ������ ����
    DeleteObject(hPen);  // ������� ��������� ����
}

// ������� ��� ��������� ����������� �������� ���� (������ � ����������)
void DrawMatrix(HWND hwnd, HDC hdc) {
    RECT rect;
    GetClientRect(hwnd, &rect);  // �������� ������� ���������� ������� ����
    int cellWidth = rect.right / N;  // ������ ����� ������
    int cellHeight = rect.bottom / N;  // ������ ����� ������

    HPEN hCrossPen = CreatePen(PS_SOLID, 5, RGB(0, 255, 0));  // ���� ��� ������� (������)
    HPEN hCirclePen = CreatePen(PS_SOLID, 5, RGB(255, 50, 255));  // ���� ��� ����������� (����������)
    HPEN hOldPen;

    for (int row = 0; row < N; ++row) {
        for (int col = 0; col < N; ++col) {
            int x1 = col * cellWidth;  // ����� ������� ���� ������
            int y1 = row * cellHeight;  // ����� ������� ���� ������
            int x2 = x1 + cellWidth;  // ������ ������ ���� ������
            int y2 = y1 + cellHeight;  // ������ ������ ���� ������
            int padding = min(cellWidth, cellHeight) / 4;  // ������ ��� ��������� �����

            if (stateMatrix[row][col] == 1) {  // ���� � ������ �����
                hOldPen = (HPEN)SelectObject(hdc, hCrossPen);  // �������� ���� ��� ������
                MoveToEx(hdc, x1 + padding, y1 + padding, NULL);  // ������ ������ ����� ������
                LineTo(hdc, x2 - padding, y2 - padding);
                MoveToEx(hdc, x2 - padding, y1 + padding, NULL);  // ������ ������ ����� ������
                LineTo(hdc, x1 + padding, y2 - padding);
                SelectObject(hdc, hOldPen);  // ��������������� ������ ����
            }
            else if (stateMatrix[row][col] == 2) {  // ���� � ������ ����������
                hOldPen = (HPEN)SelectObject(hdc, hCirclePen);  // �������� ���� ��� ����������
                HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));  // ��� �������
                Ellipse(hdc, x1 + padding, y1 + padding, x2 - padding, y2 - padding);  // ������ ����������
                SelectObject(hdc, hOldBrush);  // ��������������� ������ �����
                SelectObject(hdc, hOldPen);  // ��������������� ������ ����
            }
        }
    }
    DeleteObject(hCrossPen);  // ������� ���� ��� �������
    DeleteObject(hCirclePen);  // ������� ���� ��� �����������
}

// ������� ��� ������������� ��������� ����
void initState() {
    stateMatrix.assign(N, vector<int>(N, 0));  // ��������� ������� ������
}

// ������� ��� ��������� ���������� ��������� ������
void parseCmdParams(LPWSTR cmd) {
    int argc;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);  // ��������� ��������� ������

    if (argv && argc > 1) { // ��������, ��� ��������� ���������� (�� NULL) � �� 1 ��� �����
        int newN = _wtoi(argv[1]);  // ������������ ������ �������� � �����
        if (newN > 1 && newN < 21) {  // ��������� ������������ �������
            N = newN;  // ��������� ������ �������� ����
            isSizeParamSet = true;  // ������������� ����
        }
    }

    LocalFree(argv);  // ����������� ������, ���������� CommandLineToArgvW
    initState();  // �������������� ��������� ����
}

// �������� ������� ���������
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrev, PWSTR pCmdLine, int showWin) {
    parseCmdParams(pCmdLine);  // ������������ ��������� ��������� ������
    WNDCLASSEX wc = { 0 };  // ������� ��������� ��� �������� ������ ����
    wc.cbSize = sizeof(WNDCLASSEX);  // ������ ���������
    wc.lpfnWndProc = WinProc;  // ��������� �� ������� ��������� ���������
    wc.hInstance = hInstance;  // ���������� ���������� ����������
    wc.lpszClassName = L"Tic-tac-toe";  // ��� ������ ����
    wc.hbrBackground = bgBrush;  // ����� ��� ���� ����
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);  // ������ �� ���������

    if (!RegisterClassEx(&wc)) {  // ������������ ����� ����
        return -1;  // ���� ����������� �� �������, ��������� ���������
    }

    // ������� ����
    HWND hwnd = CreateWindowW(
        L"Tic-tac-toe",  // ��� ������ ����
        L"Tic-tac-toe",  // ��������� ����
        WS_OVERLAPPEDWINDOW,  // ����� ����
        100, 100, 320, 240,  // ������� � ������� ����
        NULL, NULL, hInstance, NULL  // �������������� ���������
    );

    if (!hwnd) return -1;  // ���� ���� �� ���������, ��������� ���������

    ShowWindow(hwnd, showWin);  // ���������� ����
    UpdateWindow(hwnd);  // ��������� ����

    // �������� ���� ��������� ���������
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);  // ����������� ���������
        DispatchMessage(&msg);  // ���������� ��������� � ������� ���������
    }

    return 0;  // ��������� ���������
}

// ������� ��������� ��������� ����
LRESULT CALLBACK WinProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    HDC hdc;  // �������� ����������
    PAINTSTRUCT ps;  // ��������� ��� ���������

    switch (uMsg) {  // ������������ ���������
    case WM_CREATE:  // ��������� � �������� ����
        LoadState(hwnd);  // ��������� ��������� ����
        return 0;

    case WM_COMMAND:  // ��������� � ������� (�� ������������)
        return 0;

    case WM_LBUTTONDOWN:  // ��������� � ������� ����� ������ ����
        updateState(hwnd, LOWORD(lParam), HIWORD(lParam), 2);  // ��������� ��������� (����������)
        return 0;

    case WM_RBUTTONDOWN:  // ��������� � ������� ������ ������ ����
        updateState(hwnd, LOWORD(lParam), HIWORD(lParam), 1);  // ��������� ��������� (�����)
        return 0;

    case WM_MOUSEMOVE:  // ��������� � �������� ���� (�� ������������)
        return 0;

    case WM_LBUTTONUP:  // ��������� �� ���������� ����� ������ ���� (�� ������������)
        return 0;

    case WM_SIZE:  // ��������� �� ��������� ������� ����
        InvalidateRect(hwnd, NULL, TRUE);  // ����������� ����������� ����
        return 0;

    case WM_PAINT:  // ��������� � ������������� ����������� ����
        hdc = BeginPaint(hwnd, &ps);  // �������� ���������
        DrawMarking(hwnd, hdc);  // ������ ��������
        DrawMatrix(hwnd, hdc);  // ������ ���������� ����
        EndPaint(hwnd, &ps);  // ����������� ���������
        return 0;

    case WM_KEYDOWN: {  // ��������� � ������� �������
        if (wParam == VK_ESCAPE || (wParam == 'Q' && GetKeyState(VK_CONTROL) < 0)) {  // ���� ������ ������� ESC ��� Ctrl+Q
            SaveState(hwnd);  // ��������� ���������
            PostQuitMessage(0);  // ��������� ����
        }
        if ((wParam == 'L') && (GetKeyState(VK_CONTROL) < 0)) {  // ���� ������ Ctrl+L
            ClearState();  // ������� ���������
            SetClassLongPtr(hwnd, GCLP_HBRBACKGROUND, (LONG_PTR)bgBrush);  // ��������� ���� ����
            InvalidateRect(hwnd, NULL, TRUE);  // �������������� ����
        }
        if ((wParam == 'C') && (GetKeyState(VK_SHIFT) < 0)) {  // ���� ������ Shift+C
            ShellExecute(NULL, L"open", L"state.txt", NULL, NULL, SW_SHOWNORMAL);  // ��������� ���� ���������
        }
        if (wParam == VK_RETURN) {  // ���� ������ ������� Enter
            DeleteObject(bgBrush);  // ������� ������ �����
            SetRandomBgColor();  // ������������� ��������� ���� ����
            bgBrush = CreateSolidBrush(bgColor);  // ������� ����� �����
            SetClassLongPtr(hwnd, GCLP_HBRBACKGROUND, (LONG_PTR)bgBrush);  // ��������� ���� ����
            InvalidateRect(hwnd, NULL, TRUE);  // �������������� ����
        }
        return 0;
    }
    case WM_MOUSEWHEEL: {  // ��������� � ��������� ������ ����
        short zDelta = GET_WHEEL_DELTA_WPARAM(wParam);  // �������� ����������� ���������

        if (zDelta > 0) {  // ���� ��������� �����
            ChangeGridColor(true);  // ����������� ������� ����� �����
        }
        else {  // ���� ��������� ����
            ChangeGridColor(false);  // ��������� ������� ����� �����
        }

        InvalidateRect(hwnd, NULL, TRUE);  // ����������� �����������
        return 0;
    }
    case WM_DESTROY:  // ��������� � �������� ����
        SaveState(hwnd);  // ��������� ���������
        PostQuitMessage(0);  // ��������� ���������
        return 0;

    default:  // ��������� ��������� ���������
        return DefWindowProc(hwnd, uMsg, wParam, lParam);  // ����������� ���������
    }
}