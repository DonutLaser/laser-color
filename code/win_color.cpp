#include <windows.h>
#include <stdio.h>

#include "lc_platform.h"

#define WINDOW_HEIGHT 256 
#define WINDOW_WIDTH  320

// Get rid of these globals someday 
static HANDLE global_log_file;

static HANDLE platform_open_file (const char* file_name) {
	HANDLE file_handle = CreateFile (file_name, GENERIC_READ | GENERIC_WRITE,
									 FILE_SHARE_READ, NULL, 
									 OPEN_ALWAYS, 
									 FILE_ATTRIBUTE_NORMAL,
									 NULL);

	if (file_handle == INVALID_HANDLE_VALUE) {
		// Does this ever fail with OPEN_ALWAYS ??? 
		return NULL;
	}

	return file_handle;
}

static void platform_close_file (HANDLE file_handle) {
	CloseHandle (file_handle);
}

static void platform_clear_file (HANDLE file_handle) {
	LARGE_INTEGER offset = { };
	SetFilePointerEx (file_handle, offset, NULL, FILE_BEGIN);
	SetEndOfFile (file_handle);
}

static bool platform_write_file (HANDLE file_handle, const char* text, int size, write_mode mode = WM_OVERWRITE) {
	 if (mode == WM_OVERWRITE)
		 platform_clear_file (file_handle);

	DWORD bytes_written = 0;
	bool success = WriteFile (file_handle, text, size, &bytes_written, NULL);

	return success;
}

static bool platform_read_file (HANDLE file_handle, char** text) {
	DWORD file_size	= GetFileSize (file_handle, NULL);
	*text = (char*)calloc (file_size + 1, sizeof (char));

	DWORD read_bytes;
	bool success = ReadFile (file_handle, *text, file_size + 1, &read_bytes, NULL);
	(*text)[file_size] = '\0';

	return success;
}

static void platform_log (const char* format, ...) {
	char message[2048];
	int bytes_written = 0;

	va_list arguments;
	va_start (arguments, format);

	bytes_written = _vsnprintf_s (message, sizeof (message) - 1, format, arguments);

	if (bytes_written > 0)
		platform_write_file (global_log_file, message, bytes_written, WM_APPEND);

	va_end (arguments);
}

// static laser_rect RECT_to_laser_rect (RECT rect) {
// 	laser_rect new_rect = { };
// 	new_rect.x = rect.left;
// 	new_rect.y = rect.top;
// 	new_rect.width = rect.right - rect.left;
// 	new_rect.height = rect.bottom - rect.top;

// 	return new_rect;
// }

// static RECT laser_rect_to_RECT (laser_rect rect) {
// 	RECT new_rect = { };
// 	new_rect.left = rect.x;
// 	new_rect.top = rect.y;
// 	new_rect.right = rect.x + rect.width;
// 	new_rect.bottom = rect.y + rect.height;

// 	return new_rect;
// }

static void remove_file_name_from_path (char* path, DWORD size) {
	for (int i = size; i > 0; --i) {
		if (path[i] == '/' || path[i] == '\\') {
			path[i + 1] = '\0';
			return;
		}
	}
}

// static void render (HWND window, platform_render_queue* queue) {
// 	PAINTSTRUCT paint_info;
// 	HDC context = BeginPaint (window, &paint_info); {
// 		SetBkMode (context, TRANSPARENT);
// 		for (int i = 0; i < queue -> count; ++i) {
// 			laser_render_data* command = (laser_render_data*)queue -> entries[i];

// 			if (command -> type == RT_RECT) {
// 				HBRUSH brush = CreateSolidBrush (RGB (command -> color.r,
// 													  command -> color.g,
// 													  command -> color.b));

// 				laser_rect* rect_data = (laser_rect*)command -> data;
// 				RECT rect = laser_rect_to_RECT (*rect_data);

// 				FillRect (context, &rect, brush);
// 				DeleteObject (brush);
// 			}
// 			else if (command -> type == RT_TEXT) {
// 				SetTextColor (context, RGB (command -> color.r,
// 											command -> color.g,
// 											command -> color.b));

// 				laser_render_text_data* text_data = (laser_render_text_data*)command -> data;
// 				RECT rect = laser_rect_to_RECT (text_data -> rect);

// 				SelectObject (context, fonts[text_data -> font]);

// 				DRAWTEXTPARAMS text_params = { };
// 				text_params.cbSize = sizeof (text_params);
// 				text_params.iTabLength = 4;
// 				DrawTextEx (context, text_data -> text, text_data -> text_size,
// 							&rect,
// 							DT_TOP | DT_LEFT | DT_EXPANDTABS | DT_TABSTOP | DT_NOPREFIX,
// 							&text_params);
// 			}
// 		}
// 	}
// 	EndPaint (window, &paint_info);
// }

void platform_push_to_render_queue (platform_render_queue* queue, void* entry) {
	queue -> entries[queue -> count++] = entry;
}

static LRESULT CALLBACK window_proc (HWND window, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg) {
		case WM_DESTROY: {
			PostQuitMessage (0);
			return 0;
		}
	}

	return DefWindowProc (window, msg, wParam, lParam);
}


int CALLBACK WinMain (HINSTANCE hInstance, HINSTANCE prevInstance,
					  LPSTR cmdLine, int cmdShow) {

	// Open the file here to be able to log everything
	global_log_file = platform_open_file ("output.log");
	platform_clear_file (global_log_file);

	platform_log ("Opening Laser Color Picker...\n\n");

	WNDCLASS wndClass = { };
	wndClass.style = CS_HREDRAW | CS_OWNDC | CS_VREDRAW;
	wndClass.lpfnWndProc = window_proc;
	wndClass.hInstance = hInstance;
	wndClass.lpszClassName = "Laser Color";

	if (RegisterClass (&wndClass)) {
		platform_log ("Creating a window of size %dx%d\n", WINDOW_WIDTH, WINDOW_HEIGHT);
		HWND window = CreateWindow ("Laser Color", "Laser Color",
									WS_OVERLAPPED | WS_CAPTION | 
										WS_SYSMENU | WS_MINIMIZEBOX | 
										WS_MAXIMIZEBOX,
									CW_USEDEFAULT, CW_USEDEFAULT,
									WINDOW_WIDTH, WINDOW_HEIGHT,
									0, 0,
									hInstance, 
									0);

		if (window) {
			ShowWindow (window, cmdShow);

			BOOL running = TRUE;
			while (running) {
				MSG msg;
				running = GetMessage (&msg, 0, 0, 0);

				if (!running)
					break;

				TranslateMessage (&msg);
				DispatchMessage (&msg);
			}
		}
		else
			platform_log ("Windows couldn't create a window. Aborting...\n");
	}

	platform_log ("Closing Laser Color Picker...\n");

	platform_close_file (global_log_file);
	return 0;
}