#include <windows.h>
#include <windowsx.h>
#include <shlobj.h>
#include <stdio.h>
#include <gl/gl.h>

#include "lc_platform.h"
#include "lc.h"
#include "lc_memory.h"
#include "lc_opengl.h"

#define WINDOW_HEIGHT 		351 
#define WINDOW_WIDTH 		256 
#define INITIAL_WINDOW_X	888	
#define INITIAL_WINDOW_Y	368	

#define Kilobytes(x) ((x * 1024))
#define Megabytes(x) ((Kilobytes(x) * 1024))
#define Gigabytes(x) ((Megabytes(x) * 1024))

// Get rid of these globals someday 
static HANDLE global_log_file;
static vector2 title_bar_size;

static int string_length (const char* str) {
	int result = 0;
	while (*str != '\0') {
		++result;
		++str;
	}

	return result;
}

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

static bool platform_read_file (HANDLE file_handle, char** text, unsigned* size) {
	DWORD file_size	= GetFileSize (file_handle, NULL);
	*text = (char*)calloc (file_size + 1, sizeof (char));

	DWORD read_bytes;
	bool success = ReadFile (file_handle, *text, file_size + 1, &read_bytes, NULL);
	(*text)[file_size] = '\0';

	if (size != NULL)
		*size = (int)file_size;

	return success;
}

static void platform_log (bool new_line, const char* format, ...) {
	char message[2048];
	int bytes_written = 0;

	va_list arguments;
	va_start (arguments, format);

	bytes_written = _vsnprintf_s (message, sizeof (message) - 1, format, arguments);

	if (new_line)
		message[bytes_written++] = '\n';

	if (bytes_written > 0)
		platform_write_file (global_log_file, message, bytes_written, WM_APPEND);

	va_end (arguments);
}

static void platform_copy_to_clipboard (const char* text) {
	int size = string_length (text) + 1;

	HANDLE clipboard_memory_handle = GlobalAlloc (GMEM_MOVEABLE, size);
	void* memory = GlobalLock (clipboard_memory_handle);

	for (int i = 0; i < size; ++i)
		((char*)memory)[i] = text[i];

	GlobalUnlock (clipboard_memory_handle);

	OpenClipboard (0); {
		EmptyClipboard ();
		SetClipboardData (CF_TEXT, clipboard_memory_handle);
	}
	CloseClipboard ();
}

static void platform_close_application () {
	HWND window = GetActiveWindow ();
	PostQuitMessage (0);
}

static void platform_minimize_application () {
	HWND window = GetActiveWindow ();
	ShowWindow (GetActiveWindow (), SW_MINIMIZE);
}

