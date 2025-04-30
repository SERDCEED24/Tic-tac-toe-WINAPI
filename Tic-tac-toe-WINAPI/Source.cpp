#include <windows.h>  // Подключение библиотеки Windows API для работы с окнами
#include <vector>     // Подключение библиотеки для работы с векторами
#include <random>     // Подключение библиотеки для генерации случайных чисел
#include <cstdio>     // Подключение библиотеки для работы с файловыми указателями
#include <fstream>    // Подключение библиотеки для работы с потоками ввода/вывода
#include <string>     // Подключение библиотеки для работы со строками
#include <iostream>   // Подключение библиотеки для ввода/вывода

using namespace std;  // Использование стандартного пространства имен

// Объявление функции обработки сообщений окна
LRESULT CALLBACK WinProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// Глобальные переменные
int N = 3; // Размер игрового поля (по умолчанию 3x3)
bool isSizeParamSet = false; // Флаг, указывающий, был ли задан размер через параметры
vector<vector<int>> stateMatrix; // Локальная копия матрицы состояния
COLORREF markingColor = RGB(255, 0, 0); // Изначальный цвет сетки
int markingColorChangeSpeed = 5; // Скорость изменения цвета сетки
COLORREF bgColor = RGB(51, 129, 255); // Начальный цвет фона
HBRUSH bgBrush = CreateSolidBrush(bgColor); // Кисть для фона
const LPCWSTR configFile = L"config.bin"; // Имя конфигурационного файла

// Новые переменные для работы с разделяемой памятью
HANDLE hMapFile = NULL; // Дескриптор отображения файла (используется для разделяемой памяти)
LPVOID pSharedMem = NULL; // Указатель на область разделяемой памяти
HANDLE hMutex = NULL; // Мьютекс для синхронизации доступа к разделяемой памяти

// Определение пользовательских сообщений для межпроцессного взаимодействия
UINT WM_UPDATE_GAME_STATE = 0; // Сообщение для обновления состояния игры
UINT WM_UPDATE_COLORS = 0; // Сообщение для обновления цветов

// Структура для хранения данных в разделяемой памяти
struct SharedGameState {
    int N; // Размер игрового поля
    COLORREF bgColor; // Цвет фона
    COLORREF markingColor; // Цвет сетки
    int matrix[400]; // Плоский массив для матрицы состояния (максимум 20x20 = 400 элементов)
};

// Функция для инициализации разделяемой памяти
void InitSharedMemory() {
    // Создаем мьютекс для синхронизации доступа к разделяемой памяти
    hMutex = CreateMutex(NULL, FALSE, L"TicTacToeMutex");
    if (hMutex == NULL) {
        MessageBox(NULL, L"Ошибка создания мьютекса", L"Ошибка", MB_OK | MB_ICONERROR);
        return;
    }

    // Создаем или открываем отображение файла в память
    hMapFile = CreateFileMapping(
        INVALID_HANDLE_VALUE, // Используем страничный файл (анонимное отображение)
        NULL, // Атрибуты безопасности по умолчанию
        PAGE_READWRITE, // Доступ на чтение и запись
        0, sizeof(SharedGameState), // Размер выделяемой памяти
        L"TicTacToeSharedMemory" // Имя объекта разделяемой памяти
    );

    if (hMapFile == NULL) {
        MessageBox(NULL, L"Ошибка создания разделяемой памяти", L"Ошибка", MB_OK | MB_ICONERROR);
        CloseHandle(hMutex);
        return;
    }

    // Отображаем разделяемую память в адресное пространство процесса
    pSharedMem = MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(SharedGameState));
    if (pSharedMem == NULL) {
        MessageBox(NULL, L"Ошибка отображения памяти", L"Ошибка", MB_OK | MB_ICONERROR);
        CloseHandle(hMapFile);
        CloseHandle(hMutex);
        return;
    }

    // Получаем указатель на структуру в разделяемой памяти
    SharedGameState* sharedState = (SharedGameState*)pSharedMem;

    // Захватываем мьютекс перед доступом к разделяемой памяти
    WaitForSingleObject(hMutex, INFINITE);

    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        // Если память уже существует, загружаем данные из нее
        N = sharedState->N;
        bgColor = sharedState->bgColor;
        markingColor = sharedState->markingColor;

        // Инициализируем локальную матрицу состояния
        stateMatrix.assign(N, vector<int>(N, 0));

        // Копируем данные из плоского массива в матрицу
        for (int i = 0; i < N; ++i)
            for (int j = 0; j < N; ++j)
                stateMatrix[i][j] = sharedState->matrix[i * N + j];
    }
    else {
        // Инициализируем новую разделяемую память
        sharedState->N = N;
        sharedState->bgColor = bgColor;
        sharedState->markingColor = markingColor;

        // Инициализируем локальную матрицу состояния
        stateMatrix.assign(N, vector<int>(N, 0));

        // Инициализируем плоский массив в разделяемой памяти
        for (int i = 0; i < N * N; ++i)
            sharedState->matrix[i] = 0;
    }

    // Освобождаем мьютекс
    ReleaseMutex(hMutex);
}

