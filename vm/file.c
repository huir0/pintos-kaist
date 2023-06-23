/* file.c: Implementation of memory backed file object (mmaped object). */

#include "vm/vm.h"
#include "filesys/file.h"
#include "threads/mmu.h"
#include "userprog/process.h"
#include "userprog/syscall.h"
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
	/* Set up the handler */
	page->operations = &file_ops;

	struct file_page *file_page = &page->file;
}

/* Swap in the page by read contents from the file. */
static bool
file_backed_swap_in (struct page *page, void *kva) {
	struct file_page *file_page = &page->file;

    lock_acquire(&filesys_lock);
    off_t size = file_read_at(file_page->file, kva, (off_t)file_page->page_read_bytes, file_page->ofs);
    lock_release(&filesys_lock);

    if (size != file_page->page_read_bytes)
        return false;

    memset(kva + file_page->page_read_bytes, 0, file_page->page_zero_bytes);

    return true;
}

/* Swap out the page by writeback contents to the file. */
static bool
file_backed_swap_out (struct page *page) {
	struct file_page *file_page = &page->file;
    struct thread *curr = thread_current();

    if (pml4_is_dirty(curr->pml4, page->va))
    {
        lock_acquire(&filesys_lock);
        file_write_at(file_page->file, page->va, file_page->page_read_bytes, file_page->ofs);
        lock_release(&filesys_lock);

        pml4_set_dirty(curr->pml4, page->va, false);
    }
    pml4_clear_page(curr->pml4, page->va);
    page->frame = NULL;

    return true;
}

/* Destory the file backed page. PAGE will be freed by the caller. */
static void
file_backed_destroy (struct page *page) {
	struct file_page *file_page = &page->file;
    free(page->frame);
    
}

/* Do the mmap */
void *
do_mmap (void *addr, size_t length, int writable,
		struct file *file, off_t offset) {
/*우선적으로 인자로 들어온 file을 reopen()을 통해 동일한 파일에 대해 다른 주소를 가지는 파일 구조체를 생성합니다. 
reopen()하는 이유는 mmap을 하는 동안 만약 외부에서 해당 파일을 close()하는 불상사를 예외처리하기 위함입니다.
저희 팀의 경우 해당 함수를 구현할때 prcess.c의 load_segment()함수를 떠올렸습니다.
생각해보면 load_segment() 또한 load()에서 open()한 파일을 받아서 lazy-loading을 수행하도록 페이지를 생성해줍니다.
즉,사실상 거의 동일한 로직을 사용해도 된다는 것입니다.
다만 차이점이 한가지 있다면 load_segment()는 페이지 이니셜라이저를 호출할때 페이지의 타입의 ANON으로 설정했고 
do_mmap()은 이와 달리 FILE 타입으로 인자를 넘겨줘야한다는것입니다.
구현적으로 팁을 드리자면 해당 함수는 void*형의 주소를 반환해야합니다.즉,성공적으로 페이지를 생성하면 addr을 반환합니다.
하지만 addr 주소의 경우 iter를 돌면서 PGSIZE만큼 변경되기 때문에 초기의 addr을 리턴값으로 저장해두고 iter를 돌아야합니다.*/
	void *origin_addr = addr;
	size_t f_length = length;
	struct file *c_file = file_reopen(file);
	
   	while (f_length > 0)
   	{
      	/* Do calculate how to fill this page.
       	* We will read PAGE_READ_BYTES bytes from FILE
       	* and zero the final PAGE_ZERO_BYTES bytes. */

      	struct lazy *aux = (struct lazy*)malloc(sizeof(struct lazy));
    	size_t page_read_bytes = f_length < PGSIZE ? f_length : PGSIZE;

		if (spt_find_page(&thread_current()->spt,addr)) return NULL;
      	/* TODO: Set up aux to pass information to the lazy_load_segment. */
      	aux->file_ = c_file;
		aux->offset = offset;
		aux->read_bytes = page_read_bytes;
		aux->zero_bytes = PGSIZE - page_read_bytes;
		if (!vm_alloc_page_with_initializer(VM_FILE, addr, writable, lazy_load_segment, aux)) return NULL;
		struct page *page = spt_find_page(&thread_current()->spt, addr);
		if (page == NULL) return NULL;
		f_length -= page_read_bytes;
		addr += PGSIZE;
		offset += page_read_bytes; 
	}
	return origin_addr;
}

/* Do the munmap */
void
do_munmap (void *addr) {
/* 구현을 하기 앞서 해당 함수가 어떻게 돌아가는지를 먼저 설명하겠습니다.
우선 mmap()함수의 역연산을 하는 함수입니다.mmap()을 통해 저희는 파일 타입이 FILE인 UNINIT페이지를 생성했습니다.
이후 page-fault가 발생하면 해당 페이지는 FILE타입의 페이지로 초기화되며 물리프레임과 연결됩니다.
그러면 do_munmap()에서는 연결된 물리프레임과의 연결을 끊어주어야합니다.
다만,중요한점이 FILE 타입의 페이지는 file-backed 페이지이기에 디스크에 존재하는 파일과 연결된 페이지입니다.
그래서 만약 해당 페이지에 수정사항이 있을 경우 이를 감지하여 변경사항을 디스크의 파일에 써줘야합니다.
코드를 구현하려면 우선 주어진 addr을 통해서 SPT로부터 page를 하나 찾습니다.
이후,해당 페이지가 변경이 되어있을 경우 디스크에 존재하는 file에 write해주고 dirty-beat를 다시 0으로 변경시켜줍니다.
만약 변경이 되어있지 않을 경우 해당 페이지를 pml4에서 삭제 해주고 addr을 다음 페이지 주소로 변경해줍니다.
페이지를 변경하는 iter를 저희조는 가장 간단하게 while(true)를 통해 모든 페이지를 찾아주었는데,
이는 논리적인 허점이 있으니 mmap()에서 페이지의 넘버링을 통해 페이지의 시퀀스를 저장한다음 해당 시퀀스를 iter의 종료조건으로 넣어주시는것도 좋은 방법입니다.
(그 외 다양한 방법이 존재하는것 같습니다)*/
	struct thread *curr = thread_current();
	struct page *page = spt_find_page(&curr->spt, addr);
	if(page == NULL) return NULL;	
	struct lazy *c_page = page;
	if(c_page->file_ == NULL) return NULL;
	while(page != NULL){
		if(pml4_is_dirty(curr->pml4,addr)){
			lock_acquire(&filesys_lock);
			file_write_at(c_page->file_, addr, c_page->read_bytes,c_page->offset);
			lock_release(&filesys_lock); 
			pml4_set_dirty(curr->pml4,addr,false);
		}
		pml4_clear_page(curr->pml4,addr);
		addr += PGSIZE;
		page = spt_find_page(&curr->spt,addr);
	}	
}