static void platform_move_window (vector2 new_position) {
	HWND window = GetActiveWindow ();
	SetWindowPos (window, 0, new_position.x, new_position.y, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
}

static void platform_get_window_position (vector2* result) {
	HWND window = GetActiveWindow ();
	RECT client_rect;
	GetWindowRect (window, &client_rect);

	*result = { client_rect.left, client_rect.top };
}

static bool initialize_open_gl (HWND window, vector2 client_size) {
	HDC device_context = GetDC (window);
	PIXELFORMATDESCRIPTOR format = { };
	format.nSize = sizeof (format);
	format.nVersion = 1;
	format.iPixelType = PFD_TYPE_RGBA;
	format.dwFlags = PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW | PFD_DOUBLEBUFFER;
	format.cColorBits = 32;
	format.cAlphaBits = 8;
	format.iLayerType = PFD_MAIN_PLANE;

	int format_index = ChoosePixelFormat (device_context, &format);
	PIXELFORMATDESCRIPTOR suggested_format;
	DescribePixelFormat (device_context, format_index, 
						 sizeof (suggested_format), &suggested_format);
	SetPixelFormat (device_context, format_index, &suggested_format);

	HGLRC gl_context = wglCreateContext (device_context);
	if (!wglMakeCurrent (device_context, gl_context)) {
		platform_log (true, "Wasn't able to initialize OpenGL rendering context. Exiting.");
		return false;
	}

	ReleaseDC (window, device_context);

	opengl_init (client_size);

	return true;
}

static LRESULT CALLBACK window_proc (HWND window, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg) {
		case WM_DESTROY: {
			PostQuitMessage (0);
			return 0;
		}
		case WM_NCHITTEST: {
			LRESULT hit = DefWindowProc (window, msg, wParam, lParam);

			POINT pt = { GET_X_LPARAM (lParam), GET_Y_LPARAM (lParam) };
			ScreenToClient (window, &pt);

			RECT title_bar = { };
			title_bar.top = 0;
			title_bar.left = 0;
			title_bar.right = title_bar_size.x;
			title_bar.bottom = title_bar_size.y;
			if (hit == HTCLIENT && PtInRect (&title_bar, pt))
				return HTCAPTION;

			return hit;
		}
		case WM_NCCALCSIZE: { 
			// The key to making a window borderless and have proper animations,
			// have the ability to minimize it with Win + M or clicking in the
			// task bar. WS_POPUP window style doesn't allow these.
			// Animations can be worked around, but minimization cannot.
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

	platform_log (true, "Opening Laser Color Picker...");

	WNDCLASS wndClass = { };
	wndClass.style = CS_HREDRAW | CS_VREDRAW;
	wndClass.lpfnWndProc = window_proc;
	wndClass.hInstance = hInstance;
	wndClass.hIcon = LoadIcon (wndClass.hInstance, MAKEINTRESOURCE (101));
	wndClass.lpszClassName = "Laser Color";

	if (RegisterClass (&wndClass)) {
		platform_log (false, "Creating a window of size %dx%d...", WINDOW_WIDTH, WINDOW_HEIGHT);
		HWND window = CreateWindow ("Laser Color", "Laser Color",
									WS_OVERLAPPEDWINDOW,
									INITIAL_WINDOW_X, INITIAL_WINDOW_Y,
									WINDOW_WIDTH, WINDOW_HEIGHT,
									0, 0, hInstance, 0);

		if (window) {
			platform_log (true, " Success.");

			HDC device_context = GetDC (window);

			RECT client_rect;
			GetClientRect (window, &client_rect);
			vector2 client_size = { client_rect.right - client_rect.left, client_rect.bottom - client_rect.top };

			if (initialize_open_gl (window, client_size))
				platform_log (true, "Initialized OpenGL rendering context.");
			else {
				platform_log (true, "Wasn't able to initialize OpenGL rendering context. Exiting.");
				return -1;
			}

			lc_memory app_memory = { };
			app_memory.storage_size = Megabytes (128);
			app_memory.storage = VirtualAlloc (0, app_memory.storage_size, 
											   MEM_RESERVE | MEM_COMMIT,
											   PAGE_READWRITE);

			app_memory.temp_storage_size = Megabytes (256);
			app_memory.temp_storage = VirtualAlloc (0, app_memory.temp_storage_size,
													MEM_RESERVE | MEM_COMMIT,
													PAGE_READWRITE);

			if (!app_memory.storage) {
				platform_log (true, "Could not allocate %d bytes of memory for the application. Exiting.", app_memory.storage_size);
				return 0;
			}

			platform_api api = { };
			api.open_file = platform_open_file;
			api.write_file = platform_write_file;
			api.read_file = platform_read_file;
			api.close_file = platform_close_file;
			api.log = platform_log;
			api.copy_to_clipboard = platform_copy_to_clipboard;
			api.close_application = platform_close_application;
			api.minimize_application = platform_minimize_application;
			api.move_application_window = platform_move_window;
			api.get_window_position = platform_get_window_position;

			char documents_path[PATH_MAX];
			SHGetFolderPath (NULL, CSIDL_MYDOCUMENTS, NULL, SHGFP_TYPE_CURRENT, documents_path);

			app_init (&app_memory, api, client_size, documents_path, &title_bar_size);

			HCURSOR default_cursor = LoadCursor (NULL, IDC_ARROW);
			SetCursor (default_cursor);

			ShowWindow (window, cmdShow);

			BOOL running = TRUE;
			while (running) {
				MSG msg;
				running = GetMessage (&msg, 0, 0, 0);

				if (!running)
					break;

				lc_input input = { };
				input.modifier = M_NONE;
				switch (msg.message) {
					case WM_SYSKEYDOWN:
					case WM_KEYDOWN: {
						if (GetKeyState (VK_CONTROL) & 0x8000)
							input.modifier |= M_CTRL ;
						if (GetKeyState (VK_SHIFT) & 0x8000)
							input.modifier |= M_SHIFT;
						if (GetKeyState (VK_MENU) & 0x8000)
							input.modifier |= M_ALT;
						if (GetKeyState (VK_CAPITAL) & 0x0001)
							input.modifier |= M_CAPS;

						switch (msg.wParam) {
							case VK_F4: {
								if (input.modifier & M_ALT)
									platform_close_application ();

								break;
							}
							default: {
								input.key = (int)msg.wParam; 
								break;
							}
						}
						break;
					}
					case WM_LBUTTONDOWN: {
						input.mouse.lmb_down = true;
						break;
					}
					case WM_LBUTTONUP: {
						input.mouse.lmb_up = true;
						break;
					}
					default: {
						TranslateMessage (&msg);
						DispatchMessage (&msg);
						break;
					}
				}

				// Get mouse position relative to the screen
				POINT mouse_position;
				GetCursorPos (&mouse_position);
				input.mouse.screen_position = { mouse_position.x, mouse_position.y };

				// Get mouse position relative to the application window
				ScreenToClient (window, &mouse_position);
				input.mouse.position = { mouse_position.x, mouse_position.y };

				app_update (&app_memory, input);
				SwapBuffers (device_context);
			}

			ReleaseDC (window, device_context);

			app_close (&app_memory);
		}
		else
			platform_log (true, "Couldn't create a window. Aborting.");
	}

	platform_log (true, "Closing Laser Color Picker...");

	platform_close_file (global_log_file);
	return 0;
}