// Функция для обновления разделяемой памяти
void UpdateSharedMemory() {
    if (!pSharedMem) return; // Проверяем, что память инициализирована

    // Получаем указатель на структуру в разделяемой памяти
    SharedGameState* sharedState = (SharedGameState*)pSharedMem;

    // Захватываем мьютекс перед изменением данных
    WaitForSingleObject(hMutex, INFINITE);

    // Обновляем данные в разделяемой памяти
    sharedState->N = N;
    sharedState->bgColor = bgColor;
    sharedState->markingColor = markingColor;

    // Копируем данные из матрицы в плоский массив
    for (int i = 0; i < N; ++i)
        for (int j = 0; j < N; ++j)
            sharedState->matrix[i * N + j] = stateMatrix[i][j];

    // Освобождаем мьютекс
    ReleaseMutex(hMutex);
}

// Функция для синхронизации локального состояния с разделяемой памятью
void SyncFromSharedMemory(HWND hwnd) {
    if (!pSharedMem) return; // Проверяем, что память инициализирована

    // Получаем указатель на структуру в разделяемой памяти
    SharedGameState* sharedState = (SharedGameState*)pSharedMem;

    // Захватываем мьютекс перед чтением данных
    WaitForSingleObject(hMutex, INFINITE);

    // Обновляем локальные переменные из разделяемой памяти
    N = sharedState->N;
    bgColor = sharedState->bgColor;
    markingColor = sharedState->markingColor;

    // Обновляем локальную матрицу состояния
    stateMatrix.assign(N, vector<int>(N, 0));
    for (int i = 0; i < N; ++i)
        for (int j = 0; j < N; ++j)
            stateMatrix[i][j] = sharedState->matrix[i * N + j];

    // Освобождаем мьютекс
    ReleaseMutex(hMutex);

    // Обновляем кисть фона
    DeleteObject(bgBrush);
    bgBrush = CreateSolidBrush(bgColor);

    // Устанавливаем новую кисть фона для окна
    SetClassLongPtr(hwnd, GCLP_HBRBACKGROUND, (LONG_PTR)bgBrush);

    // Запрашиваем перерисовку окна
    InvalidateRect(hwnd, NULL, TRUE);
}

// Функция для плавного изменения цвета сетки
void ChangeGridColor(HWND hwnd, bool increase) {
    // Получаем компоненты текущего цвета
    int r = GetRValue(markingColor);
    int g = GetGValue(markingColor);
    int b = GetBValue(markingColor);

    // Изменяем красную компоненту цвета
    if (increase) {
        r = (r + markingColorChangeSpeed) % 256; // Увеличиваем яркость
    }
    else {
        r = (r - markingColorChangeSpeed + 256) % 256; // Уменьшаем яркость
    }

    // Устанавливаем новый цвет
    markingColor = RGB(r, g, b);

    // Обновляем разделяемую память
    UpdateSharedMemory();

    // Отправляем сообщение всем окнам для обновления цветов
    PostMessage(HWND_BROADCAST, WM_UPDATE_COLORS, 0, 0);

    // Запрашиваем перерисовку текущего окна
    InvalidateRect(hwnd, NULL, TRUE);
}

// Функция для установки случайного цвета фона
void SetRandomBgColor(HWND hwnd) {
    // Инициализируем генератор случайных чисел
    static random_device rd;
    static mt19937 gen(rd());
    static uniform_int_distribution<int> dist(0, 255);

    // Генерируем случайный цвет
    bgColor = RGB(dist(gen), dist(gen), dist(gen));

    // Обновляем кисть фона
    DeleteObject(bgBrush);
    bgBrush = CreateSolidBrush(bgColor);

    // Обновляем разделяемую память
    UpdateSharedMemory();

    // Отправляем сообщение всем окнам для обновления цветов
    PostMessage(HWND_BROADCAST, WM_UPDATE_COLORS, 0, 0);

    // Устанавливаем новую кисть фона для окна
    SetClassLongPtr(hwnd, GCLP_HBRBACKGROUND, (LONG_PTR)bgBrush);

    // Запрашиваем перерисовку окна
    InvalidateRect(hwnd, NULL, TRUE);
}

