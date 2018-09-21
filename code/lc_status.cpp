#include "lc_status.h"

#include <stdio.h>
#include <stdarg.h>

void status_show (status* status_bar, const char* format, ...) {
	for (int i = 0; i < 128; ++i)
		status_bar -> message[i] = 0;

	status_bar -> elapsed_time = 0.0; 

	va_list arguments;
	va_start (arguments, format);

	int bytes_written = _vsnprintf_s (status_bar -> message, sizeof (status_bar -> message) - 1, format, arguments);
	status_bar -> show_message = bytes_written > 0;

	va_end (arguments);
}

void status_update (status* status_bar, double delta_time) {
	if (!status_bar -> show_message)
		return;

	status_bar -> elapsed_time += delta_time;

	if (status_bar -> elapsed_time >= MESSAGE_TIME)
		status_bar -> show_message = false;
}
