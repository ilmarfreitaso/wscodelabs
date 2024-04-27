#include <kserv.h>
#include <vfs/fs.h>
#include <vprintf.h>
#include <vfs/vfs.h>
#include <ipc.h>
#include <kstring.h>
#include <syscall.h>
#include <stdlib.h>
#include <proto.h>
#include <unistd.h>

#define FS_BUF_SIZE (16*KB)

int fs_open(const char* name, int32_t flags) {
	int fd = vfs_open(name, flags);
	if(fd < 0)
		return -1;

	fs_info_t info;
	if(syscall2(SYSCALL_PFILE_NODE_BY_FD, fd, (int32_t)&info) != 0) {
		fs_close(fd);
		return -1;
	}
	if(info.dev_serv_pid == 0) 
		return fd;

	proto_t* proto = proto_new(NULL, 0);
	proto_add_int(proto, info.node);
	proto_add_int(proto, flags);

	package_t* pkg = ipc_req(info.dev_serv_pid, 0, FS_OPEN, proto->data, proto->size, true);
	proto_free(proto);
	if(pkg == NULL || pkg->type == PKG_TYPE_ERR) {
		if(pkg != NULL) free(pkg);
		return -1;
	}
	free(pkg);
	return fd;
}

int fs_pipe_open(int fds[2]) {
	return vfs_pipe_open(fds);
}

int fs_close(int fd) {
	fs_info_t info;
	if(fd < 0 || syscall2(SYSCALL_PFILE_NODE_BY_FD, fd, (int32_t)&info) != 0)
		return -1;
	if(vfs_close(fd) != 0)
		return -1;
	if(info.dev_serv_pid > 0)
		ipc_req(info.dev_serv_pid, 0, FS_CLOSE, &info, sizeof(fs_info_t), false);
	return 0;
}

int fs_remove(const char* name) {
	if(name[0] == 0)
		return -1;
	fs_info_t info;
	if(vfs_node_by_name(name, &info) != 0)
		return -1;

	if(info.dev_serv_pid > 0) {	
		package_t* pkg = ipc_req(info.dev_serv_pid, 0, FS_REMOVE, &info, sizeof(fs_info_t), true);
		if(pkg == NULL || pkg->type == PKG_TYPE_ERR) {
			if(pkg != NULL) free(pkg);
			return -1;
		}
		free(pkg);
	}
	return vfs_del(info.node);
}

int32_t fs_dma(int fd, uint32_t* size) {
	*size = 0;
	fs_info_t info;
	if(fd < 0 || syscall2(SYSCALL_PFILE_NODE_BY_FD, fd, (int32_t)&info) != 0)
		return -1;
	if(info.dev_serv_pid == 0) 
		return -1;
	package_t* pkg = ipc_req(info.dev_serv_pid, 0, FS_DMA, &info.node, 4, true);
	if(pkg == NULL || pkg->type == PKG_TYPE_ERR) {
		if(pkg != NULL) free(pkg);
		return -1;
	}

	proto_t* proto = proto_new(get_pkg_data(pkg), pkg->size);
	int32_t ret = proto_read_int(proto);
	*size = proto_read_int(proto);
	proto_free(proto);
	free(pkg);
	return ret;
}

int32_t fs_flush(int fd) {
	fs_info_t info;
	if(fd < 0 || syscall2(SYSCALL_PFILE_NODE_BY_FD, fd, (int32_t)&info) != 0)
		return -1;
	
	if(info.dev_serv_pid > 0) {
		ipc_req(info.dev_serv_pid, 0, FS_FLUSH, &info.node, 4, false);
		/*
		package_t* pkg = ipc_req(info.dev_serv_pid, 0, FS_FLUSH, (void*)&node, 4, true);
		if(pkg == NULL || pkg->type == PKG_TYPE_ERR) {
			if(pkg != NULL) free(pkg);
			return -1;
		}
		free(pkg);
		*/
	}
	return 0;
}

