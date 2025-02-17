#include <windows.h>  // Подключение библиотеки Windows API для работы с окнами
#include <vector>     // Подключение библиотеки для работы с векторами
#include <sstream>    // Подключение библиотеки для работы со строками
#include <fstream>    // Подключение библиотеки для работы с файлами
#include <random>     // Подключение библиотеки для генерации случайных чисел

using namespace std;  // Использование стандартного пространства имен

// Объявление функции обработки сообщений окна
LRESULT CALLBACK WinProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// Глобальные переменные для взаимодействия с состоянием игры
int N = 3;  // Размер игрового поля (по умолчанию 3x3)
bool isSizeParamSet = false;  // Флаг, указывающий, был ли задан размер через параметры командной строки
vector<vector<int>> stateMatrix(N, vector<int>(N, 0));  // Матрица состояния игрового поля
COLORREF markingColor = RGB(255, 0, 0);  // Изначальный цвет сетки (красный)
int markingColorChangeSpeed = 5;  // Скорость изменения цвета сетки
COLORREF bgColor = RGB(51, 129, 255);  // Начальный цвет фона
HBRUSH bgBrush = CreateSolidBrush(bgColor);  // Кисть для заливки фона
const char* saveFile = "state.txt";  // Имя файла для сохранения состояния

// Функция для плавного изменения цвета сетки
void ChangeGridColor(bool increase) {
    // Извлекаем компоненты цвета (R, G, B)
    int r = GetRValue(markingColor);
    int g = GetGValue(markingColor);
    int b = GetBValue(markingColor);

    // Изменяем компоненты цвета в зависимости от направления изменения
    if (increase) {
        r = (r + markingColorChangeSpeed) % 256;  // Увеличиваем яркость красного
    }
    else {
        r = (r - markingColorChangeSpeed + 256) % 256;  // Уменьшаем яркость красного
    }

    // Обновляем цвет сетки
    markingColor = RGB(r, g, b);
}

// Функция для установки случайного цвета фона
void SetRandomBgColor() {
    static random_device rd;  // Генератор случайных чисел
    static mt19937 gen(rd());  // Механизм генерации случайных чисел
    static uniform_int_distribution<int> dist(0, 255);  // Распределение для RGB (0-255)
    bgColor = RGB(dist(gen), dist(gen), dist(gen));  // Генерация случайного цвета
}

// Функция для сохранения состояния игры в файл
void SaveState(HWND hwnd) {
    ofstream file(saveFile);  // Открываем файл для записи
    if (!file) return;  // Если файл не открылся, выходим
    RECT rect;
    GetWindowRect(hwnd, &rect);  // Получаем размеры окна
    file << N << '\n';  // Сохраняем размер игрового поля
    file << (rect.right - rect.left) << ' ' << (rect.bottom - rect.top) << '\n';  // Сохраняем размеры окна
    file << bgColor << '\n';  // Сохраняем цвет фона
    file << markingColor << '\n';  // Сохраняем цвет сетки
    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < N; ++j) {
            file << stateMatrix[i][j] << ' ';  // Сохраняем состояние каждой ячейки
        }
        file << '\n';
    }
}

// Функция для загрузки состояния игры из файла
void LoadState(HWND hwnd) {
    ifstream file(saveFile);  // Открываем файл для чтения
    if (!file) return;  // Если файл не открылся, выходим
    int width = 320, height = 240;  // Значения по умолчанию для размеров окна
    int savedN;
    file >> savedN;  // Читаем размер игрового поля
    file >> width >> height;  // Читаем размеры окна
    file >> bgColor;  // Читаем цвет фона
    file >> markingColor;  // Читаем цвет сетки
    if (!isSizeParamSet || savedN == N) {  // Если размер не был задан через параметры командной строки или совпадает с параметром командной строки
        N = savedN;  // Обновляем размер игрового поля
        stateMatrix.assign(N, vector<int>(N, 0));  // Обновляем матрицу состояния
        for (int i = 0; i < N; ++i)
            for (int j = 0; j < N; ++j)
                file >> stateMatrix[i][j];  // Читаем состояние каждой ячейки
    }
    DeleteObject(bgBrush);  // Удаляем старую кисть
    bgBrush = CreateSolidBrush(bgColor);  // Создаем новую кисть с загруженным цветом
    SetClassLongPtr(hwnd, GCLP_HBRBACKGROUND, (LONG_PTR)bgBrush);  // Устанавливаем новый цвет фона
    SetWindowPos(hwnd, NULL, 0, 0, width, height, SWP_NOMOVE | SWP_NOZORDER);  // Устанавливаем размеры окна
    InvalidateRect(hwnd, NULL, TRUE);  // Перерисовываем окно
}

