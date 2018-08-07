#if !defined (LC_PLATFORM_H)
#define LC_PLATFORM_H

enum write_mode { WM_OVERWRITE, WM_APPEND };

// Setting up the function pointers
#define OPEN_FILE(name) void* name(const char* file_name)
typedef OPEN_FILE(open_file);

#define WRITE_FILE(name) bool name(void* file_handle, const char* text, int size, write_mode mode)
typedef WRITE_FILE(write_file);

#define READ_FILE(name) bool name(void* file_handle, char** text)
typedef READ_FILE(read_file);

#define CLOSE_FILE(name) void name(void* file_handle)
typedef CLOSE_FILE(close_file);
// Perhaps log shouldn't be specific to a platform?
#define LOG(name) void name(const char* format, ...)
typedef LOG(log);

#define COPY_TO_CLIPBOARD(name) void name(const char* text)
typedef COPY_TO_CLIPBOARD(copy_to_clipboard);

struct platform_api {
	open_file* 	open_file;
	write_file* write_file;	
	read_file*	read_file;
	close_file* close_file;
	log*		log;

	copy_to_clipboard* copy_to_clipboard;
};

#endif