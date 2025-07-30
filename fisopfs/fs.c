#include "fs.h"

filesystem_t fs;

// Search for a free inode slot in the filesystem
static int
find_free_inode_slot(filesystem_t *fs)
{
	for (int i = 0; i < MAX_INODES; i++) {
		if (fs->inodes_bitmap[i] == NOT_USED_INODE) {
			return i;
		}
	}
	return BAD_INDEX;
}

// Extracts the filename from the given path
void
extract_filename(const char *path, char *out)
{
	const char *slash = strrchr(path, SLASH);
	if (slash && *(slash + 1) != STRING_END) {
		strcpy(out, slash + 1);
	} else if (slash) {
		strcpy(out, SLASH_STR);
	} else {
		strcpy(out, path);
	}
}

// Extracts the previous path from the given path
void
extract_prev_path(const char *path, char *out)
{
	strcpy(out, path);
	char *slash = strrchr(out, SLASH);
	if (slash == NULL || slash == out) {
		strcpy(out, SLASH_STR);
	} else {
		*slash = STRING_END;
	}
}

// Initialize an inode with default values
void
init_inode(inode_t *inode,
           const char *name,
           const char *path,
           const char *prev_path,
           int type)
{
	memset(inode, 0, sizeof(inode_t));
	inode->size = 0;
	inode->uid = getuid();
	inode->gid = getgid();
	inode->type = type;
	inode->mode =
	        (type == FILE_TYPE) ? (__S_IFREG | 0644) : (__S_IFDIR | 0755);
	inode->access_time = inode->modification_time = inode->creation_time =
	        time(NULL);
	inode->nlink = (type == FILE_TYPE) ? MIN_FILE_NLINKS : MIN_DIR_NLINKS;
	if (name) {
		strcpy(inode->name, name);
	}
	if (path) {
		strcpy(inode->path, path);
	}
	if (prev_path) {
		strcpy(inode->prev_path, prev_path);
	}
	memset(inode->data, 0, sizeof(inode->data));
}
// Add an inode to the filesystem
int
fs_add_inode(inode_t *inode)
{
	int index = find_free_inode_slot(&fs);
	if (index == BAD_INDEX)
		return BAD_INDEX;
	for (int i = 0; i < MAX_INODES; i++) {
		if (fs.inodes_bitmap[i] == USED_INODE &&
		    strcmp(fs.inodes[i].name, inode->name) == 0 &&
		    strcmp(fs.inodes[i].prev_path, inode->prev_path) == 0) {
			fs.inodes[i].nlink++;
			return i;
		}
	}
	fs.inodes[index] = *inode;
	fs.inodes_bitmap[index] = USED_INODE;
	fs.inodes_amount++;
	return index;
}

// Modify the nlink count of an inode at a given path
void
modify_nlink_path(const char *path, bool add)
{
	int index = fs_lookup(path);
	if (index < 0) {
		return;
	}
	if (add) {
		fs.inodes[index].nlink += 1;
	} else if (fs.inodes[index].nlink > 0) {
		fs.inodes[index].nlink -= 1;
	}
}

// create a new entry in the filesystem
int
fs_create_entry(const char *path, mode_t mode, int type)
{
	printf(LOG_ENTRY, path, mode, type);
	if (fs.inodes_amount == MAX_INODES) {
		fprintf(stderr, ERR_CREATE_INODE);
		return -ENOMEM;
	}
	inode_t inode;
	char name[MAX_PATH_NAME], prev_path[MAX_PATH_NAME];
	extract_filename(path, name);
	extract_prev_path(path, prev_path);
	init_inode(&inode, name, path, prev_path, type);
	int index = fs_add_inode(&inode);
	if (index == BAD_INDEX)
		return BAD_INDEX;
	modify_nlink_path(prev_path, true);
	return EXIT_SUCCESS;
}

// search for an inode by its path
int
fs_lookup(const char *path)
{
	if (strcmp(path, SLASH_STR) == 0) {
		return ROOT_INDEX;
	}
	char name[MAX_PATH_NAME], prev_path[MAX_PATH_NAME];
	extract_filename(path, name);
	extract_prev_path(path, prev_path);
	for (int i = 0; i < MAX_INODES; i++) {
		if (fs.inodes_bitmap[i] == USED_INODE &&
		    strcmp(fs.inodes[i].name, name) == 0 &&
		    strcmp(fs.inodes[i].prev_path, prev_path) == 0) {
			return i;
		}
	}
	return BAD_INDEX;
}

// initialize the filesystem
void
fs_initialize()
{
	printf(LOG_INIT);
	memset(&fs, 0, sizeof(filesystem_t));
	inode_t root_inode;
	init_inode(&root_inode, SLASH_STR, SLASH_STR, ROOT_PREV_PATH, DIR_TYPE);
	fs.inodes[ROOT_INDEX] = root_inode;
	fs.inodes_bitmap[ROOT_INDEX] = USED_INODE;
	fs.inodes_amount = 1;
}

// write the filesystem to a file
int
fs_serialize(const char *filename)
{
	FILE *f = fopen(filename, BINARY_WRITE);
	if (f == NULL) {
		fprintf(stderr, ERR_FS_FOPEN, filename);
		perror(NULL);
		return FS_ERROR;
	}
	if (fwrite(&fs, sizeof(fs), 1, f) != 1) {
		fprintf(stderr, ERR_FS_FWRITE, filename);
		perror(NULL);
		fclose(f);
		return FS_ERROR;
	}
	fclose(f);
	printf(LOG_SERIALIZE, filename);
	return EXIT_SUCCESS;
}

// read from the filedisk
int
fs_deserialize(const char *filename)
{
	FILE *f = fopen(filename, BINARY_READ);
	if (f == NULL) {
		fprintf(stderr, ERR_FS_FOPEN, filename);
		perror(NULL);
		return FS_ERROR;
	}
	if (fread(&fs, sizeof(fs), 1, f) != 1) {
		fprintf(stderr, ERR_FS_FREAD, filename);
		perror(NULL);
		fclose(f);
		return FS_ERROR;
	}
	fclose(f);
	printf(LOG_DESERIALIZE, filename);
	return EXIT_SUCCESS;
}