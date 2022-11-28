#include <semaphore.h>
#include <system.h>
#include <proc.h>

static inline uint32_t semaphore_phy_addr(int32_t* s) {
	uint32_t paddr = resolve_phy_address(_current_proc->space->vm, (uint32_t)s);
	return paddr;
}

/*wake up all blocked processes by specific semaphore*/
static void wake_procs(int32_t* s) {
	uint32_t paddr = semaphore_phy_addr(s);
	process_t *p = _current_proc->next;
	while(p != _current_proc) {
		if (p->state == SLEEPING &&
				p->wait_semaphore_paddr == paddr) {
			p->wait_semaphore_paddr = 0;
			p->state = READY;
		}
		p = p->next;
	}
}

int32_t semaphore_init(int32_t *s) {
	if(s == NULL)
		return -1;
	*s = 0;
	return 0;
}

int32_t semaphore_close(int32_t* s) {
	wake_procs(s);
	*s = -1;
	return 0;
}

int32_t semaphore_lock(int32_t* s) {
	if(s == NULL || (*s) < 0)
		return 0;
	
	CRIT_IN
	if(*s > 0) { /*still locked by other process, put current process to sleep*/
		_current_proc->state = SLEEPING;
		_current_proc->wait_semaphore_paddr = semaphore_phy_addr(s);
		CRIT_OUT
		return -1;
	}	
	(*s)++;
	CRIT_OUT
	return 0;
}

int32_t semaphore_unlock(int32_t* s) {
	if(s == NULL || (*s) <= 0)
		return 0;

	CRIT_IN
	(*s)--;
	if((*s) == 0) /*if all locks cleared, wake up processed blocked by this semaphore*/
		wake_procs(s);
	CRIT_OUT
	return 0;
}