int fs_read(int fd, char* buf, uint32_t size) {
	fs_info_t info;
	if(fd < 0 || syscall2(SYSCALL_PFILE_NODE_BY_FD, fd, (int32_t)&info) != 0)
		return -1;
	uint32_t buf_size = size >= FS_BUF_SIZE ? FS_BUF_SIZE : 0;
	int seek = syscall1(SYSCALL_PFILE_GET_SEEK, fd);
	if(seek < 0)
		return -1;

	proto_t* proto = proto_new(NULL, 0);
	proto_add_int(proto, info.node);
	proto_add_int(proto, size);
	proto_add_int(proto, seek);
	package_t* pkg = ipc_req(info.dev_serv_pid, buf_size, FS_READ, proto->data, proto->size, true);
	proto_free(proto);

	errno = ENONE;
	if(pkg == NULL || pkg->type == PKG_TYPE_ERR || pkg->type == PKG_TYPE_AGAIN) {
		if(pkg->type == PKG_TYPE_AGAIN)
			errno = EAGAIN;
		if(pkg != NULL) free(pkg);
		return -1;
	}

	int sz = pkg->size;
	if(sz == 0) {
		free(pkg);
		return 0;
	}
	
	memcpy(buf, get_pkg_data(pkg), sz);
	free(pkg);

	seek += sz;
	syscall2(SYSCALL_PFILE_SEEK, fd, seek);
	return sz;
}

int fs_ctrl(int fd, int32_t cmd, void* input, uint32_t isize, void* output, uint32_t osize) {
	fs_info_t info;
	if(fd < 0 || syscall2(SYSCALL_PFILE_NODE_BY_FD, fd, (int32_t)&info) != 0)
		return -1;

	proto_t* proto = proto_new(NULL, 0);
	proto_add_int(proto, info.node);
	proto_add_int(proto, cmd);
	proto_add(proto, input, isize);
	package_t* pkg = ipc_req(info.dev_serv_pid, 0, FS_CTRL, proto->data, proto->size, true);
	proto_free(proto);

	errno = ENONE;
	if(pkg == NULL || pkg->type == PKG_TYPE_ERR || pkg->type == PKG_TYPE_AGAIN) {
		if(pkg->type == PKG_TYPE_AGAIN)
			errno = EAGAIN;
		return -1;
	}

	if(output == NULL || osize == 0) {
		free(pkg);
		return 0;
	}
	uint32_t sz = pkg->size;

	if(sz > osize)
		sz = osize;
	memcpy(output, get_pkg_data(pkg), sz);
	free(pkg);
	return 0;
}

int fs_write(int fd, const char* buf, uint32_t size) {
	fs_info_t info;
	if(fd < 0 || syscall2(SYSCALL_PFILE_NODE_BY_FD, fd, (int32_t)&info) != 0)
		return -1;
	uint32_t buf_size = size >= FS_BUF_SIZE ? FS_BUF_SIZE : 0;

	int seek = syscall1(SYSCALL_PFILE_GET_SEEK, fd);
	if(seek < 0)
		return -1;

	proto_t* proto = proto_new(NULL, 0);
	proto_add_int(proto, info.node);
	proto_add(proto, (void*)buf, size);
	proto_add_int(proto, seek);
	package_t* pkg = ipc_req(info.dev_serv_pid, buf_size, FS_WRITE, proto->data, proto->size, true);
	proto_free(proto);

	errno = ENONE;
	if(pkg == NULL || pkg->type == PKG_TYPE_ERR || pkg->type == PKG_TYPE_AGAIN) {
		if(pkg->type == PKG_TYPE_AGAIN)
			errno = EAGAIN;
		if(pkg != NULL) free(pkg);
		return -1;
	}

	int sz = *(int*)get_pkg_data(pkg);
	free(pkg);

	seek += sz;
	syscall2(SYSCALL_PFILE_SEEK, fd, seek);
	return sz;
}

