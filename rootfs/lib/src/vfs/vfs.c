#include <vfs/vfs.h>
#include <vfs/vfscmd.h>
#include <kserv.h>
#include <proto.h>
#include <stdlib.h>
#include <ipc.h>
#include <package.h>

#define VFS_NAME "kserv.vfsd"

inline uint32_t vfs_add(uint32_t node, const char* name, uint32_t size, void* data) {
	int32_t serv_pid = kserv_get_pid(VFS_NAME);	
	if(serv_pid < 0)
		return 0;

	proto_t* proto = proto_new(NULL, 0);
	proto_add_int(proto, (int32_t)node);
	proto_add_str(proto, name);
	proto_add_int(proto, (int32_t)size);
	proto_add_int(proto, (int32_t)data);

	package_t* pkg = ipc_req(serv_pid, 0, VFS_CMD_ADD, proto->data, proto->size, true);
	proto_free(proto);
	if(pkg == NULL || pkg->type == PKG_TYPE_ERR) {
		if(pkg != NULL) free(pkg);
		return 0;
	}

	uint32_t ret = *(uint32_t*)get_pkg_data(pkg);
	free(pkg);
	return ret;
}

inline int32_t vfs_del(uint32_t node) {
	int32_t serv_pid = kserv_get_pid(VFS_NAME);	
	if(serv_pid < 0)
		return -1;

	package_t* pkg = ipc_req(serv_pid, 0, VFS_CMD_DEL, &node, 4, true);
	if(pkg == NULL || pkg->type == PKG_TYPE_ERR) {
		if(pkg != NULL) free(pkg);
		return -1;
	}
	free(pkg);
	return 0;
}

inline int32_t vfs_node_by_name(const char* fname, fs_info_t* info) {
	int32_t serv_pid = kserv_get_pid(VFS_NAME);	
	if(serv_pid < 0)
		return -1;

	proto_t* proto = proto_new(NULL, 0);
	proto_add_str(proto, fname);
	package_t* pkg = ipc_req(serv_pid, 0, VFS_CMD_NODE_BY_NAME, proto->data, proto->size, true);
	proto_free(proto);
	if(pkg == NULL || pkg->type == PKG_TYPE_ERR) {
		if(pkg != NULL) free(pkg);
		return -1;
	}
	memcpy(info, get_pkg_data(pkg), sizeof(fs_info_t));
	free(pkg);
	return 0;
}

inline int32_t vfs_node_update(fs_info_t* info) {
	int32_t serv_pid = kserv_get_pid(VFS_NAME);	
	if(serv_pid < 0)
		return -1;

	package_t* pkg = ipc_req(serv_pid, 0, VFS_CMD_INFO_UPDATE, info, sizeof(fs_info_t), true);
	if(pkg == NULL || pkg->type == PKG_TYPE_ERR) {
		if(pkg != NULL) free(pkg);
		return -1;
	}
	free(pkg);
	return 0;
}

inline uint32_t vfs_mount(const char* fname, const char* devName, int32_t devIndex, bool isFile) {
	int32_t serv_pid = kserv_get_pid(VFS_NAME);	
	if(serv_pid < 0)
		return 0;

	proto_t* proto = proto_new(NULL, 0);
	proto_add_str(proto, fname);
	proto_add_str(proto, devName);
	proto_add_int(proto, devIndex);
	proto_add_int(proto, (int32_t)isFile);
	package_t* pkg = ipc_req(serv_pid, 0, VFS_CMD_MOUNT, proto->data, proto->size, true);
	proto_free(proto);
	if(pkg == NULL || pkg->type == PKG_TYPE_ERR) {
		if(pkg != NULL) free(pkg);
		return 0;
	}
	uint32_t node = *(uint32_t*)get_pkg_data(pkg);
	free(pkg);
	return node;
}

inline int32_t vfs_mounted(const char* fname) {
	int32_t serv_pid = kserv_get_pid(VFS_NAME);	
	if(serv_pid < 0)
		return 0;

	proto_t* proto = proto_new(NULL, 0);
	proto_add_str(proto, fname);
	package_t* pkg = ipc_req(serv_pid, 0, VFS_CMD_MOUNTED, proto->data, proto->size, true);
	proto_free(proto);
	if(pkg == NULL || pkg->type == PKG_TYPE_ERR) {
		if(pkg != NULL) free(pkg);
		return -1;
	}
	free(pkg);
	return 0;
}

inline int32_t vfs_unmount(uint32_t node) {
	int32_t serv_pid = kserv_get_pid(VFS_NAME);	
	if(serv_pid < 0)
		return -1;

	package_t* pkg = ipc_req(serv_pid, 0, VFS_CMD_UNMOUNT, &node, 4, true);
	if(pkg == NULL || pkg->type == PKG_TYPE_ERR) {
		if(pkg != NULL) free(pkg);
		return -1;
	}
	free(pkg);
	return 0;

}

int32_t vfs_kid(uint32_t node, int32_t index, fs_info_t* info) {
	int32_t serv_pid = kserv_get_pid(VFS_NAME);	
	if(serv_pid < 0)
		return -1;

	proto_t* proto = proto_new(NULL, 0);
	proto_add_int(proto, node);
	proto_add_int(proto, index);
	package_t* pkg = ipc_req(serv_pid, 0, VFS_CMD_KID, proto->data, proto->size, true);
	proto_free(proto);
	if(pkg == NULL || pkg->type == PKG_TYPE_ERR || pkg->size == 0) {
		if(pkg != NULL) free(pkg);
		return -1;
	}
	memcpy(info, get_pkg_data(pkg), pkg->size);
	free(pkg);
	return 0;
}

int32_t vfs_mount_by_index(int32_t index, mount_t* mnt) {
	int32_t serv_pid = kserv_get_pid(VFS_NAME);	
	if(serv_pid < 0)
		return -1;

	package_t* pkg = ipc_req(serv_pid, 0, VFS_CMD_MOUNT_BY_INDEX, &index, 4, true);
	if(pkg == NULL || pkg->type == PKG_TYPE_ERR || pkg->size == 0) {
		if(pkg != NULL) free(pkg);
		return -1;
	}
	memcpy(mnt, get_pkg_data(pkg), pkg->size);
	free(pkg);
	return 0;
}

int32_t vfs_mount_by_fname(const char* fname, mount_t* mnt) {
	int32_t serv_pid = kserv_get_pid(VFS_NAME);	
	if(serv_pid < 0)
		return -1;

	proto_t* proto = proto_new(NULL, 0);
	proto_add_str(proto, fname);
	package_t* pkg = ipc_req(serv_pid, 0, VFS_CMD_MOUNT_BY_FNAME, proto->data, proto->size, true);
	proto_free(proto);
	if(pkg == NULL || pkg->type == PKG_TYPE_ERR || pkg->size == 0) {
		if(pkg != NULL) free(pkg);
		return -1;
	}
	memcpy(mnt, get_pkg_data(pkg), pkg->size);
	free(pkg);
	return 0;
}

//get the full name by node.
int32_t vfs_node_full_name(uint32_t node, char* full, uint32_t len) {
	full[0] = 0;
	int32_t serv_pid = kserv_get_pid(VFS_NAME);	
	if(serv_pid < 0)
		return -1;

	package_t* pkg = ipc_req(serv_pid, 0, VFS_CMD_NODE_FULLNAME, &node, 4, true);
	if(pkg == NULL || pkg->type == PKG_TYPE_ERR || pkg->size == 0) {
		if(pkg != NULL) free(pkg);
		return -1;
	}
	strncpy(full, (char*)get_pkg_data(pkg), len-1);
	free(pkg);
	return 0;
}

