/*
 * operations on IDE disk.
 */

#include "serv.h"
#include <drivers/dev_disk.h>
#include <lib.h>
#include <mmu.h>

// Overview:
//  read data from IDE disk. First issue a read request through
//  disk register and then copy data from disk buffer
//  (512 bytes, a sector) to destination array.
//
// Parameters:
//  diskno: disk number.
//  secno: start sector number.
//  dst: destination for data read from IDE disk.
//  nsecs: the number of sectors to read.
//
// Post-Condition:
//  Panic if any error occurs. (you may want to use 'panic_on')
//
// Hint: Use syscalls to access device registers and buffers.
// Hint: Use the physical address and offsets defined in 'include/drivers/dev_disk.h':
//  'DEV_DISK_ADDRESS', 'DEV_DISK_ID', 'DEV_DISK_OFFSET', 'DEV_DISK_OPERATION_READ',
//  'DEV_DISK_START_OPERATION', 'DEV_DISK_STATUS', 'DEV_DISK_BUFFER'
void ide_read(u_int diskno, u_int secno, void *dst, u_int nsecs) {
	u_int begin = secno * BY2SECT;
	u_int end = begin + nsecs * BY2SECT;

	for (u_int off = 0; begin + off < end; off += BY2SECT) {
		uint32_t temp = diskno;
		/* Exercise 5.3: Your code here. (1/2) */
		panic_on(syscall_write_dev(&temp, DEV_DISK_ADDRESS + DEV_DISK_ID, 4));
		temp = begin + off;
		panic_on(syscall_write_dev(&temp, DEV_DISK_ADDRESS + DEV_DISK_OFFSET, 4));
		temp = DEV_DISK_OPERATION_READ;
		panic_on(syscall_write_dev(&temp, DEV_DISK_ADDRESS + DEV_DISK_START_OPERATION, 4));
		panic_on(syscall_read_dev(&temp, DEV_DISK_ADDRESS + DEV_DISK_STATUS, 4));
		if (!temp) {
			user_panic("ide_read fail");
		}
		panic_on(syscall_read_dev(dst + off, DEV_DISK_ADDRESS + DEV_DISK_BUFFER, BY2SECT));
	}
}

// Overview:
//  write data to IDE disk.
//
// Parameters:
//  diskno: disk number.
//  secno: start sector number.
//  src: the source data to write into IDE disk.
//  nsecs: the number of sectors to write.
//
// Post-Condition:
//  Panic if any error occurs.
//
// Hint: Use syscalls to access device registers and buffers.
// Hint: Use the physical address and offsets defined in 'include/drivers/dev_disk.h':
//  'DEV_DISK_ADDRESS', 'DEV_DISK_ID', 'DEV_DISK_OFFSET', 'DEV_DISK_BUFFER',
//  'DEV_DISK_OPERATION_WRITE', 'DEV_DISK_START_OPERATION', 'DEV_DISK_STATUS'
void ide_write(u_int diskno, u_int secno, void *src, u_int nsecs) {
	u_int begin = secno * BY2SECT;
	u_int end = begin + nsecs * BY2SECT;

	for (u_int off = 0; begin + off < end; off += BY2SECT) {
		uint32_t temp = diskno;
		/* Exercise 5.3: Your code here. (2/2) */
		panic_on(syscall_write_dev(src + off, DEV_DISK_ADDRESS + DEV_DISK_BUFFER, BY2SECT));
		panic_on(syscall_write_dev(&temp, DEV_DISK_ADDRESS + DEV_DISK_ID, 4));
		temp = begin + off;
		panic_on(syscall_write_dev(&temp, DEV_DISK_ADDRESS + DEV_DISK_OFFSET, 4));
		temp = DEV_DISK_OPERATION_WRITE;
		panic_on(syscall_write_dev(&temp, DEV_DISK_ADDRESS + DEV_DISK_START_OPERATION, 4));
		panic_on(syscall_read_dev(&temp, DEV_DISK_ADDRESS + DEV_DISK_STATUS, 4));
		if (!temp) {
			user_panic("ide_write fail");
		}
	}
}

// lab5-1-extra
#define FLASH2BOLCK 32
int flash_map[FLASH2BOLCK];
int flash_bit_map[FLASH2BOLCK];
int flash_use[FLASH2BOLCK];

void ssd_init() {
	memset(flash_map, -1, sizeof(flash_map));
	memset(flash_bit_map, 1, sizeof(flash_bit_map));
	memset(flash_use, 0, sizeof(flash_use));
}

int ssd_read(u_int logic_no, void *dst) {
	int flash_no = flash_map[logic_no];
	if (flash_no < 0) {
		return -1;
	}
	ide_read(0, flash_no, dst, 1);
	return 0;
}

void ssd_erase_physic(u_int flash_no) {
	int temp[128] = {0};
	ide_write(0, flash_no, temp, 1);
	flash_use[flash_no]++;
	flash_bit_map[flash_no] = 1;
}

int get_new_block() {
	int min = 1<<30, flash_no, min_unwritable = 1<<30, flash_no_unwritable;
	for (int i = 0; i < FLASH2BOLCK; i++) {
		if (flash_bit_map[i] && min > flash_use[i]) {
			min = flash_use[i];
			flash_no = i;
		}
		if (!flash_bit_map[i] && min_unwritable > flash_use[i]) {
			min_unwritable = flash_use[i];
			flash_no_unwritable = i;
		}
	}
	if (min >= 5) {
		int temp[128];
		ide_read(0, flash_no_unwritable, temp, 1);
		ide_write(0, flash_no, temp, 1);
		flash_bit_map[flash_no] = 0;
		for (int i = 0; i < FLASH2BOLCK; i++) {
			if (flash_map[i] == flash_no_unwritable) {
				flash_map[i] = flash_no;
			}
		}
		ssd_erase_physic(flash_no_unwritable);
		return flash_no_unwritable;
	}
	return flash_no;
}

void ssd_write(u_int logic_no, void *src) {
	int flash_no = flash_map[logic_no];
	ssd_erase(logic_no);
	flash_no = get_new_block();
	flash_map[logic_no] = flash_no;
	ide_write(0, flash_no, src, 1);
	flash_bit_map[flash_no] = 0;
}

void ssd_erase(u_int logic_no) {
	int flash_no = flash_map[logic_no];
	if (flash_no < 0) {
		return;
	}
	ssd_erase_physic(flash_no);
	flash_map[logic_no] = -1;
}
