#if !defined (LC_MEMORY_H)
#define LC_MEMORY_H

struct lc_memory {
	int storage_size;
	void* storage;

	int temp_storage_size;
	void* temp_storage;

	// For temp storage
	size_t used_temp_size;
};

void* memory_push_to_temp_ (lc_memory* memory, size_t size);
#define memory_push_to_temp(memory, type, count) memory_push_to_temp_ (memory, sizeof (type) * count)

#endif