// Функция для сохранения конфигурации (размера поля) в файл
void SaveConfig(int n) {
    // Создаем или перезаписываем файл конфигурации
    HANDLE hFile = CreateFileW(configFile, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return;

    DWORD written;
    // Записываем размер поля в файл
    WriteFile(hFile, &n, sizeof(n), &written, NULL);

    // Закрываем файл
    CloseHandle(hFile);
}

// Функция для загрузки конфигурации (размера поля) из файла
void LoadConfig(HWND hwnd) {
    // Открываем файл конфигурации для чтения
    HANDLE hFile = CreateFileW(configFile, GENERIC_READ, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        MessageBox(NULL, L"Ошибка открытия файла конфигурации", L"Ошибка", MB_OK | MB_ICONERROR);
        return;
    }

    int savedN;
    DWORD read;
    // Читаем сохраненный размер поля из файла
    ReadFile(hFile, &savedN, sizeof(savedN), &read, NULL);

    // Если размер не был задан через параметры или совпадает с сохраненным
    if (!isSizeParamSet || savedN == N) {
        N = savedN; // Устанавливаем сохраненный размер
    }

    // Закрываем файл
    CloseHandle(hFile);
}

// Функция для обновления состояния игрового поля
void updateState(HWND hwnd, int x, int y, int type) {
    RECT rect;
    GetClientRect(hwnd, &rect); // Получаем размеры клиентской области окна
    int cellWidth = rect.right / N; // Вычисляем ширину одной ячейки
    int cellHeight = rect.bottom / N; // Вычисляем высоту одной ячейки

    // Определяем строку и столбец ячейки по координатам
    int col = x / cellWidth;
    int row = y / cellHeight;

    // Проверяем, что ячейка в пределах поля
    if (row >= 0 && row < N && col >= 0 && col < N) {
        // Если состояние ячейки изменилось
        if (stateMatrix[row][col] != type) {
            stateMatrix[row][col] = type; // Обновляем состояние

            // Обновляем разделяемую память
            UpdateSharedMemory();

            // Отправляем сообщение всем окнам для обновления состояния
            PostMessage(HWND_BROADCAST, WM_UPDATE_GAME_STATE, 0, 0);

            // Определяем область ячейки для перерисовки
            RECT cellRect = { col * cellWidth, row * cellHeight,
                             (col + 1) * cellWidth, (row + 1) * cellHeight };

            // Запрашиваем перерисовку только этой ячейки
            InvalidateRect(hwnd, &cellRect, TRUE);
        }
    }
}

// Функция для очистки состояния игры
void ClearState(HWND hwnd) {
    // Очищаем матрицу состояния
    stateMatrix.assign(N, vector<int>(N, 0));

    // Сбрасываем цвет сетки на красный
    markingColor = RGB(255, 0, 0);

    // Обновляем кисть фона
    DeleteObject(bgBrush);
    bgColor = RGB(51, 129, 255); // Синий цвет по умолчанию
    bgBrush = CreateSolidBrush(bgColor);

    // Обновляем разделяемую память
    UpdateSharedMemory();

    // Отправляем сообщения всем окнам для обновления
    PostMessage(HWND_BROADCAST, WM_UPDATE_GAME_STATE, 0, 0);
    PostMessage(HWND_BROADCAST, WM_UPDATE_COLORS, 0, 0);
}

// Функция для рисования разметки игрового поля
void DrawMarking(HWND hwnd, HDC hdc) {
    if (N <= 1) return; // Не рисуем для некорректных размеров

    RECT rect;
    GetClientRect(hwnd, &rect); // Получаем размеры клиентской области
    int width = rect.right;
    int height = rect.bottom;
    int cellWidth = width / N; // Ширина одной ячейки
    int cellHeight = height / N; // Высота одной ячейки

    // Создаем перо для рисования линий сетки
    HPEN hPen = CreatePen(PS_SOLID, 5, markingColor);
    HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);

    // Рисуем вертикальные линии
    for (int i = 1; i < N; ++i) {
        int x = i * cellWidth;
        MoveToEx(hdc, x, 0, NULL);
        LineTo(hdc, x, height);
    }

    // Рисуем горизонтальные линии
    for (int i = 1; i < N; ++i) {
        int y = i * cellHeight;
        MoveToEx(hdc, 0, y, NULL);
        LineTo(hdc, width, y);
    }

    // Восстанавливаем старое перо и удаляем созданное
    SelectObject(hdc, hOldPen);
    DeleteObject(hPen);
}