// Функция для обновления состояния игрового поля
void updateState(HWND hwnd, int x, int y, int type) {
    RECT rect;
    GetClientRect(GetForegroundWindow(), &rect);  // Получаем размеры клиентской области окна
    int cellWidth = rect.right / N;  // Ширина одной ячейки
    int cellHeight = rect.bottom / N;  // Высота одной ячейки

    int col = x / cellWidth;  // Определяем столбец ячейки
    int row = y / cellHeight;  // Определяем строку ячейки

    if (row >= 0 && row < N && col >= 0 && col < N) {  // Проверяем, что ячейка в пределах поля
        if (stateMatrix[row][col] != type) {  // Если состояние ячейки изменилось
            stateMatrix[row][col] = type;  // Обновляем состояние ячейки (1 - крест, 2 - окружность)

            RECT cellRect = {
                col * cellWidth, row * cellHeight,
                (col + 1) * cellWidth, (row + 1) * cellHeight
            };  // Определяем область ячейки

            InvalidateRect(hwnd, &cellRect, TRUE);  // Перерисовываем ячейку
        }
    }
}

// Функция для очистки состояния игры
void ClearState() {
    for (int i = 0; i < N; ++i)
        for (int j = 0; j < N; ++j)
            stateMatrix[i][j] = 0;  // Очищаем матрицу состояния
    markingColor = RGB(255, 0, 0);  // Сбрасываем цвет сетки
    DeleteObject(bgBrush);  // Удаляем старую кисть
    bgColor = RGB(51, 129, 255); // Ставим значение фона по умолчанию
    bgBrush = CreateSolidBrush(bgColor);  // Создаем новую кисть для фона с цветом по умолчанию
}

// Функция для рисования разметки игрового поля
void DrawMarking(HWND hwnd, HDC hdc) {
    if (N <= 1) return;  // Защита от некорректных значений
    RECT rect;
    GetClientRect(hwnd, &rect);  // Получаем размеры клиентской области окна

    int width = rect.right;  // Ширина клиентской области
    int height = rect.bottom;  // Высота клиентской области
    int cellWidth = width / N;  // Ширина одной ячейки
    int cellHeight = height / N;  // Высота одной ячейки

    HPEN hPen = CreatePen(PS_SOLID, 5, markingColor);  // Создаем перо для рисования линий
    HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);  // Выбираем перо в контекст устройства

    // Рисуем вертикальные линии
    for (int i = 1; i < N; ++i) {
        int x = i * cellWidth;
        MoveToEx(hdc, x, 0, NULL);  // Перемещаемся к началу линии
        LineTo(hdc, x, height);  // Рисуем линию
    }

    // Рисуем горизонтальные линии
    for (int i = 1; i < N; ++i) {
        int y = i * cellHeight;
        MoveToEx(hdc, 0, y, NULL);  // Перемещаемся к началу линии
        LineTo(hdc, width, y);  // Рисуем линию
    }

    SelectObject(hdc, hOldPen);  // Восстанавливаем старое перо
    DeleteObject(hPen);  // Удаляем созданное перо
}

