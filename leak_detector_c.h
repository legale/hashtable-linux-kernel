#ifdef LEAKCHECK
#ifndef  LEAK_DETECTOR_C_H
#define  LEAK_DETECTOR_C_H

#define  FILE_NAME_LENGTH   	   512
#define  OUTPUT_FILE			        "/tmp/leak_info.txt"
#define  malloc(size) 	    	   	xmalloc (size, __FILE__, __LINE__)
#define  calloc(elements, size)  	xcalloc (elements, size, __FILE__, __LINE__)
#define  realloc(ptr, size)  		  xrealloc (ptr, size, __FILE__, __LINE__)
#define  free(mem_ref)            xfree(mem_ref)

struct _MEM_INFO
{
	void			*address;
	unsigned int	size;
	char			file_name[FILE_NAME_LENGTH + 1];
	unsigned int	line;
};
typedef struct _MEM_INFO MEM_INFO;

struct _MEMLEAK {
	MEM_INFO mem_info;
	struct _MEMLEAK *next;
	struct _MEMLEAK *prev;
};
typedef struct _MEMLEAK MEMLEAK;


void * xmalloc(unsigned int size, const char * file, unsigned int line);
void * xrealloc(void *ptr, unsigned int size, const char * file, unsigned int line);
void * xcalloc(unsigned int elements, unsigned int size, const char * file, unsigned int line);
void xfree(void * mem_ref);

void add_mem_info (void * mem_ref, unsigned int size,  const char * file, unsigned int line);
void remove_mem_info (void * mem_ref);
void report_mem_leak(void);

#endif
#endif