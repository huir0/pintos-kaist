/* vm.c: Generic interface for virtual memory objects. */

#include "threads/malloc.h"
#include "vm/vm.h"
#include "vm/inspect.h"
#include "lib/kernel/hash.h"
#include "include/threads/vaddr.h"
#include "threads/mmu.h"
#include "include/userprog/process.h"
/* Initializes the virtual memory subsystem by invoking each subsystem's
 * intialize codes. */
void
vm_init (void) {
	vm_anon_init ();
	vm_file_init ();
#ifdef EFILESYS  /* For project 4 */
	pagecache_init ();
#endif
	register_inspect_intr ();
	/* DO NOT MODIFY UPPER LINES. */
	/* TODO: Your code goes here. */

}

/* Get the type of the page. This function is useful if you want to know the
 * type of the page after it will be initialized.
 * This function is fully implemented now. */
enum vm_type
page_get_type (struct page *page) {
	int ty = VM_TYPE (page->operations->type);
	switch (ty) {
		case VM_UNINIT:
			return VM_TYPE (page->uninit.type);
		default:
			return ty;
	}
}

/* Helpers */
static struct frame *vm_get_victim (void);
static bool vm_do_claim_page (struct page *page);
static struct frame *vm_evict_frame (void);

/* Create the pending page object with initializer. If you want to create a
 * page, do not create it directly and make it through this function or
 * `vm_alloc_page`. */
bool
vm_alloc_page_with_initializer (enum vm_type type, void *upage, bool writable,
		vm_initializer *init, void *aux) {

	ASSERT (VM_TYPE(type) != VM_UNINIT)

	struct supplemental_page_table *spt = &thread_current ()->spt;

	/* Check wheter the upage is already occupied or not. */
	if (spt_find_page (spt, upage) == NULL) {
		/* TODO: Create the page, fetch the initialier according to the VM type,
		 * TODO: and then create "uninit" page struct by calling uninit_new. You
		 * TODO: should modify the field after calling the uninit_new. */
		/* TODO: Insert the page into the spt. */
		struct page *uninit = (struct page*)malloc(sizeof(struct page));
		// uninit->va = upage;
		switch (VM_TYPE(type))
		{
		case VM_ANON:
			uninit_new(uninit, upage, init, type, aux, anon_initializer);
			break;
		case VM_FILE:
			uninit_new(uninit, upage, init, type, aux, file_backed_initializer);
			break;
		default:
			break;
		}
		uninit->writable = writable;
		return spt_insert_page(spt, uninit);
	}
err:
	return false;
}

/* Find VA from spt and return page. On error, return NULL. */
struct page *
spt_find_page (struct supplemental_page_table *spt UNUSED, void *va UNUSED) {
	
	struct page page;
	/* TODO: Fill this function. */
	struct hash_elem *e;
	page.va = pg_round_down(va);
	e = hash_find(&spt->hash, &page.elem);
	if (e) return hash_entry(e, struct page, elem);
	else return NULL;

}

/* Insert PAGE into spt with validation. */
bool
spt_insert_page (struct supplemental_page_table *spt UNUSED,
		struct page *page UNUSED) {
	int succ = false;
	/* TODO: Fill this function. */
	// if(hash_insert(spt->hash, &page->elem))
	// 	succ = true;
	// if (spt_find_page(spt, page->va) != NULL) return false;
	return hash_insert(&spt->hash, &page->elem) == NULL;
}

void
spt_remove_page (struct supplemental_page_table *spt, struct page *page) {
	// hash_delete(spt->hash, &page->elem);
	// vm_dealloc_page (page);
	return true;
}

/* Get the struct frame, that will be evicted. */
static struct frame *
vm_get_victim (void) {
	struct frame *victim = NULL;
	 /* TODO: The policy for eviction is up to you. */
	
	return victim;
}

/* Evict one page and return the corresponding frame.
 * Return NULL on error.*/
static struct frame *
vm_evict_frame (void) {
	struct frame *victim UNUSED = vm_get_victim ();
	/* TODO: swap out the victim and return the evicted frame. */
	// swap_out(victim->page);
	return NULL;
}

/* palloc() and get frame. If there is no available page, evict the page
 * and return it. This always return valid address. That is, if the user pool
 * memory is full, this function evicts the frame to get the available memory
 * space.*/
static struct frame *
vm_get_frame (void) {
	struct frame *frame = NULL;
	/* TODO: Fill this function. */
	frame = malloc(sizeof(struct frame));	
	frame->kva = palloc_get_page(PAL_USER);
	if (frame->kva == NULL){
		// frame->page = NULL;
		PANIC("todo");
	} 
		
	frame->page = NULL;
	ASSERT (frame != NULL);
	ASSERT (frame->page == NULL);
	return frame;
}

/* Growing the stack. */
static void
vm_stack_growth (void *addr UNUSED) {
}

/* Handle the fault on write_protected page */
static bool
vm_handle_wp (struct page *page UNUSED) {
}

