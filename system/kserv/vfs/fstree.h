#ifndef FS_TREE_H
#define FS_TREE_H

#include "types.h"
#include "tree.h"
#include "fsinfo.h"

#define FSN(n) ((FSNodeT*)((n)->data))

void fsTreeNodeInit(TreeNodeT* node);

TreeNodeT* fsNewNode();

TreeNodeT* fsTreeSimpleGet(TreeNodeT* father, const char* name);

TreeNodeT* fsTreeGet(TreeNodeT* father, const char* name);

TreeNodeT* fsTreeSimpleAdd(TreeNodeT* father, const char* name);


#endif