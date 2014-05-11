// memory management
#include "bloodpill.h"

using namespace std;

typedef struct
{
	char  *name;
	char  *file;
	int    line;
	size_t size;
	void  *ptr;
	bool   free;
}memsentinel;

bool                initialized = false;
size_t              total_allocated;
size_t              total_active;
size_t              total_active_peak;
vector<memsentinel> sentinels;
HANDLE              sentinelMutex = NULL;
HANDLE              sentinelMutex2 = NULL;

/*
==========================================================================================

	DYNAMIC MEMORY

==========================================================================================
*/

void Mem_Error(char *message_format, ...) 
{
	char msg[16384];
	va_list argptr;

	va_start(argptr, message_format);
	vsprintf(msg, message_format, argptr);
	va_end(argptr);
	Error(msg);
}

void Mem_Init(void)
{
	if (initialized)
		return;

	initialized = true;
	total_allocated = 0;
	total_active = 0;
	total_active_peak = 0;

	sentinels.clear();
	sentinelMutex = CreateMutex(NULL, FALSE, NULL);
	sentinelMutex2 = CreateMutex(NULL, FALSE, NULL);
}

void Mem_Shutdown(void)
{
	if (!initialized)
		return;
	if (memstats)
	{
		size_t leaks = 0;
		size_t leaked = 0;
		for (std::vector<memsentinel>::iterator s = sentinels.begin(); s < sentinels.end(); s++)
		{
			if (!s->free)
			{
				leaked += s->size;
				leaks;
			}
		}

		Print("----------------------------------------\n");
		Print(" Dynamic memory usage stats\n");
		Print("----------------------------------------\n");
		Print("        Peak allocated: %.3f Mb\n", (double)total_active_peak / 1048576.0 );
		Print("       total allocated: %.3f Mb\n", (double)total_allocated / 1048576.0 );
		Print("                 leaks: %.3f Mb\n", (double)leaked / 1048576.0 );
		if (leaks)
		{
			Print("            leak spots: %i\n", leaks );
			Print("\n");
			for (std::vector<memsentinel>::iterator s = sentinels.begin(); s < sentinels.end(); s++)
				if (!s->free)
					Print("%s:%i (%s) %i bytes (%.3f Mb)\n", s->file, s->line, s->name, s->size, (double)s->size / 1048576.0);
		}
		Print("\n");
	}
	initialized = false;
	sentinels.clear();
}

void _mem_sentinel(char *name, void *ptr, size_t size, char *file, int line)
{
	if (!memstats)
		return;

	WaitForSingleObject(sentinelMutex, INFINITE);

	// create sentinel
	// pick free one or allocate
	bool picked = false;
	for (std::vector<memsentinel>::iterator s = sentinels.begin(); s < sentinels.end(); s++)
	{
		if (s->free)
		{
			s->name = name;
			s->file = file;
			s->line = line;
			s->size = size;
			s->ptr = ptr;
			s->free = false;
			picked = true;
			break;
		}
	}
	if (!picked)
	{
		memsentinel s;
		s.name = name;
		s.file = file;
		s.line = line;
		s.size = size;
		s.ptr  = ptr;
		s.free = false;
		sentinels.push_back(s);
	}

	// pop global stats
	total_allocated += size;
	total_active += size;
	if (total_active > total_active_peak)
		total_active_peak = total_active;

	ReleaseMutex(sentinelMutex);
}

bool _mem_sentinel_free(char *name, void *ptr, char *file, int line)
{
	if (!memstats)
		return true;

	WaitForSingleObject(sentinelMutex2, INFINITE);
	// find sentinel for pointer
	int found = -1;
	for (std::vector<memsentinel>::iterator s = sentinels.begin(); s < sentinels.end(); s++)
	{
		if (s->ptr == ptr)
		{
			if (!s->free)
			{
				// throw sentinel
				total_active -= s->size;
				s->size = 0;
				s->file = 0;
				s->line = 0;
				s->name = 0;
				s->ptr = 0;
				s->free = true;
				found = 1;
			}
			else
			{
				// already freed
				found = -2;
			}
			break;
		}
	}
	ReleaseMutex(sentinelMutex2);
	// oops, this pointer was not allocated
	if (found == 1)
		return true;
	if (found == -1)
		Mem_Error("%s:%i (%s) - trying to free non-allocated page %i (sentinel not found)\n", name, file, line, ptr);
	if (found == -2)
		Mem_Error("%s:%i (%s) - trying to free non-allocated page %i (sentinel already freed)\n", name, file, line, ptr);
	return false;
}