/* 페이지 폴트가 일어나면, 페이지 폴트 핸들러는 vm_try_handle_fault 함수에게 제어권을 넘깁니다. 
이 함수는 유효한 페이지 폴트인지를 우선 검사합니다. 
이 페이지 폴트가 유효하지 않은 페이지에 접근한 폴트라면 찐 페이지 폴트일 것입니다. 
그렇지 않고 bogus 폴트라면 당신은 페이지에서 콘텐츠를 로드하고 유저 프로그램에게 제어권을 반환해야 합니다.
(지연 로딩으로 인해 물리 메모리와 매핑이 되어 있지만 콘텐츠가 로드되어 있지 않은 경우가 있을 수 있습니다. 
따라서 물리 메모리와 매핑은 되어 있지만 콘텐츠가 로드되어 있지 않은 경우- bogus fault- 라면  콘텐츠를 로드하면 되고, 
매핑되지 않은 페이지라면 그대로 유효한 페이지 폴트라는 의미인 것 같습니다.) 
bogus 페이지 폴트가 일어나는 3가지 케이스가 있습니다. 지연 로딩 페이지, 스왑 아웃 페이지, 쓰기 보호 페이지. 
(Copy-on-Write (Extra)를 참고하세요.)
지금은 첫 번째 케이스인 지연 로드 페이지에 대해서만 생각해봅시다. 만일 지연 로딩때문에 페이지 폴트가 일어나면, 
커널은 이미 당신이 세그먼트를 지연 로딩하기 위해 vm_alloc_page_with_initializer 함수에서 세팅해 놓은 초기화 함수를 호출합니다.  
당신은 userprog/process.c 에 있는 lazy_load_segment  함수를 구현해야 합니다.*/
/* Return true on success */
bool
vm_try_handle_fault (struct intr_frame *f UNUSED, void *addr UNUSED,
		bool user UNUSED, bool write UNUSED, bool not_present UNUSED) {
	struct supplemental_page_table *spt UNUSED = &thread_current ()->spt;
	struct page *page = NULL;
	/* TODO: Validate the fault */
	/* TODO: Your code goes here */
	if (is_kernel_vaddr(addr)) return false;
	if (addr > f->rsp) return false;
	// uintptr_t distance = (uintptr_t) f->rsp > (uintptr_t) addr ? (uintptr_t) f->rsp - (uintptr_t) addr : (uintptr_t) addr - (uintptr_t) f->rsp;
	// if (distance > USER_STACK) return false;

	if (not_present) {
		// page = malloc(sizeof(struct page));	
		return vm_claim_page(addr);
	}
	// else {
	// }
	// if (page = spt_find_page(spt, addr))
	// 	return vm_do_claim_page(page);
	return false;
}
/* Free the page.
 * DO NOT MODIFY THIS FUNCTION. */
void
vm_dealloc_page (struct page *page) {
	destroy (page);
	free (page);
}

/* Claim the page that allocate on VA. */
bool
vm_claim_page (void *va UNUSED) {
	struct page *page = NULL;
	/* TODO: Fill this function */
	page = spt_find_page(&thread_current()->spt, va);
	if (page == NULL) return false;
	return vm_do_claim_page (page);
}

/* Claim the PAGE and set up the mmu. */
static bool
vm_do_claim_page (struct page *page) {
	struct frame *frame = vm_get_frame ();

	/* Set links */
	frame->page = page;
	page->frame = frame;
	/* TODO: Insert page table entry to map page's VA to frame's PA. */
	if (pml4_get_page(thread_current()->pml4, page->va) == NULL && pml4_set_page(thread_current()->pml4, page->va, frame->kva, page->writable));
	return swap_in(page, frame->kva);
	// return true;
	return false;
}


uint64_t hash_hash_func_ (const struct hash_elem *e, void *aux) {
// 해시 값을 구해주는 함수
	struct page *page_ = hash_entry(e, struct page, elem);
	return hash_int(page_->va);
}
bool hash_less_func_ (const struct hash_elem *a, const struct hash_elem *b, void *aux) {
//해시 element 들의 크기를 비교해주는 함수
	struct page *a_page = hash_entry(a, struct page, elem);
	struct page *b_page = hash_entry(b, struct page, elem);
	return a_page->va < b_page->va;
}

void hash_action_func_ (struct hash_elem *e, void *aux) {
	// struct page *page = hash_entry(e, struct page, elem);
	// palloc_free_page(page);
	// vm_dealloc_page(page);
}
 
/* Initialize new supplemental page table */
void
supplemental_page_table_init (struct supplemental_page_table *spt UNUSED) {
	hash_init(&spt->hash, hash_hash_func_, hash_less_func_, NULL);
}

/* Copy supplemental page table from src to dst */
bool
supplemental_page_table_copy (struct supplemental_page_table *dst UNUSED,
		struct supplemental_page_table *src UNUSED) {
}

/* Free the resource hold by the supplemental page table */
void
supplemental_page_table_kill (struct supplemental_page_table *spt UNUSED) {
	/* TODO: Destroy all the supplemental_page_table hold by thread and
	 * TODO: writeback all the modified contents to the storage. */
	// hash_destroy(&spt->hash, hash_action_func_);
}
