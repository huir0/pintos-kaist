/* vm.c: Generic interface for virtual memory objects. */

#include "threads/malloc.h"
#include "vm/vm.h"
#include "vm/inspect.h"
#include "lib/kernel/hash.h"
#include "include/threads/vaddr.h"
#include "threads/mmu.h"
#include "include/userprog/process.h"
#include "lib/string.h"
#include "vm/uninit.h"
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
static struct frame *vm_get_frame (void);
uint64_t vm_hash_func (const struct hash_elem *e,void *aux);
bool vm_less_func (const struct hash_elem *a, const struct hash_elem *b);
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
		if (uninit == NULL)
		{
			return false;
		}
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
	page.va = pg_round_down(va);
	struct hash_elem *e = hash_find(&spt->spt_hash, &page.h_elem);
	if (e) return hash_entry(e, struct page, h_elem);
	else return NULL;
}

/* Insert PAGE into spt with validation. */
bool
spt_insert_page (struct supplemental_page_table *spt UNUSED, struct page *page UNUSED) {
	int succ = false;
	/* TODO: Fill this function. */
	// if(hash_insert(spt->hash, &page->elem))
	// 	succ = true;
	// if (spt_find_page(spt, page->va) != NULL) return false;
	if(hash_insert(&spt->spt_hash, &page->h_elem) == NULL) succ = true;
	return succ;
}

void
spt_remove_page (struct supplemental_page_table *spt, struct page *page) {
	// hash_delete(&spt->spt_hash, &page->h_elem);
	vm_dealloc_page (page);
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
	return NULL;
}

/* palloc() and get frame. If there is no available page, evict the page
 * and return it. This always return valid address. That is, if the user pool
 * memory is full, this function evicts the frame to get the available memory
 * space.*/
static struct frame *
vm_get_frame (void) {
	struct frame *frame = (struct frame *)malloc(sizeof(struct frame));
	
	frame->kva = palloc_get_page(PAL_USER);
	if (frame->kva == NULL){
		PANIC ("todo");
	}
	frame->page = NULL;
	ASSERT (frame != NULL);
	ASSERT (frame->page == NULL);
	return frame;
}

/* Growing the stack. */
static void
vm_stack_growth (void *addr UNUSED) {
	if(vm_alloc_page(VM_ANON, pg_round_down(addr), true))	{
		thread_current()->stack_bottom -= PGSIZE;
		return true;
	}
	return false;
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
	/* TODO: Validate the fault */
	/* TODO: Your code goes here */
	if (is_kernel_vaddr(addr)||!addr||!not_present) return false;
	if (not_present){
		// 커널이면 thread구조체의 rsp_stack을, 유저면 interrupt frame의 rsp를 사용함
		void *rsp_stack = is_kernel_vaddr(f->rsp) ? thread_current()->rsp_stack : f->rsp;
		/* 유저 스택영역에 접근하는 경우임, 0x100000 = 2^20 = 1MB 
			rsp_stack과 한개의 페이지 크기 8사이의 주소에서 page_fault가 났는지, 주소가 유저스택의
			최대 최소 영역 안에 있는지 */
		if (rsp_stack - 8 <= addr && USER_STACK - 0x100000 <= addr && addr <= USER_STACK ) vm_stack_growth(addr);
		struct page *page = spt_find_page(spt, addr);
		if (page == NULL) return false;
		if (write == 1 && !page->writable) return false;
		return vm_do_claim_page (page);
    }
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
	
	if (frame == NULL) return false;
	/* Set links */
	frame->page = page;
	page->frame = frame;

	/* TODO: Insert page table entry to map page's VA to frame's PA. 
	페이지의 VA를 프레임의 PA에 매핑하려면 페이지 테이블 항목을 삽입합니다.*/
	if (pml4_set_page(thread_current()->pml4, page->va, frame->kva, page->writable))
		return swap_in (page, frame->kva);
		// return true;
	return false;
}


static unsigned hash_hash_func_ (const struct hash_elem *e, void *aux) {
// 해시 값을 구해주는 함수
	struct page *p = hash_entry(e, struct page, h_elem);
	return hash_int(p->va);
}
static bool hash_less_func_ (const struct hash_elem *a, const struct hash_elem *b, void *aux) {
//해시 element 들의 크기를 비교해주는 함수
	struct page *v_a = hash_entry(a, struct page, h_elem);
	struct page *v_b = hash_entry(b, struct page, h_elem);

	return v_a->va < v_b->va;
}
static void
hash_alloc_func(struct hash_elem *he, void *aux){
	struct page *page = hash_entry(he,struct page,h_elem);
	if(page -> files != NULL) vm_dealloc_page(page);
}
 
void hash_action_func_ (struct hash_elem *e, void *aux) {
	struct page *page = hash_entry(e, struct page, h_elem);
	vm_dealloc_page(page);
}
 
