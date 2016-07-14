#define _CRT_SECURE_NO_WARNINGS 1
#include <Windows.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#include <xmmintrin.h>

typedef void calculate_func_t(unsigned char * out, double X0, double Y0, double scale, unsigned YSTART, unsigned SX, unsigned SY);

extern void calculate_float (unsigned char * out, double X0, double Y0, double scale, unsigned YSTART, unsigned SX, unsigned SY);
extern void calculate_double (unsigned char * out, double X0, double Y0, double scale, unsigned YSTART, unsigned SX, unsigned SY);

extern void calculate_sse_float (unsigned char * out, double X0, double Y0, double scale, unsigned YSTART, unsigned SX, unsigned SY);
extern void calculate_sse_double (unsigned char * out, double X0, double Y0, double scale, unsigned YSTART, unsigned SX, unsigned SY);

extern void calculate_avx_float (unsigned char * out, double X0, double Y0, double scale, unsigned YSTART, unsigned SX, unsigned SY);
extern void calculate_avx_double (unsigned char * out, double X0, double Y0, double scale, unsigned YSTART, unsigned SX, unsigned SY);

extern void calculate_avx2_float (unsigned char * out, double X0, double Y0, double scale, unsigned YSTART, unsigned SX, unsigned SY);
extern void calculate_avx2_double (unsigned char * out, double X0, double Y0, double scale, unsigned YSTART, unsigned SX, unsigned SY);

extern void calculate_fma_float (unsigned char * out, double X0, double Y0, double scale, unsigned YSTART, unsigned SX, unsigned SY);
extern void calculate_fma_double (unsigned char * out, double X0, double Y0, double scale, unsigned YSTART, unsigned SX, unsigned SY);


/* ------------------------------------------------------------------------------------------------ */

calculate_func_t * calculate_func;

calculate_func_t * funcs [20];
char * method_names [20];
int func_count = 0;
int func_index = 0;
BOOL full_mode = FALSE;

double X0 = 0;
double Y0 = 0;
double scale;
unsigned SX; // unaligned (real) size
unsigned SY;

BYTE * data = 0;
size_t data_size = 0;
BOOL image_valid = FALSE;

BOOL mousedown = FALSE;
POINT cursor_pos;

unsigned processor_count;


struct {
    BITMAPINFOHEADER h;
    RGBQUAD colors[256];
} bi;

void init_threads (void)
{
    SYSTEM_INFO si;
    GetSystemInfo (&si);
    processor_count = si.dwNumberOfProcessors;
}

void init_functions ()
{
    int out [4];

    __cpuid (out, 1);

    BOOL sse42 = (out [2] & (1 << 20)) != 0;
    BOOL sse41 = (out [2] & (1 << 19)) != 0;
    BOOL fma = (out [2] & (1 << 12)) != 0;
    BOOL avx = (out [2] & (1 << 28)) != 0;
    BOOL osxsave = (out [2] & (1 << 29)) != 0;

    __cpuidex (out, 7, 0);
    BOOL avx2 = (out [1] & 0x20) != 0;

    int i = 0;
    funcs [i] = calculate_float; method_names [i] = "float"; if (full_mode) i++;

    if (sse42) {
        funcs [i] = calculate_sse_float; method_names [i] = "float SSE"; if (full_mode) i++;
    }
    if (osxsave && avx) {
        funcs [i] = calculate_avx_float; method_names [i] = "float AVX"; if (full_mode) i++;
    }
    if (osxsave && avx && avx2) {
        funcs [i] = calculate_avx2_float; method_names [i] = "float AVX2"; if (full_mode) i++;
    }
    if (osxsave && avx && avx2 && fma) {
        funcs [i] = calculate_fma_float; method_names [i] = "float FMA"; if (full_mode) i++;
    }
    if (!full_mode) i++;

    funcs [i] = calculate_double; method_names [i] = "double"; if (full_mode) i++;

    if (sse42) {
        funcs [i] = calculate_sse_double; method_names [i] = "double SSE"; if (full_mode) i++;
    }
    if (osxsave && avx) {
        funcs [i] = calculate_avx_double; method_names [i] = "double AVX"; if (full_mode) i++;
    }
    if (osxsave && avx && avx2) {
        funcs [i] = calculate_avx2_double; method_names [i] = "double AVX2"; if (full_mode) i++;
    }
    if (osxsave && avx && avx2 && fma) {
        funcs [i] = calculate_fma_double; method_names [i] = "double FMA"; if (full_mode) i++;
    }
    if (!full_mode) i++;

    func_count = i;
    func_index = 0;
    calculate_func = funcs [func_index];
}

