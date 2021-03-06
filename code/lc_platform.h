#if !defined (LC_PLATFORM_H)
#define LC_PLATFORM_H

enum write_mode { WM_OVERWRITE, WM_APPEND };

union vector2;

// Setting up the function pointers
#define OPEN_FILE(name) void* name(const char* file_name)
typedef OPEN_FILE(open_file);

#define WRITE_FILE(name) bool name(void* file_handle, const char* text, int size, write_mode mode)
typedef WRITE_FILE(write_file);

#define READ_FILE(name) bool name(void* file_handle, char** text, unsigned* size)
typedef READ_FILE(read_file);

#define CLOSE_FILE(name) void name(void* file_handle)
typedef CLOSE_FILE(close_file);
// Perhaps log shouldn't be specific to a platform?
#define LOG(name) void name(bool new_line, const char* format, ...)
typedef LOG(log_message);

#define COPY_TO_CLIPBOARD(name) void name(const char* text)
typedef COPY_TO_CLIPBOARD(copy_to_clipboard);

#define CLOSE_APP(name) void name()
typedef CLOSE_APP(close_application);

#define MINIMIZE_APP(name) void name()
typedef MINIMIZE_APP(minimize_application);

#define MOVE_WINDOW(name) void name(vector2 new_position)
typedef MOVE_WINDOW(move_window);

#define GET_WINDOW_POSITION(name) void name(vector2* result)
typedef GET_WINDOW_POSITION(get_window_position);

struct platform_api {
	open_file* 				open_file;
	write_file* 			write_file;	
	read_file*				read_file;
	close_file* 			close_file;
	log_message* 			log;

	copy_to_clipboard* 		copy_to_clipboard;

	close_application* 		close_application;
	minimize_application* 	minimize_application;
	move_window*			move_application_window;
	get_window_position*	get_window_position;
};

#endif