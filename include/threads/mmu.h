#ifndef THREAD_MMU_H
#define THREAD_MMU_H

#include <stdbool.h>
#include <stdint.h>
#include "threads/pte.h"
/* threads/mmu.c에 있는 코드들은 x86-64 하드웨어 페이지 테이블의 추상화된 인터페이스입니다.
(당신이 프로젝트에서 사용할) Pintos에서의 페이지 테이블은 pml4 라 불립니다. 
이는 인텔 프로세서 공식문서에 따르면 Page-Map-Level-4 인데, 왜냐하면 테이블이 4개 단계로 이루어져 있기 때문이죠.
페이지 테이블 인터페이스는 페이지 테이블을 나타내기 위해 uint64_t *를 사용합니다. 
이 방법이 페이지 테이블의 내부 구조에 액세스하는데 편리하기 때문입니다. 
아래 절에서는 페이지 테이블 인터페이스와 그 내용들을 설명하겠습니다.*/

typedef bool pte_for_each_func (uint64_t *pte, void *va, void *aux);

uint64_t *pml4e_walk (uint64_t *pml4, const uint64_t va, int create);
/* 새로운 페이지 테이블을 생성하여 리턴합니다.
새 페이지 테이블은 Pintos의 표준 커널 가상페이지 매핑을 담고 있지만, 
유저 가상 매핑은 담고 있지 않습니다. 
메모리 획득이 불가능하다면 널 포인터를 리턴합니다.*/
uint64_t *pml4_create (void);
bool pml4_for_each (uint64_t *, pte_for_each_func *, void *);
/* 페이지 테이블 자신과 매핑된 프레임을 포함해서, pml4가 가진 모든 리소스를 해제(free)시킵니다. 
이 함수는 페이지 테이블의 모든 단계의 리소스들을 해제시키기 위해서 
pdpe_destroy, pgdir_destory, pt_destroy 를 재귀적으로 호출합니다.*/
void pml4_destroy (uint64_t *pml4);
/* pml4를 활성화시킵니다. 활성화된 페이지 테이블은 CPU가 메모리 참조를 중계하는 데에 쓰입니다. */
void pml4_activate (uint64_t *pml4);
/* pml4에서 uaddr와 매핑된 프레임을 찾습니다. uaddr이 매핑되어있다면 해당 프레임에 대한 커널 가상 주소를 리턴합니다. 매핑되어있지 않다면 null 포인터를 리턴합니다.  */
void *pml4_get_page (uint64_t *pml4, const void *upage);
/* pd에 유저 페이지 upage와 ‘프레임’간의 매핑을 추가합니다. 
‘프레임’은 커널 가상주소 kpage로 인해 식별됩니다. 
(인자로 들어온) rw가 true 라면, 페이지는 read/wirte 로 매핑되고, 그렇지 않다면 read-only로 매핑됩니다.  
유저페이지 upage는 pml4 에 매핑되어있지 않아야 합니다. 
커널 페이지 kpage는 palloc_get_page(PAL_USER) 을 사용해서 유저 풀로부터 획득한 커널 가상 주소여야 합니다. 
성공적으로 add 했다면 true를 반환하고, 실패했을 경우 false를 반환합니다. 
페이지 테이블을 위한 추가적인 메모리 공간을 얻을 수 없다면 실패하게 됩니다. */
bool pml4_set_page (uint64_t *pml4, void *upage, void *kpage, bool rw);
/* pml4에서 upage 를 “존재하지 않음”으로 표시합니다. 이후에 이 페이지에 접근하는 것을 fault를 발생시킬 것입니다. */
void pml4_clear_page (uint64_t *pml4, void *upage);
/* pml4에서 vpage 대한 페이지 테이블 엔트리가 dirty bit가 표시되었다면 (1로 설정되었다면) true를 반환합니다.  */
bool pml4_is_dirty (uint64_t *pml4, const void *upage);
/* pml4가 vpage에 대한 페이지테이블을 갖고 있다면, dirty bit가 주어진 값으로 업데이트 합니다. */
void pml4_set_dirty (uint64_t *pml4, const void *upage, bool dirty);
/* pml4에서 vpage 대한 페이지 테이블 엔트리가 accessed bit가 표시되었다면 (1로 설정되었다면) true를 반환합니다.  */
bool pml4_is_accessed (uint64_t *pml4, const void *upage);
/* pml4가 vpage에 대한 페이지테이블을 갖고 있다면, accessed bit를 주어진 값으로 업데이트 합니다. */
void pml4_set_accessed (uint64_t *pml4, const void *upage, bool accessed);

/* PTE가 가리키는 가상주소가 작성 가능한 지(wriatable) 아닌 지 확인합니다. */
#define is_writable(pte) (*(pte) & PTE_W)
/* 페이지 테이블 엔트리(PTE)의 주인이 유저인지 커널인지 확인합니다. 
* user pte 라면 유저/커널, kernel pte 라면 커널 only. */
#define is_user_pte(pte) (*(pte) & PTE_U)
/* 페이지 테이블 엔트리(PTE)의 주인이 유저인지 커널인지 확인합니다. 
* user pte 라면 유저/커널, kernel pte 라면 커널 only. */
#define is_kern_pte(pte) (!is_user_pte (pte))
/* 각 pml4가 유효한 entry를 가지고 있는지 검사하며, 
검사를 위해 보조값 aux를 받는 함수 func를 추가적으로 활용합니다. 
va는 entry의 가상주소입니다. pte_for_each_func가 false를 리턴하면, 
반복을 멈추고 false를 리턴합니다.*/
#define pte_get_paddr(pte) (pg_round_down(*(pte)))

/* Segment descriptors for x86-64. */
/* pml4_for_each의 인자로 전달될 수 있는 func의 예시를 보여줍니다. */
struct desc_ptr {
	uint16_t size;
	uint64_t address;
} __attribute__((packed));

#endif /* thread/mm.h */