/* Initialize new supplemental page table */
void
supplemental_page_table_init (struct supplemental_page_table *spt UNUSED) {
	hash_init(&spt->spt_hash, hash_hash_func_, hash_less_func_, NULL);
}
/* Copy supplemental page table from src to dst */
bool
supplemental_page_table_copy (struct supplemental_page_table *dst UNUSED, struct supplemental_page_table *src UNUSED) {
/*src부터 dst까지 supplemental page table를 복사하세요. 
이것은 자식이 부모의 실행 context를 상속할 필요가 있을 때 사용됩니다.(예 - fork()). 
src의 supplemental page table를 반복하면서 dst의 supplemental page table의 엔트리의 정확한 복사본을 만드세요. 
당신은 초기화되지않은(uninit) 페이지를 할당하고 그것들을 바로 요청할 필요가 있을 것입니다.*/
/* 해당 함수는 부모 프로세스가 가지고 있는 본인의 SPT 정보를 빠짐없이 자식 프로세스에게 복사해주는 기능을 수행합니다.(fork 시스템 콜)
우선 해시테이블을 통해 구현한 SPT를 iteration해줘야 합니다.이를 구현하기 위한 방법을 hash.c 파일에서 제공합니다.저희 팀의 경우 간단히 while문을 통해 iter해주는 방식을 택하였습니다.
이후 iter를 돌 때마다 해당 hash_elem과 연결된 page를 찾아서 해당 페이지 구조체의 정보들을 저장합니다.
페이지 구조체의 어떠한 정보를 저장해야할지 감이 안오신다면 vm_alloc_page_with_initializer()함수의 인자를 참고하시길 바랍니다.
부모 페이지들의 정보를 저장한 뒤,자식이 가질 새로운 페이지를 생성해야합니다.생성을 위해 부모 페이지의 타입을 먼저 검사합니다.즉,부모 페이지가UNINIT 페이지인 경우와 그렇지 않은 경우를 나누어 페이지를 생성해줘야합니다.
만약 uninit이 아닌 경우 setup_stack()함수에서 했던 것처럼 페이지를 생성한뒤 바로 해당 페이지의 타입에 맞는 initializer를 호출해 페이지 타입을 변경시켜줍니다.그리고 나서 부모페이지의 물리 메모리 정보를 자식에게도 복사해주어야 합니다.
모든 함수 호출이 정상적으로 이루어졌다면 return true를 하며 함수를 종료합니다.*/
	struct hash_iterator h_iter;
	struct page *p_page;
	bool success = false;
	hash_first(&h_iter, &src->spt_hash);// 초기 반복자를 src 해시 테이블의 첫 번째 원소로 설정

	// 해시 테이블의 다음 원소가 있을 때까지 반복
	while (hash_next(&h_iter))
	{	
		// 현재 해시 테이블 원소를 가져온 후 struct page로 변환
		p_page = hash_entry(hash_cur(&h_iter), struct page, h_elem);
		// 페이지를 할당하고 초기화
		success = vm_alloc_page_with_initializer(p_page->uninit.type, p_page->va, p_page->writable, p_page->uninit.init, p_page->uninit.aux);
		if (!success) return false;	 // 할당에 실패한 경우 false 반환
		struct page *c_page = spt_find_page(dst, p_page->va);	// 복사할 페이지 (c_page) 찾기
		if (p_page->frame)	  	// 원본 페이지에 frame이 있는 경우
		{
			// c_page에 대한 메모리를 할당하고 초기화
       		success = vm_do_claim_page(c_page);
        	if (!success) return false;            // 초기화에 실패한 경우 false 반환

        	// 원본 페이지와 복사 페이지의 메모리 간에 데이터를 복사
        	memcpy(c_page->frame->kva, p_page->frame->kva, PGSIZE);
		}
	}	
	return success;
}

/* Free the resource hold by the supplemental page table */
void
supplemental_page_table_kill (struct supplemental_page_table *spt UNUSED) {
/* TODO: Destroy all the supplemental_page_table hold by thread and
	* TODO: writeback all the modified contents to the storage. */
/* supplemental page table에 의해 유지되던 모든 자원들을 free합니다. 
이 함수는 process가 exit할 때(userprog/process.c의 process_exit()) 호출됩니다. 
당신은 페이지 엔트리를 반복하면서 테이블의 페이지에 destroy(page)를 호출하여야 합니다. 
당신은 이 함수에서 실제 페이지 테이블(pml4)와 물리 주소(palloc된 메모리)에 대해 걱정할 필요가 없습니다. 
supplemental page table이 정리되어지고 나서, 호출자가 그것들을 정리할 것입니다. 
깃북에 적힌것처럼 해당 함수는 존재하는 SPT를 모두 free()하며 할당 해제해주는 함수입니다.
저희는 간단한게 hash.c 파일에 존재하는 hash_destroy() 함수만 사용했습니다.
해당 함수를 사용하려면 두번째 인자로 넘어가는 aux에 대응하는 함수를 하나 만들어야합니다.
만들어야하는 함수의 경우 hash_elem을 인자로 받기에 elem을 통해 page를 찾고 해당 페이지를 할당 해제해주는 로직을 구현했습니다.
추후 memory-mapped 부분을 진행할때,해당 함수를 추가적으로 수정할 예정입니다*/
	hash_destroy(&spt->spt_hash,hash_alloc_func);
}