uint32_t fs_add(int fd, const char* name, uint32_t type) {
	fs_info_t info;
	if(fd < 0 || syscall2(SYSCALL_PFILE_NODE_BY_FD, fd, (int32_t)&info) != 0)
		return 0;
	
	int size = strlen(name);
	if(size == 0)
		return 0;

	proto_t* proto = proto_new(NULL, 0);
	proto_add_int(proto, info.node);
	proto_add_str(proto, name);
	proto_add_int(proto, type);
	package_t* pkg = ipc_req(info.dev_serv_pid, 0, FS_ADD, proto->data, proto->size, true);
	proto_free(proto);

	if(pkg == NULL || pkg->type == PKG_TYPE_ERR) {
		if(pkg != NULL) free(pkg);
		return 0;
	}
	int32_t res = *(int32_t*)get_pkg_data(pkg);
	free(pkg);
	if(res != 0)
		return 0;

	uint32_t node = 0;
	if(type == FS_TYPE_DIR)
		node = vfs_add(info.node, name, VFS_DIR_SIZE, NULL);
	else
		node = vfs_add(info.node, name, 0, NULL);
	return node;
}

int fs_getch(int fd) {
	char buf[1];
	if(fs_read(fd, buf, 1) != 1)
		return 0;
	return buf[0];
}

int fs_putch(int fd, int c) {
	char buf[1];
	buf[0] = (char)c;
	return fs_write(fd, buf, 1);
}

int fs_finfo(const char* name, fs_info_t* info) {
	if(name[0] == 0)
		return -1;
	return vfs_node_by_name(name, info);
}

int fs_info(int fd, fs_info_t* info) {
	if(fd < 0 || syscall2(SYSCALL_PFILE_NODE_BY_FD, fd, (int32_t)info) != 0)
		return -1;
	return 0;
}

int fs_ninfo(uint32_t node, fs_info_t* info) {
	if(node == 0 || syscall2(SYSCALL_PFILE_NODE_BY_ADDR, node, (int32_t)info) != 0)
		return -1;
	return 0;
}

int fs_ninfo_update(fs_info_t* info) {
	if(vfs_node_update(info) != 0)
		return -1;
	return 0;
}

int fs_kid(int fd, int32_t index, fs_info_t* kid) {
	fs_info_t info;
	if(fd < 0 || syscall2(SYSCALL_PFILE_NODE_BY_FD, fd, (int32_t)&info) != 0)
		return -1;
	return vfs_kid(info.node, index, kid);
}

char* fs_read_file(const char* fname, int32_t *size) {
	int fd = open(fname, O_RDONLY);
	if(fd < 0) 
		return NULL;

	fs_info_t info;
	if(fs_info(fd, &info) != 0 || info.size <= 0) {
		fs_close(fd);
		return NULL;
	}

	char* buf = (char*)malloc(info.size);
	int res = read(fd, buf, info.size);
	fs_close(fd);

	if(res <= 0) {
		free(buf);
		buf = NULL;
		*size = 0;
	}
	*size = info.size;
	return buf;
}

int32_t fs_full_name(const char* fname, char* full, uint32_t full_len) {
	char pwd[FULL_NAME_MAX];
	getcwd(pwd, FULL_NAME_MAX-1);

	if(fname[0] == 0) {
		strncpy(full, pwd, full_len-1);
	}
	else if(fname[0] == '/') {
		strcpy(full, fname);
	}
	else {
		if(strcmp(pwd, "/") == 0)
			snprintf(full, full_len-1, "/%s", fname);
		else
			snprintf(full, full_len-1, "%s/%s", pwd, fname);
	}
	return 0;
}

int32_t fs_parse_name(const char* fname, char* dir, uint32_t dir_len, char* name, uint32_t name_len) {
	char full[FULL_NAME_MAX];
	fs_full_name(fname, full, FULL_NAME_MAX);
	int32_t i = strlen(full);
	while(i >= 0) {
		if(full[i] == '/') {
			full[i] = 0;
			break;
		}
		--i;	
	}
	strncpy(dir, full, dir_len-1); 
	strncpy(name, full+i+1, name_len-1); 
	if(dir[0] == 0)
		strcpy(dir, "/");
	return 0;
}