// Функция для рисования содержимого игрового поля (крестиков и ноликов)
void DrawMatrix(HWND hwnd, HDC hdc) {
    RECT rect;
    GetClientRect(hwnd, &rect); // Получаем размеры клиентской области
    int cellWidth = rect.right / N; // Ширина одной ячейки
    int cellHeight = rect.bottom / N; // Высота одной ячейки

    // Создаем перья для рисования крестиков и ноликов
    HPEN hCrossPen = CreatePen(PS_SOLID, 5, RGB(0, 255, 0)); // Зеленый для крестиков
    HPEN hCirclePen = CreatePen(PS_SOLID, 5, RGB(255, 50, 255)); // Фиолетовый для ноликов
    HPEN hOldPen;

    // Перебираем все ячейки поля
    for (int row = 0; row < N; ++row) {
        for (int col = 0; col < N; ++col) {
            // Координаты ячейки
            int x1 = col * cellWidth;
            int y1 = row * cellHeight;
            int x2 = x1 + cellWidth;
            int y2 = y1 + cellHeight;

            // Отступ от краев ячейки для рисования фигур
            int padding = min(cellWidth, cellHeight) / 4;

            // Если в ячейке крестик (значение 1)
            if (stateMatrix[row][col] == 1) {
                hOldPen = (HPEN)SelectObject(hdc, hCrossPen);
                // Рисуем первую диагональ крестика
                MoveToEx(hdc, x1 + padding, y1 + padding, NULL);
                LineTo(hdc, x2 - padding, y2 - padding);
                // Рисуем вторую диагональ крестика
                MoveToEx(hdc, x2 - padding, y1 + padding, NULL);
                LineTo(hdc, x1 + padding, y2 - padding);
                SelectObject(hdc, hOldPen);
            }
            // Если в ячейке нолик (значение 2)
            else if (stateMatrix[row][col] == 2) {
                hOldPen = (HPEN)SelectObject(hdc, hCirclePen);
                // Устанавливаем прозрачную кисть для заливки
                HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
                // Рисуем окружность
                Ellipse(hdc, x1 + padding, y1 + padding, x2 - padding, y2 - padding);
                // Восстанавливаем кисть
                SelectObject(hdc, hOldBrush);
                SelectObject(hdc, hOldPen);
            }
        }
    }

    // Удаляем созданные перья
    DeleteObject(hCrossPen);
    DeleteObject(hCirclePen);
}

// Функция для обработки параметров командной строки
void parseCmdParams(LPWSTR cmd) {
    int argc;
    // Разбираем командную строку на аргументы
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);

    if (argv && argc > 1) {
        // Пытаемся преобразовать первый аргумент в число (размер поля)
        int newN = _wtoi(argv[1]);
        if (newN > 1 && newN < 21) { // Проверяем корректность размера
            N = newN;
            isSizeParamSet = true; // Устанавливаем флаг, что размер задан через параметры
        }
    }

    // Освобождаем память, выделенную для аргументов
    LocalFree(argv);
}

