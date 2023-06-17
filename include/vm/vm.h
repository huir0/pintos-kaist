#ifndef VM_VM_H
#define VM_VM_H
#include <stdbool.h>
#include "threads/palloc.h"
#include "lib/kernel/hash.h"

enum vm_type {
	/* page not initialized */
	VM_UNINIT = 0,
	/* page not related to the file, aka anonymous page */
	VM_ANON = 1,
	/* page that realated to the file */
	VM_FILE = 2,
	/* page that hold the page cache, for project 4 */
	VM_PAGE_CACHE = 3,

	/* Bit flags to store state */
	/* Auxillary bit flag marker for store information. You can add more
	 * markers, until the value is fit in the int. */
	VM_MARKER_0 = (1 << 3),
	VM_MARKER_1 = (1 << 4),
	VM_MARKER_2 = (1 << 5),
	
	VM_STACK =  (VM_MARKER_0 | VM_ANON),
	VM_FILE_SWAP = (VM_FILE | VM_MARKER_1),
	VM_ANON_SWAP = (VM_ANON | VM_MARKER_2),
	

	/* DO NOT EXCEED THIS VALUE. */
	VM_MARKER_END = (1 << 31),
};

#include "vm/uninit.h"
#include "vm/anon.h"
#include "vm/file.h"
#ifdef EFILESYS
#include "filesys/page_cache.h"
#endif

struct page_operations;
struct thread;

#define VM_TYPE(type) ((type) & 7)

/* The representation of "page".
 * This is kind of "parent class", which has four "child class"es, which are
 * uninit_page, file_page, anon_page, and page cache (project4).
 * DO NOT REMOVE/MODIFY PREDEFINED MEMBER OF THIS STRUCTURE. */
struct page {
	const struct page_operations *operations;
	void *va;              /* A ddress in terms of user space */
	struct frame *frame;   /* Back reference for frame */

	/* Your implementation */
	bool is_loaded;
	struct file *_file;
	struct list_elem mmap_elem;
	off_t offset;
	size_t read_bytes;
	size_t zero_bytes;
	size_t swap_slot;
	struct hash_elem elem;
	bool writable;

	/* Per-type data are binded into the union.
	 * Each function automatically detects the current union */
	union {
		struct uninit_page uninit;
		struct anon_page anon;
		struct file_page file;
#ifdef EFILESYS
		struct page_cache page_cache;
#endif
	};
};

/* The representation of "frame" */
struct frame {
	void *kva;
	struct page *page;
};

/* The function table for page operations.
 * This is one way of implementing "interface" in C.
 * Put the table of "method" into the struct's member, and
 * call it whenever you needed. */
struct page_operations {
	bool (*swap_in) (struct page *, void *);
	bool (*swap_out) (struct page *);
	void (*destroy) (struct page *);
	enum vm_type type;
};

#define swap_in(page, v) (page)->operations->swap_in ((page), v) 
#define swap_out(page) (page)->operations->swap_out (page)
#define destroy(page) \
	if ((page)->operations->destroy) (page)->operations->destroy (page)

/* Representation of current process's memory space.
 * We don't want to force you to obey any specific design for this struct.
 * All designs up to you for this. */
struct supplemental_page_table {
	/*각각의 페이지에 대해서 데이터가 존재하는 곳(frame, disk, swap 중 어디에 존재하는지), 
	이에 상응하는 커널 가상주소를 가리키는 포인터 정보, active인지 inactive 인지 등 
	보조 페이지 테이블은 최소한 두 가지 목적으로 쓰입니다. 
	가장 중요하게는, 페이지 폴트가 발생했을 때 그곳에 어떤 데이터가 있었어야 했는지를 알아내기 위해 
	커널은 보조 페이지 테이블에서 폴트가 발생한 가상 페이지를 탐색합니다. 
	두번째로는 커널이 프로세스가 종료될 때 어떤 자원을 해제(free)할지 고르기 위해서 보조 페이지 테이블을 조사합니다.*/
	// struct hash hash;	
	struct hash hash;	
};

#include "threads/thread.h"
void supplemental_page_table_init (struct supplemental_page_table *spt);
bool supplemental_page_table_copy (struct supplemental_page_table *dst,
		struct supplemental_page_table *src);
void supplemental_page_table_kill (struct supplemental_page_table *spt);
struct page *spt_find_page (struct supplemental_page_table *spt,
		void *va);
bool spt_insert_page (struct supplemental_page_table *spt, struct page *page);
void spt_remove_page (struct supplemental_page_table *spt, struct page *page);

void vm_init (void);
bool vm_try_handle_fault (struct intr_frame *f, void *addr, bool user,
		bool write, bool not_present);
static struct frame * vm_get_frame (void);
#define vm_alloc_page(type, upage, writable) \
	vm_alloc_page_with_initializer ((type), (upage), (writable), NULL, NULL)
bool vm_alloc_page_with_initializer (enum vm_type type, void *upage,
		bool writable, vm_initializer *init, void *aux);
void vm_dealloc_page (struct page *page);
bool vm_claim_page (void *va);
enum vm_type page_get_type (struct page *page);

#endif  /* VM_VM_H */
