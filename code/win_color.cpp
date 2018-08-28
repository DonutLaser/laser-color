#include <windows.h>
#include <shlobj.h>
#include <stdio.h>
#include <gl/gl.h>

#include "lc_platform.h"
#include "lc.h"
#include "lc_memory.h"
#include "lc_opengl.h"

#define WINDOW_HEIGHT 230 
#define WINDOW_WIDTH  302

#define Kilobytes(x) ((x * 1024))
#define Megabytes(x) ((Kilobytes(x) * 1024))
#define Gigabytes(x) ((Megabytes(x) * 1024))

// Get rid of these globals someday 
static HANDLE global_log_file;

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

static void platform_log (const char* format, ...) {
	char message[2048];
	int bytes_written = 0;

	va_list arguments;
	va_start (arguments, format);

	bytes_written = _vsnprintf_s (message, sizeof (message) - 1, format, arguments);
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

static bool initialize_open_gl (HWND window, int client_width, int client_height) {
	HDC device_context = GetDC (window);
	PIXELFORMATDESCRIPTOR format = { };
	format.nSize = sizeof (format);
	format.nVersion = 1;
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
		platform_log ("Wasn't able to initialize OpenGL rendering context.Exiting...\n");
		return false;
	}

	ReleaseDC (window, device_context);

	opengl_init (client_width, client_height);

	return true;
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

	platform_log ("Opening Laser Color Picker...\n");

	WNDCLASS wndClass = { };
	wndClass.style = CS_HREDRAW | CS_OWNDC | CS_VREDRAW;
	wndClass.lpfnWndProc = window_proc;
	wndClass.hInstance = hInstance;
	wndClass.hIcon = LoadIcon (wndClass.hInstance, MAKEINTRESOURCE (101));
	wndClass.lpszClassName = "Laser Color";

	if (RegisterClass (&wndClass)) {
		platform_log ("Creating a window of size %dx%d", WINDOW_WIDTH, WINDOW_HEIGHT);
		HWND window = CreateWindow ("Laser Color", "Laser Color",
									WS_OVERLAPPED | WS_CAPTION | 
										WS_SYSMENU | WS_MINIMIZEBOX,
									CW_USEDEFAULT, CW_USEDEFAULT,
									WINDOW_WIDTH, WINDOW_HEIGHT,
									0, 0,
									hInstance, 
									0);

		if (window) {
			HDC device_context = GetDC (window);

			RECT client_rect;
			GetClientRect (window, &client_rect);
			int client_width = client_rect.right - client_rect.left;
			int client_height = client_rect.bottom - client_rect.top;

			if (initialize_open_gl (window, client_width, client_height))
				platform_log ("Initialized OpenGL rendering context.");
			else {
				platform_log ("Wasn't able to initialize OpenGL rendering context.Exiting...");
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
				platform_log ("Could not allocate %d bytes of memory for the application. Exiting...", app_memory.storage_size);
				return 0;
			}

			platform_api api = { };
			api.open_file = platform_open_file;
			api.write_file = platform_write_file;
			api.read_file = platform_read_file;
			api.close_file = platform_close_file;
			api.log = platform_log;
			api.copy_to_clipboard = platform_copy_to_clipboard;

			char documents_path[PATH_MAX];
			SHGetFolderPath (NULL, CSIDL_MYDOCUMENTS, NULL, SHGFP_TYPE_CURRENT, documents_path);

			app_init (&app_memory, api, client_width, client_height, documents_path);

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
									PostQuitMessage (0);

								break;
							}
							default: {
								input.key = (int)msg.wParam; 
								break;
							}
						}
						break;
					}
					default: {
						TranslateMessage (&msg);
						DispatchMessage (&msg);
						break;
					}
				}

				app_update (&app_memory, input);
				SwapBuffers (device_context);
			}

			ReleaseDC (window, device_context);

			app_close (&app_memory);
		}
		else
			platform_log ("Windows couldn't create a window. Aborting...");
	}

	platform_log ("Closing Laser Color Picker...");

	platform_close_file (global_log_file);
	return 0;
}