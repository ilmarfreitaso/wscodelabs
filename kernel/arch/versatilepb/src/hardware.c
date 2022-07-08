#include <hardware.h>
#include <mm/mmu.h>

unsigned int getPhyRamSize() {
	return 255*MB;
}

unsigned int getMMIOBasePhy() {
	return 0x10000000;
}

unsigned int getMMIOMemSize() {
	return 2*MB;
}

unsigned int getUartIrq() {
	return 12;
}

unsigned int getTimerIrq() {
	return 5;
}

void archSetKernelVM(PageDirEntryT* vm) {
	(void)vm;
}
