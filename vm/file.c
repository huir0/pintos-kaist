/* file.c: Implementation of memory backed file object (mmaped object). */

#include "vm/vm.h"
#include "threads/vaddr.h"
#include "userprog/process.h"

static bool file_backed_swap_in (struct page *page, void *kva);
static bool file_backed_swap_out (struct page *page);
static void file_backed_destroy (struct page *page);

/* DO NOT MODIFY this struct */
static const struct page_operations file_ops = {
	.swap_in = file_backed_swap_in,
	.swap_out = file_backed_swap_out,
	.destroy = file_backed_destroy,
	.type = VM_FILE,
};

/* The initializer of file vm */
void
vm_file_init (void) {
}

/* Initialize the file backed page */
bool
file_backed_initializer (struct page *page, enum vm_type type, void *kva) {
	struct segment *aux = (struct segment *)page->uninit.aux;
	/* Set up the handler */
	page->operations = &file_ops;

	struct file_page *file_page = &page->file;
}

/* Swap in the page by read contents from the file. */
static bool
file_backed_swap_in (struct page *page, void *kva) {
	struct file_page *file_page UNUSED = &page->file;
}

/* Swap out the page by writeback contents to the file. */
static bool
file_backed_swap_out (struct page *page) {
	struct file_page *file_page UNUSED = &page->file;
}

/* Destory the file backed page. PAGE will be freed by the caller. */
static void
file_backed_destroy (struct page *page) {
	struct file_page *file_page UNUSED = &page->file;
	free(page->frame);
}

/* Do the mmap */
void *
do_mmap (void *addr, size_t length, int writable, struct file *file, off_t offset) {
	void *return_value = addr;
	struct file *reopen_file = file_reopen(file);
	if(reopen_file == NULL) {
		return NULL;
	}
	size_t file_size = (size_t)file_length(reopen_file);
	size_t read_bytes = length > file_size ? file_size : length; 

	while(read_bytes > 0) {
		size_t page_read_bytes = read_bytes < PGSIZE ? read_bytes : PGSIZE;
		size_t page_zero_bytes = PGSIZE - page_read_bytes;

		struct segment *aux = (struct segment *)malloc(sizeof(struct segment));
		aux->file = file;
		aux->ofs = offset;
		aux->read_bytes = page_read_bytes;
		if (!vm_alloc_page_with_initializer(VM_FILE, addr, writable, lazy_load_segment, aux)) {
			free(aux);
			return NULL;
		}

		/* Advance. */
		read_bytes -= page_read_bytes;
		offset += page_read_bytes;
		addr += PGSIZE;
	}

	return return_value;
}

/* Do the munmap */
void
do_munmap (void *addr) {
	struct thread *cur = thread_current();
	struct supplemental_page_table *spt = &cur->spt;
	struct page *page = spt_find_page(spt, addr);
	if(page == NULL) {
		return ;
	}
	while(page != NULL) {
		struct segment *seg = (struct segment *)page->uninit.aux;
		if(pml4_is_dirty(cur->pml4, addr)) {
			lock_acquire(&filesys_lock);
			file_write_at(seg->file, addr, seg->read_bytes, seg->ofs);
			lock_release(&filesys_lock);
			pml4_set_dirty(cur->pml4, addr, 0);
		}
		pml4_clear_page(cur->pml4, page->va);
		addr += PGSIZE;
		page = spt_find_page(&cur->spt, addr);
	}
}