// Функция для рисования содержимого игрового поля (кресты и окружности)
void DrawMatrix(HWND hwnd, HDC hdc) {
    RECT rect;
    GetClientRect(hwnd, &rect);  // Получаем размеры клиентской области окна
    int cellWidth = rect.right / N;  // Ширина одной ячейки
    int cellHeight = rect.bottom / N;  // Высота одной ячейки

    HPEN hCrossPen = CreatePen(PS_SOLID, 5, RGB(0, 255, 0));  // Перо для крестов (зелёное)
    HPEN hCirclePen = CreatePen(PS_SOLID, 5, RGB(255, 50, 255));  // Перо для окружностей (фиолетовое)
    HPEN hOldPen;

    for (int row = 0; row < N; ++row) {
        for (int col = 0; col < N; ++col) {
            int x1 = col * cellWidth;  // Левый верхний угол ячейки
            int y1 = row * cellHeight;  // Левый верхний угол ячейки
            int x2 = x1 + cellWidth;  // Правый нижний угол ячейки
            int y2 = y1 + cellHeight;  // Правый нижний угол ячейки
            int padding = min(cellWidth, cellHeight) / 4;  // Отступ для рисования фигур

            if (stateMatrix[row][col] == 1) {  // Если в ячейке крест
                hOldPen = (HPEN)SelectObject(hdc, hCrossPen);  // Выбираем перо для креста
                MoveToEx(hdc, x1 + padding, y1 + padding, NULL);  // Рисуем первую линию креста
                LineTo(hdc, x2 - padding, y2 - padding);
                MoveToEx(hdc, x2 - padding, y1 + padding, NULL);  // Рисуем вторую линию креста
                LineTo(hdc, x1 + padding, y2 - padding);
                SelectObject(hdc, hOldPen);  // Восстанавливаем старое перо
            }
            else if (stateMatrix[row][col] == 2) {  // Если в ячейке окружность
                hOldPen = (HPEN)SelectObject(hdc, hCirclePen);  // Выбираем перо для окружности
                HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));  // Без заливки
                Ellipse(hdc, x1 + padding, y1 + padding, x2 - padding, y2 - padding);  // Рисуем окружность
                SelectObject(hdc, hOldBrush);  // Восстанавливаем старую кисть
                SelectObject(hdc, hOldPen);  // Восстанавливаем старое перо
            }
        }
    }
    DeleteObject(hCrossPen);  // Удаляем перо для крестов
    DeleteObject(hCirclePen);  // Удаляем перо для окружностей
}

// Функция для инициализации состояния игры
void initState() {
    stateMatrix.assign(N, vector<int>(N, 0));  // Заполняем матрицу нулями
}

// Функция для обработки параметров командной строки
void parseCmdParams(LPWSTR cmd) {
    wstringstream wss(cmd);  // Создаем поток для чтения параметров
    int newN;
    if (wss >> newN && newN > 1 && newN < 21) {  // Если параметр корректен
        N = newN;  // Обновляем размер игрового поля
        isSizeParamSet = true;  // Устанавливаем флаг, гласящий о том, что был введён параметр командной строки, на true
    }
    initState();  // Инициализируем состояние игры
}

// Основная функция программы
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrev, PWSTR pCmdLine, int showWin) {
    parseCmdParams(pCmdLine);  // Обрабатываем параметры командной строки
    WNDCLASSEX wc = { 0 };  // Создаем структуру для описания класса окна
    wc.cbSize = sizeof(WNDCLASSEX);  // Размер структуры
    wc.lpfnWndProc = WinProc;  // Указатель на функцию обработки сообщений
    wc.hInstance = hInstance;  // Дескриптор экземпляра приложения
    wc.lpszClassName = L"Tic-tac-toe";  // Имя класса окна
    wc.hbrBackground = bgBrush;  // Кисть для фона окна
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);  // Курсор по умолчанию

    if (!RegisterClassEx(&wc)) {  // Регистрируем класс окна
        return -1;  // Если регистрация не удалась, завершаем программу
    }

    // Создаем окно
    HWND hwnd = CreateWindowW(
        L"Tic-tac-toe",  // Имя класса окна
        L"Tic-tac-toe",  // Заголовок окна
        WS_OVERLAPPEDWINDOW,  // Стиль окна
        100, 100, 320, 240,  // Позиция и размеры окна
        NULL, NULL, hInstance, NULL  // Дополнительные параметры
    );

    if (!hwnd) return -1;  // Если окно не создалось, завершаем программу

    ShowWindow(hwnd, showWin);  // Показываем окно
    UpdateWindow(hwnd);  // Обновляем окно

    // Основной цикл обработки сообщений
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);  // Преобразуем сообщения
        DispatchMessage(&msg);  // Отправляем сообщения в функцию обработки
    }

    return 0;  // Завершаем программу
}

