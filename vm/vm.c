/* vm.c: Generic interface for virtual memory objects. */

#include "threads/malloc.h"
#include "vm/vm.h"
#include "vm/inspect.h"
#include "lib/kernel/hash.h"
#include "threads/vaddr.h"
#include "threads/mmu.h"
#include "lib/string.h"
#include "userprog/process.h"

/* Initializes the virtual memory subsystem by invoking each subsystem's
 * intialize codes. */
struct list frame_list;
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
	list_init(&frame_list);
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
		struct page *page = (struct page *)malloc(sizeof(struct page));
		if(page == NULL) {
			free(page);
			goto err;
		}
		typedef bool (*page_initializer) (struct page *, enum vm_type, void *kva);
		page_initializer new_initializer = NULL;
		switch(VM_TYPE(type)) {
			case VM_ANON:
				new_initializer = anon_initializer;
				break;
			case VM_FILE:
				new_initializer = file_backed_initializer;
				break;
			default:
				break;
		}
		/* TODO: Insert the page into the spt. */
		uninit_new(page, upage, init, type, aux, new_initializer);
		page->writable = writable;
		bool succ = spt_insert_page(spt, page);
		return succ;
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
	page.va = pg_round_down(va); // va의 페이지 시작지점으로 옮긴다.
	e = hash_find(&spt->hash, &page.elem); // page의 hash_elem 값으로 hash_find 함수를 통해 e를 가져온다.
	if(e == NULL) {
		return NULL;
	}
	return hash_entry(e, struct page, elem); // hash_entry 함수로 해당 구조체 (여기선 struct page)로 반환시켜준다. list_entry와 동일한 구조
}

/* Insert PAGE into spt with validation. */
bool
spt_insert_page (struct supplemental_page_table *spt UNUSED,
		struct page *page UNUSED) {
	int succ = false;
	/* TODO: Fill this function. */	
	if(page == NULL) {
		return false;
	}
	if(hash_insert(&spt->hash, &page->elem) == NULL) {
		succ = true; // hash table에 page가 삽입이 되면 true, 안되면 false를 반환한다.
	}
	return succ;
}

void
spt_remove_page (struct supplemental_page_table *spt, struct page *page) {
	vm_dealloc_page (page); // page를 해제시켜준다.
	return true;
}

/* Get the struct frame, that will be evicted. */
static struct frame *
vm_get_victim (void) {
	struct frame *victim = NULL;
	 /* TODO: The policy for eviction is up to you. */
	victim = list_pop_front(&frame_list); // FIFO형식으로 일단 구현
	return victim;
}

/* Evict one page and return the corresponding frame.
 * Return NULL on error.*/
static struct frame *
vm_evict_frame (void) {
	struct frame *victim UNUSED = vm_get_victim ();
	/* TODO: swap out the victim and return the evicted frame. */
	swap_out(victim->page);
	return victim;
}

/* palloc() and get frame. If there is no available page, evict the page
 * and return it. This always return valid address. That is, if the user pool
 * memory is full, this function evicts the frame to get the available memory
 * space.*/
static struct frame *
vm_get_frame (void) {
	struct frame *frame = NULL;
	/* TODO: Fill this function. */
	frame = (struct frame *)malloc(sizeof(struct frame));
	frame->kva = palloc_get_page(PAL_USER); // 구조체 할당
	if(frame->kva == NULL) { // 할당할 물리메모리가 부족할 경우 프레임과 연결된 페이지 한 개를 swap_out 시키고 사용 가능한 프레임을 가져온다
		frame = vm_evict_frame();
		frame->page = NULL;
		return frame;
	}
	list_push_back(&frame_list, &frame->frame_elem); // frame_list에 해당 프레임을 삽입.
	frame->page = NULL;
	ASSERT (frame != NULL);
	ASSERT (frame->page == NULL);
	return frame;
}

/* Growing the stack. */
static void
vm_stack_growth (void *addr UNUSED) {
	struct thread *cur = thread_current();
	bool status;
	status = vm_alloc_page(VM_ANON, addr, true);
	if(status) {
		cur->stack_bottom -= PGSIZE;
	}
	addr = pg_round_down(addr);
	while(addr < cur->stack_bottom) {
		status = vm_alloc_page(VM_ANON, addr, true);
		vm_claim_page(addr);
		addr += PGSIZE;
	}
}

/* Handle the fault on write_protected page */
static bool
vm_handle_wp (struct page *page UNUSED) {
}

