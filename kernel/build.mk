KERNEL_DIR=kernel


ARCH_OBJS = $(KERNEL_DIR)/arch/$(arch)/src/irq.o \
	$(KERNEL_DIR)/arch/$(arch)/src/uart.o \
	$(KERNEL_DIR)/arch/$(arch)/src/timer.o \
	$(KERNEL_DIR)/arch/$(arch)/src/hardware.o

DEV_OBJS = $(KERNEL_DIR)/src/dev/uart.o

OBJS += $(ARCH_OBJS) \
	$(DEV_OBJS) \
	$(KERNEL_DIR)/src/kernel.o \
	$(KERNEL_DIR)/src/irq.o \
	$(KERNEL_DIR)/src/syscalls.o \
	$(KERNEL_DIR)/src/mm/startup.o \
	$(KERNEL_DIR)/src/mm/mmu.o \
	$(KERNEL_DIR)/src/mm/kalloc.o \
	$(KERNEL_DIR)/src/mm/trunkmalloc.o \
	$(KERNEL_DIR)/src/mm/kmalloc.o \
	$(KERNEL_DIR)/src/proc.o \
	$(KERNEL_DIR)/src/kmessage.o \
	$(KERNEL_DIR)/src/kfile.o \
	$(KERNEL_DIR)/src/kserv.o \
	$(KERNEL_DIR)/src/scheduler.o 

