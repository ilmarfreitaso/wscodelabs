#include <sdread.h>
#include <vfs/ext2fs.h>
#include <syscall.h>
#include <unistd.h>
#include <stdlib.h>
#include <device.h>
#include <kstring.h>

static int32_t _typeid = dev_typeid(DEV_SDC, 0);

static int32_t read_block(int32_t block, char* buf) {
	if(syscall2(SYSCALL_DEV_BLOCK_READ, _typeid, block) < 0)
		return -1;

	int32_t res = -1;
	while(true) {
		res = syscall2(SYSCALL_DEV_BLOCK_READ_DONE, _typeid, (int32_t)buf);
		if(res == 0)
			break;
		sleep(0);
	}
	return 0;
}

static int32_t simple_search(INODE *ip, const char *name, char* buf) {
	int32_t i; 
	char c, *cp;
	DIR  *dp;

	for (i=0; i<12; i++){
		if ( ip->i_block[i] ){
			read_block(ip->i_block[i], buf);
			dp = (DIR *)buf;
			cp = buf;
			while (cp < &buf[BLOCK_SIZE]){
				c = dp->name[dp->name_len];  // save last byte
				dp->name[dp->name_len] = 0;   
				if (strcmp(dp->name, name) == 0 ){
					return(dp->inode - 1);
				}
				dp->name[dp->name_len] = c; // restore that last byte
				cp += dp->rec_len;
				dp = (DIR *)cp;
			}
		}
	}
	return -1;
}

#define MAX_DIR_DEPTH 4

static char* ext2_load(const char* filename, int32_t *sz) {
	char* buf1 = (char*)malloc(BLOCK_SIZE);
	int32_t depth, i, ino, iblk, count, u, blk12;
	char name[MAX_DIR_DEPTH][64];
	char *ret, *addr;
	uint32_t *up;
	GD *gp;
	INODE *ip;
	*sz = -1;

	depth = 0;
	while(true) {
		char* hold = name[depth];
		u = 0;
		if(*filename == '/') filename++; //skip '/'

		while(*filename != '/') {
			hold[u] = *filename;
			u++;
			filename++;
			if(*filename == 0 || u >= 63)
				break;
		}
		hold[u] = 0;
		depth++;
		if(*filename != 0)
			filename++;
		else
			break;
	}

	/* read blk#2 to get group descriptor 0 */
	if(read_block(2, buf1) != 0) {
		free(buf1);
		return NULL;
	}
	gp = (GD *)buf1;
	iblk = (uint16_t)gp->bg_inode_table;

	if(read_block(iblk, buf1) != 0) // read first inode block
		return NULL;
	ip = (INODE *)buf1 + 1;   // ip->root inode #2

	char* buf2 = (char*)malloc(BLOCK_SIZE);
	/* serach for system name */
	for (i=0; i<depth; i++) {
		ino = simple_search(ip, name[i], buf2);
		if (ino < 0) {
			free(buf1);
			free(buf2);
			return NULL;
		}
		if(read_block(iblk+(ino/8), buf1) != 0) { // read block inode of me
			free(buf1);
			free(buf2);
			return NULL;
		}
		ip = (INODE *)buf1 + (ino % 8);
	}

	*sz = ip->i_size;
	int32_t mlc_size = ALIGN_UP(*sz, BLOCK_SIZE);
	blk12 = ip->i_block[12];
	addr = (char *)malloc(mlc_size);
	ret = addr;
	/* read indirect block into b2 */

	count = 0;
	for (i=0; i<12 && count<(*sz); i++){
		if (ip->i_block[i] == 0)
			break;
		read_block(ip->i_block[i], addr);
		addr += BLOCK_SIZE;
		count += BLOCK_SIZE;
	}

	if (blk12) { // only if file has indirect blocks
		read_block(blk12, buf1);
		up = (uint32_t *)buf1;      
		while(*up && count < (*sz)){
			read_block(*up, addr);
			addr += BLOCK_SIZE;
			up++; 
			count += BLOCK_SIZE;
		}
	}

	free(buf1);
	free(buf2);
	return ret;
}

char* from_sd(const char *filename, int32_t* sz) { 
	return ext2_load(filename, sz);
}