// Функция обработки сообщений окна
LRESULT CALLBACK WinProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    HDC hdc;  // Контекст устройства
    PAINTSTRUCT ps;  // Структура для рисования

    switch (uMsg) {  // Обрабатываем сообщения
    case WM_CREATE:  // Сообщение о создании окна
        LoadState(hwnd);  // Загружаем состояние игры
        return 0;

    case WM_COMMAND:  // Сообщение о команде (не используется)
        return 0;

    case WM_LBUTTONDOWN:  // Сообщение о нажатии левой кнопки мыши
        updateState(hwnd, LOWORD(lParam), HIWORD(lParam), 2);  // Обновляем состояние (окружность)
        return 0;

    case WM_RBUTTONDOWN:  // Сообщение о нажатии правой кнопки мыши
        updateState(hwnd, LOWORD(lParam), HIWORD(lParam), 1);  // Обновляем состояние (крест)
        return 0;

    case WM_MOUSEMOVE:  // Сообщение о движении мыши (не используется)
        return 0;

    case WM_LBUTTONUP:  // Сообщение об отпускании левой кнопки мыши (не используется)
        return 0;

    case WM_SIZE:  // Сообщение об изменении размера окна
        InvalidateRect(hwnd, NULL, TRUE);  // Запрашиваем перерисовку окна
        return 0;

    case WM_PAINT:  // Сообщение о необходимости перерисовки окна
        hdc = BeginPaint(hwnd, &ps);  // Начинаем рисование
        DrawMarking(hwnd, hdc);  // Рисуем разметку
        DrawMatrix(hwnd, hdc);  // Рисуем содержимое поля
        EndPaint(hwnd, &ps);  // Заканчиваем рисование
        return 0;

    case WM_KEYDOWN: {  // Сообщение о нажатии клавиши
        if (wParam == VK_ESCAPE || (wParam == 'Q' && GetKeyState(VK_CONTROL) < 0)) {  // Если нажата клавиша ESC или Ctrl+Q
            SaveState(hwnd);  // Сохраняем состояние
            PostQuitMessage(0);  // Закрываем окно
        }
        if ((wParam == 'L') && (GetKeyState(VK_CONTROL) < 0)) {  // Если нажата Ctrl+L
            ClearState();  // Очищаем состояние
            SetClassLongPtr(hwnd, GCLP_HBRBACKGROUND, (LONG_PTR)bgBrush);  // Обновляем цвет фона
            InvalidateRect(hwnd, NULL, TRUE);  // Перерисовываем окно
        }
        if ((wParam == 'C') && (GetKeyState(VK_SHIFT) < 0)) {  // Если нажата Shift+C
            ShellExecute(NULL, L"open", L"state.txt", NULL, NULL, SW_SHOWNORMAL);  // Открываем файл состояния
        }
        if (wParam == VK_RETURN) {  // Если нажата клавиша Enter
            DeleteObject(bgBrush);  // Удаляем старую кисть
            SetRandomBgColor();  // Устанавливаем случайный цвет фона
            bgBrush = CreateSolidBrush(bgColor);  // Создаем новую кисть
            SetClassLongPtr(hwnd, GCLP_HBRBACKGROUND, (LONG_PTR)bgBrush);  // Обновляем цвет фона
            InvalidateRect(hwnd, NULL, TRUE);  // Перерисовываем окно
        }
        return 0;
    }
    case WM_MOUSEWHEEL: {  // Сообщение о прокрутке колеса мыши
        short zDelta = GET_WHEEL_DELTA_WPARAM(wParam);  // Получаем направление прокрутки

        if (zDelta > 0) {  // Если прокрутка вверх
            ChangeGridColor(true);  // Увеличиваем яркость цвета сетки
        }
        else {  // Если прокрутка вниз
            ChangeGridColor(false);  // Уменьшаем яркость цвета сетки
        }

        InvalidateRect(hwnd, NULL, TRUE);  // Запрашиваем перерисовку
        return 0;
    }
    case WM_DESTROY:  // Сообщение о закрытии окна
        SaveState(hwnd);  // Сохраняем состояние
        PostQuitMessage(0);  // Завершаем программу
        return 0;

    default:  // Обработка остальных сообщений
        return DefWindowProc(hwnd, uMsg, wParam, lParam);  // Стандартная обработка
    }
}