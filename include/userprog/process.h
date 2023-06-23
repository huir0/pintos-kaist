#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H

#include "threads/thread.h"

tid_t process_create_initd (const char *file_name);
tid_t process_fork (const char *name, struct intr_frame *if_);
int process_exec (void *f_name);
int process_wait (tid_t);
void process_exit (void);
void process_activate (struct thread *next);
int process_add_file (struct file *f);
struct file *process_get_file(int fd);
void process_close_file(int fd);
void remove_child_process(struct thread *cp);

bool
lazy_load_segment(struct page *page, void *aux);

struct lazy{
	struct file *file_;
	off_t offset;
	uint8_t *upage;
	size_t read_bytes; 
	size_t zero_bytes; 
	bool writable;
};


#endif /* userprog/process.h */