// Основная функция программы
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrev, PWSTR pCmdLine, int showWin) {
    // Регистрируем пользовательские сообщения для межпроцессного взаимодействия
    WM_UPDATE_GAME_STATE = RegisterWindowMessage(L"TicTacToe_UpdateGameState");
    WM_UPDATE_COLORS = RegisterWindowMessage(L"TicTacToe_UpdateColors");

    // Проверяем успешность регистрации сообщений
    if (WM_UPDATE_GAME_STATE == 0 || WM_UPDATE_COLORS == 0) {
        MessageBox(NULL, L"Ошибка регистрации сообщений", L"Ошибка", MB_OK | MB_ICONERROR);
        return -1;
    }

    // Обрабатываем параметры командной строки
    parseCmdParams(pCmdLine);

    // Загружаем конфигурацию (размер поля) из файла
    LoadConfig(NULL);

    // Инициализируем разделяемую память
    InitSharedMemory();

    // Регистрируем класс окна
    WNDCLASSEX wc = { 0 };
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = WinProc; // Указатель на функцию обработки сообщений
    wc.hInstance = hInstance; // Дескриптор экземпляра приложения
    wc.lpszClassName = L"Tic-tac-toe"; // Имя класса окна
    wc.hbrBackground = bgBrush; // Кисть для фона окна
    wc.hCursor = LoadCursor(NULL, IDC_ARROW); // Курсор по умолчанию

    if (!RegisterClassEx(&wc)) {
        return -1; // Если регистрация не удалась, завершаем программу
    }

    // Создаем главное окно приложения
    HWND hwnd = CreateWindowW(
        L"Tic-tac-toe", // Имя класса окна
        L"Tic-tac-toe", // Заголовок окна
        WS_OVERLAPPEDWINDOW, // Стиль окна
        100, 100, 320, 240, // Позиция и размеры
        NULL, NULL, hInstance, NULL // Дополнительные параметры
    );

    if (!hwnd) return -1; // Если окно не создано, завершаем программу

    // Показываем и обновляем окно
    ShowWindow(hwnd, showWin);
    UpdateWindow(hwnd);

    // Цикл обработки сообщений
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Очистка ресурсов перед выходом
    if (pSharedMem) UnmapViewOfFile(pSharedMem);
    if (hMapFile) CloseHandle(hMapFile);
    if (hMutex) CloseHandle(hMutex);
    DeleteObject(bgBrush);

    return 0;
}

// Функция обработки сообщений окна
LRESULT CALLBACK WinProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    HDC hdc;
    PAINTSTRUCT ps;

    // Обработка пользовательских сообщений
    if (uMsg == WM_UPDATE_GAME_STATE || uMsg == WM_UPDATE_COLORS) {
        SyncFromSharedMemory(hwnd); // Синхронизируем состояние из разделяемой памяти
        return 0;
    }

    switch (uMsg) {
    case WM_CREATE:
        // При создании окна синхронизируем состояние
        SyncFromSharedMemory(hwnd);
        return 0;

    case WM_LBUTTONDOWN:
        // Обработка клика левой кнопкой мыши (нолик)
        updateState(hwnd, LOWORD(lParam), HIWORD(lParam), 2);
        return 0;

    case WM_RBUTTONDOWN:
        // Обработка клика правой кнопкой мыши (крестик)
        updateState(hwnd, LOWORD(lParam), HIWORD(lParam), 1);
        return 0;

    case WM_SIZE:
        // При изменении размера окна запрашиваем перерисовку
        InvalidateRect(hwnd, NULL, TRUE);
        return 0;

    case WM_PAINT:
        // Обработка сообщения о необходимости перерисовки
        hdc = BeginPaint(hwnd, &ps);
        DrawMarking(hwnd, hdc); // Рисуем разметку
        DrawMatrix(hwnd, hdc); // Рисуем содержимое поля
        EndPaint(hwnd, &ps);
        return 0;

    case WM_KEYDOWN:
        // Обработка нажатий клавиш
        if (wParam == VK_ESCAPE || (wParam == 'Q' && GetKeyState(VK_CONTROL) < 0)) {
            // ESC или Ctrl+Q - сохраняем конфигурацию и выходим
            SaveConfig(N);
            PostQuitMessage(0);
        }
        if ((wParam == 'L') && (GetKeyState(VK_CONTROL) < 0)) {
            // Ctrl+L - очищаем состояние
            ClearState(hwnd);
            SetClassLongPtr(hwnd, GCLP_HBRBACKGROUND, (LONG_PTR)bgBrush);
            InvalidateRect(hwnd, NULL, TRUE);
        }
        if ((wParam == 'C') && (GetKeyState(VK_SHIFT) < 0)) {
            // Shift+C - открываем блокнот
            ShellExecute(NULL, L"open", L"notepad", NULL, NULL, SW_SHOWNORMAL);
        }
        if (wParam == VK_RETURN) {
            // Enter - устанавливаем случайный цвет фона
            SetRandomBgColor(hwnd);
        }
        return 0;

    case WM_MOUSEWHEEL: {
        // Обработка прокрутки колеса мыши
        short zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
        ChangeGridColor(hwnd, zDelta > 0); // Изменяем цвет сетки
        return 0;
    }

    case WM_DESTROY:
        // При закрытии окна сохраняем конфигурацию и выходим
        SaveConfig(N);
        PostQuitMessage(0);
        return 0;

    default:
        // Обработка остальных сообщений стандартным обработчиком
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
}