unsigned align(unsigned s)
{
    return (s + 7) & ~7;
}

void init_bi(void)
{
    for (int i = 0; i < 255; i++) {
        bi.colors[i].rgbRed = (BYTE)(i / 64 * 64);
        bi.colors[i].rgbGreen = (BYTE)(i / 8 % 8 * 32);
        bi.colors[i].rgbBlue = (BYTE)(i % 8 * 32);
        bi.colors[i].rgbReserved = 0;
    }
    bi.colors[255].rgbRed      = 0;
    bi.colors[255].rgbGreen    = 0;
    bi.colors[255].rgbBlue     = 0;
    bi.colors[255].rgbReserved = 0;

    bi.h.biSize          = sizeof (BITMAPINFOHEADER);
    bi.h.biWidth         = 0;
    bi.h.biHeight        = 0;
    bi.h.biPlanes        = 1;
    bi.h.biBitCount      = 8;
    bi.h.biCompression   = BI_RGB;
    bi.h.biSizeImage     = 0;
    bi.h.biXPelsPerMeter = 0;
    bi.h.biYPelsPerMeter = 0;
    bi.h.biClrUsed       = 0;
    bi.h.biClrImportant  = 0;
}


void init_borders(LONG sx, LONG sy)
{
    double x0 = -2;
    double x1 = 1;
    double y0 = -1;
    double y1 = 1;
    double xscale = (x1 - x0) / sx;
    double yscale = (y1 - y0) / sy;
    scale = max(xscale, yscale);
    X0 = (x0 + x1) / 2 - sx * scale / 2;
    Y0 = (y0 + y1) / 2 - sy * scale / 2;
    SX = sx;
    SY = sy;
}

void alloc_data()
{
    size_t size = (size_t)align (SX) * (size_t)SY;
    size_t MB = 1024 * 1024;
    size = (size + MB - 1) & ~(MB - 1);
    if (size > data_size) {
        if (data) _mm_free(data);
        data = _mm_malloc (size, 32);
        data_size = size;
    }
}

volatile LONG current_y;
const unsigned n_lines = 16;

DWORD WINAPI thread_proc(LPVOID lpThreadParameter)
{
    unsigned y0, y1;
    unsigned ASX = align(SX);
    while (TRUE) {
        y0 = (unsigned)InterlockedAdd(&current_y, n_lines) - n_lines;
        if (y0 >= SY) return 0;
        y1 = min(y0 + n_lines, SY);
        calculate_func(data + y0 * ASX, X0, Y0, scale, y0, ASX, y1);
    }
}


void calculate_multi(void)
{
    HANDLE threads[64];
    unsigned thread_count;

    DWORD_PTR process_mask = 0;
    DWORD_PTR system_mask = 0;
    GetProcessAffinityMask(GetCurrentProcess(), &process_mask, &system_mask);
    thread_count = processor_count;
    current_y = 0;
    for (unsigned i = 0; i < thread_count; i ++) {
        threads[i] = CreateThread(0, 0, thread_proc, 0, 0, 0);
    }
    WaitForMultipleObjects(thread_count, threads, TRUE, INFINITE);
    for (unsigned i = 0; i < thread_count; i++) {
        CloseHandle(threads[i]);
    }
}


void redraw(HWND hwnd, HDC dc)
{
    DWORD t1, t2;
    char s[2000];

    unsigned ASX = align(SX);

    if (!image_valid) {
        alloc_data();
        t1 = GetTickCount ();
        calculate_multi();
        t2 = GetTickCount();
        sprintf (s, "Mandelbrot Set %s; Image: %d x %d; (%f, %f, d=%g) time: %d ms; %d cores; %5.1f FPS%s", method_names [func_index], ASX, SY,
            X0, Y0, scale,
            (t2 - t1),
            processor_count,
            1000.0 / (t2 - t1),
            full_mode ? "; ALL MODES" : "");
        SetWindowText(hwnd, s);
        image_valid = TRUE;
    }
    bi.h.biWidth = ASX;
    bi.h.biHeight = -(LONG)SY;
    SetDIBitsToDevice(dc, 0, 0, ASX, SY, 0, 0, 0, SY, data, (BITMAPINFO*)&bi, DIB_RGB_COLORS);
}