/* Return true on success */
/**
 * f : 시스템 콜 또는 페이지 폴트가 발생했을 때,그 순간의 레지스터 값들을 담고 있는 구조체.
 * addr : 페이지 폴트를 일으킨 가상 주소
 * user : 해당 값이 true일 경우 현재 쓰레드가 유저 모드에서 돌아가다가 페이지 폴트를 일으켰음을 나타낸다.즉,현재 쓰레드의 rsp값이 VM의 유저 영역을 나타내는지 커널 영역을 나타내는지를 알려준다.
 * write : true일 경우,해당 페이지 폴트가 쓰기 요청이고 그렇지 않을 경우 읽기 요청을 나타냄
 * not-present : 해당 인자가 false인 경우는 read-only 페이지에 write를 하려는 상황을 나타냄.주어진 테스트 케이스에서는 mmap-ro 케이스가 해당 인자를 체크함
*/
bool
vm_try_handle_fault (struct intr_frame *f UNUSED, void *addr UNUSED, bool user UNUSED, bool write UNUSED, bool not_present UNUSED) {
	struct thread *cur = thread_current();
	struct supplemental_page_table *spt UNUSED = &cur->spt;
	struct page *page = NULL;
	/* TODO: Validate the fault */
	/* TODO: Your code goes here */
	bool status = false;
	if(is_kernel_vaddr(addr) || addr == NULL || !not_present) {
		return status;
	}
	page = spt_find_page(spt, addr);
	if(page == NULL){
		if(addr >= f->rsp - 8 && addr >= STACK_MAX && addr <= USER_STACK) {
			vm_stack_growth(addr);
			cur->stack_bottom = pg_round_down(addr);
			status = true;
			return status;
		}
	}
	else {
		status = vm_do_claim_page(page);
		return status;
	}
	return status;
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
	struct thread *cur = thread_current();
	page = spt_find_page(&cur->spt, va); // 현재 스레드의 spt에서 va값을 가진 page를 찾는다. +해당 함수에서 pg_round_down(va)로 전달받은 va를 페이지 시작주소로 옮긴다.
	if(page == NULL) {
		return false;
	}
	
	return vm_do_claim_page (page);
}

/* Claim the PAGE and set up the mmu. */
static bool
vm_do_claim_page (struct page *page) {
	struct frame *frame = vm_get_frame ();
	if(frame == NULL) {
		return false;
	}
	/* Set links */
	frame->page = page;
	page->frame = frame;

	/* TODO: Insert page table entry to map page's VA to frame's PA. */
	struct thread *cur = thread_current();
	bool succ = false;
	if(pml4_get_page(cur->pml4, page->va) == NULL) { // page table에서 해당 page가 있는지 확인한다. 없으면 NULL 반환
	 	succ = pml4_set_page(cur->pml4, page->va, frame->kva, page->writable); // page와 frame의 va, pa를 mapping 시켰다면 true반환, 실패시 null 반환
		if(succ) {
			page->is_loaded = true;
			succ = swap_in(page, frame->kva);
		}
	}
	return succ;
}

/* Returns a hash value for page p. */
/**
 * page 구조체의 va를 이용해 해싱 함수를 돌려 키를 만드는 함수
*/
unsigned
page_hash (const struct hash_elem *p_, void *aux UNUSED) {
	struct page *p = hash_entry (p_, struct page, elem);
	return hash_bytes(&p->va, sizeof(p->va));
}

/* Returns true if page a precedes page b. */
/**
 * 페이지에서 원하는 값을 리스트에서 서치할 때 사용. 함수에는 va의 대소비교를 하는 내용이 들어 있음.
*/
bool
page_less (const struct hash_elem *a_, const struct hash_elem *b_, void *aux UNUSED) {
  struct page *a = hash_entry (a_, struct page, elem);
  struct page *b = hash_entry (b_, struct page, elem);

  return a->va < b->va;
}

void hash_action (struct hash_elem *e, void *aux) {
	struct page *page = hash_entry(e, struct page, elem);
	if(page->is_loaded) {
		vm_dealloc_page(page);
	}
}

/* Initialize new supplemental page table */
void
supplemental_page_table_init (struct supplemental_page_table *spt UNUSED) {
	hash_init(&spt->hash, page_hash, page_less, NULL);
}

/* Copy supplemental page table from src to dst */
bool
supplemental_page_table_copy (struct supplemental_page_table *dst UNUSED,
		struct supplemental_page_table *src UNUSED) {
	
	struct hash_elem *e;
	struct page *parent_page, *child_page;
	bool status = false;
	struct hash_iterator i;
   	hash_first(&i, &src->hash);
   	while (e = hash_next(&i)) {
		parent_page = hash_entry(e, struct page, elem); // 부모 페이지를 src_hash에서 가져오기
		status = vm_alloc_page_with_initializer(page_get_type(parent_page), parent_page->va, parent_page->writable, parent_page->uninit.init, parent_page->uninit.aux);
		if(!status) {
			return status;
		}
		child_page = spt_find_page(dst, parent_page->va);

		if(parent_page->frame != NULL) {
			status = vm_do_claim_page(child_page);
			if(!status) {
				return status;
			}
			memcpy(child_page->frame->kva, parent_page->frame->kva, PGSIZE);
		}
	}
	return true;
}

/* Free the resource hold by the supplemental page table */
void
supplemental_page_table_kill (struct supplemental_page_table *spt UNUSED) {
	/* TODO: Destroy all the supplemental_page_table hold by thread and
	 * TODO: writeback all the modified contents to the storage. */
	hash_destroy(&spt->hash, hash_action);
}