void *_mem_realloc(void *data, size_t size, char *file, int line)
{
	if (size <= 0)
		return NULL;
	if (!_mem_sentinel_free("mem_realloc", data, file, line))
		return NULL;
	data = realloc(data, size);
	if (!data)
		Mem_Error("%s:%i - error reallocating %d bytes (%.2f Mb)\n", file, line, size, (double)size / 1048576.0);
	if (!initialized)
		return data;
	_mem_sentinel("mem_realloc", data, size, file, line);
	return data;
}

void *_mem_alloc(size_t size, char *file, int line)
{
	void *data;

	if (size <= 0)
		return NULL;
	data = malloc(size);
	if (!data)
		Mem_Error("%s:%i - error allocating %d bytes (%.2f Mb)\n", file, line, size, (double)size / 1048576.0);
	if (!initialized)
		return data;
	_mem_sentinel("mem_alloc", data, size, file, line);
	return data;
}

void _mem_calloc(void **bufferptr, size_t size, char *file, int line)
{
	void *data;

	if (size <= 0)
		return;
	data = malloc(size);
	memset(data, 0, size);
	if (!data)
		Mem_Error("%s:%i - error allocating %d bytes (%.2f Mb)\n", file, line, size, (double)size / 1048576.0);
	if (!initialized)
		return;
	_mem_sentinel("mem_calloc", data, size, file, line);
	*bufferptr = data;
}

void _mem_free(void *data, char *file, int line)
{
	if (!data)
		return;
	if (!initialized)
	{
		free(data);
		return;
	}
	if (!_mem_sentinel_free("mem_free", data, file, line))
		return;
	free(data);
}

/*
==========================================================================================

	AUTO-EXPANDABLE MEMORY BUFFERS

==========================================================================================
*/

// bcreate()
// creates new buffer
MemBuf_t *bcreate(int size)
{
	MemBuf_t *b;

	b = (MemBuf_t *)mem_alloc(sizeof(MemBuf_t));
	memset(b, 0, sizeof(MemBuf_t));
	if (size)
	{
		b->ptr = b->buffer = (byte *)mem_alloc(size);
		b->size = size;
	}
	return b;
}

// bgrow()
// checks if buffer can take requested amount of data, if not - expands
void bgrow(MemBuf_t *b, int morebytes)
{
	int newused;

	newused = b->used + morebytes;
	if (newused > b->size)
	{
		b->size = newused + MEMBUF_GROW;
		if (b->buffer)
			b->buffer = (byte *)mem_realloc(b->buffer, b->size);
		else
			b->buffer = (byte *)mem_alloc(b->size);
		b->ptr = b->buffer + b->used;
	}
}

// bputlitleint()
// puts a little int to buffer
void bputlittleint(MemBuf_t *b, int n)
{
	bgrow(b, 4);
	b->ptr[0] = n & 0xFF;
	b->ptr[1] = (n >> 8) & 0xFF;
	b->ptr[2] = (n >> 16) & 0xFF;
	b->ptr[3] = (n >> 24) & 0xFF;
	b->used += 4;
	b->ptr += 4;
}

// bputlittlefloat()
// puts a little float to buffer
void bputlittlefloat(MemBuf_t *b, float n)
{
	union {int i; float f;} in;
	in.f = n;
	bputlittleint(b, in.i);
}

// bwrite()
// writes arbitrary data grow buffer
void bwrite(MemBuf_t *b, void *data, int size)
{
	bgrow(b, size);
	memcpy(b->ptr, data, size);
	b->used += size;
	b->ptr += size;
}

// brelease()
// returns size of used data of buffer, fills bufferptr and frees temp data
int brelease(MemBuf_t *b, void **bufferptr)
{
	int bufsize;
	
	*bufferptr = b->buffer;
	bufsize = b->used;
	mem_free(b);
	return bufsize;
}

// bfree()
// free buffer and stored data
void bfree(MemBuf_t *b, byte **bufferptr)
{
	if (b->buffer)
		mem_free(b->buffer);
	mem_free(b);
}