void set_scale(double new_scale)
{
    X0 += SX * (scale - new_scale) / 2;
    Y0 += SY * (scale - new_scale) / 2;
    scale = new_scale;
}

void bigger()
{
    set_scale(scale / 1.1);
}

void smaller()
{
    set_scale(scale * 1.1);
}

void shift(double sx, double sy)
{
    X0 += sx * scale;
    Y0 += sy * scale;
}

void invalidate(HWND hwnd)
{
    RECT rc;
    image_valid = FALSE;
    GetClientRect(hwnd, &rc);
    InvalidateRect(hwnd, &rc, FALSE);
}

LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    RECT rc;
    HDC dc;
    PAINTSTRUCT ps;
    int n;

    switch (msg) {
    case WM_CREATE:
        GetClientRect(hwnd, &rc);
        init_borders(rc.right, rc.bottom);
        return 0;
    case WM_PAINT:
        dc = BeginPaint(hwnd, &ps);
        redraw(hwnd, dc);
        EndPaint(hwnd, &ps);
        return 0;
    case WM_SIZE:
        GetClientRect(hwnd, &rc);
        SX = rc.right;
        SY = rc.bottom;
        invalidate(hwnd);
        return 0;
    case WM_CLOSE:
        PostQuitMessage(0);
        return 0;
    case WM_KEYDOWN:
        switch (wparam) {
        case VK_RIGHT:
            shift(SX / 10.0, 0);
            invalidate(hwnd);
            break;
        case VK_LEFT:
            shift(- (SX / 10.0), 0);
            invalidate(hwnd);
            break;
        case VK_DOWN:
            shift(0, SY / 10.0);
            invalidate(hwnd);
            break;
        case VK_UP:
            shift(0, - (SY / 10.0));
            invalidate(hwnd);
            break;
        }
        return 0;
    case WM_CHAR:
        switch (wparam) {
        case '+':
            bigger();
            invalidate(hwnd);
            break;
        case '-':
            smaller();
            invalidate(hwnd);
            break;
        case ' ':
            func_index++;
            if (func_index == func_count) func_index = 0;
            calculate_func = funcs[func_index];
            invalidate(hwnd);
            break;
        case '\r':
            full_mode = !full_mode;
            init_functions ();
            invalidate (hwnd);
            break;
        }
        return 0;
    case WM_MOUSEWHEEL:
        n = (int16_t)(wparam >> 16) / 120;
        while (n > 0) {
            bigger();
            --n;
        }
        while (n < 0) {
            smaller();
            ++n;
        }
        invalidate(hwnd);
        return 0;
    case WM_LBUTTONDOWN:
        mousedown = TRUE;
        GetCursorPos(&cursor_pos);
        SetCapture(hwnd);
        SetCursor(LoadCursor(NULL, IDC_HAND));
        return 0;
    case WM_LBUTTONUP:
        mousedown = FALSE;
        ReleaseCapture();
        SetCursor(LoadCursor(NULL, IDC_ARROW));
        return 0;
    case WM_MOUSEMOVE:
        if (mousedown) {
            POINT p;
            GetCursorPos(&p);
            shift(cursor_pos.x - p.x, cursor_pos.y - p.y);
            cursor_pos = p;
            invalidate(hwnd);
        }
        break;
    }

    return DefWindowProc(hwnd, msg, wparam, lparam);
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR CmdLine, int nShow)
{
    WNDCLASS wc;
    HWND wnd;
    MSG msg;

    init_threads ();
    init_functions ();

    wc.style = 0;
    wc.lpfnWndProc = MainWndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInst;
    wc.hIcon = 0;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = CreateSolidBrush(RGB(0, 0, 0));
    wc.lpszMenuName = 0;
    wc.lpszClassName = "MandClass";
    if (!RegisterClass(&wc)) return 1;

    init_bi();

    wnd = CreateWindow("MandClass",
          "M-View",
          WS_OVERLAPPEDWINDOW | WS_VISIBLE,
          CW_USEDEFAULT, CW_USEDEFAULT,
          CW_USEDEFAULT, CW_USEDEFAULT,
          0,
          0,
          hInst,
          0);
    if (!wnd) return 1;
    
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}
