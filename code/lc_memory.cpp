#include "lc_memory.h"

void* memory_push_to_temp_ (lc_memory* memory, size_t size) { 
	// We do not want to fail here, it is better to increase the storage size
	// so that the app never crashes
	if (memory -> used_temp_size + size > memory -> temp_storage_size)
		return 0;

	void* ptr = (size_t*)memory -> temp_storage + memory -> used_temp_size;
	memory -> used_temp_size += size;

	return ptr;
}
