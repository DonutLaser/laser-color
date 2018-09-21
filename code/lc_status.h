#if !defined (LC_STATUS_H)
#define LC_STATUS_H

#define MESSAGE_TIME 2.0

struct status {
	char message[128];
	double elapsed_time;
	bool show_message;
};

void status_show (status* status_bar, const char* format, ...);
void status_update (status* status_bar, double delta_time);

#endif