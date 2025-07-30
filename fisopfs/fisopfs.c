#define FUSE_USE_VERSION 30
#include "fs.h"

char *filedisk = DEFAULT_FILE_DISK;

static void *
fisopfs_init(struct fuse_conn_info *conn)
{
	printf(LOG_INIT_START);
	if (fs_deserialize(filedisk) != 0) {
		printf(LOG_NO_PERSIST);
		fs_initialize();
		if (fs_serialize(filedisk) != 0) {
			fprintf(stderr, ERR_SERIALIZE);
		}
	} else {
		printf(LOG_FS_LOADED);
	}
	return NULL;
}

static void
fisopfs_destroy(void *data)
{
	printf(LOG_DESTROY);
	if (fs_serialize(filedisk) != 0) {
		fprintf(stderr, ERR_SERIALIZE);
	}
}

static int
fisopfs_flush(const char *path, struct fuse_file_info *fi)
{
	printf(LOG_FLUSH, path);
	if (fs_serialize(filedisk) != 0) {
		fprintf(stderr, ERR_FLUSH);
		return -EIO;
	}
	return EXIT_SUCCESS;
}


static int
fisopfs_getattr(const char *path, struct stat *st)
{
	printf(LOG_GETATTR, path);
	memset(st, 0, sizeof(struct stat));
	int index = fs_lookup(path);
	if (index == BAD_INDEX) {
		printf(LOG_GETATTR_NOT_FOUND, path);
		return -ENOENT;
	}
	inode_t *inode = &fs.inodes[index];
	st->st_dev = 0;
	st->st_ino = index;
	st->st_uid = inode->uid;
	st->st_gid = inode->gid;
	st->st_mode = inode->mode;
	st->st_nlink = inode->nlink;
	st->st_size = inode->size;
	st->st_atime = inode->access_time;
	st->st_mtime = inode->modification_time;
	st->st_ctime = inode->creation_time;
	return EXIT_SUCCESS;
}

static int
fisopfs_readdir(const char *path,
                void *buffer,
                fuse_fill_dir_t filler,
                off_t offset,
                struct fuse_file_info *fi)
{
	printf(LOG_READDIR, path);
	filler(buffer, ".", NULL, 0);
	filler(buffer, "..", NULL, 0);
	int index = fs_lookup(path);
	if (index == BAD_INDEX) {
		return -ENOENT;
	}
	inode_t *inode = &fs.inodes[index];
	if (inode->type != DIR_TYPE) {
		fprintf(stderr, ERR_NOT_DIR_RMDIR);
		return -ENOTDIR;
	}
	for (int i = 0; i < MAX_INODES; i++) {
		if (fs.inodes_bitmap[i] == USED_INODE &&
		    strcmp(fs.inodes[i].prev_path, inode->path) == 0) {
			printf(LOG_READDIR, fs.inodes[i].name);
			filler(buffer, fs.inodes[i].name, NULL, 0);
		}
	}
	return EXIT_SUCCESS;
}

static int
fisopfs_read(const char *path,
             char *buffer,
             size_t size,
             off_t offset,
             struct fuse_file_info *fi)
{
	printf(LOG_READ, path, offset, size);
	int index = fs_lookup(path);
	if (index == BAD_INDEX) {
		printf(ERR_READ_NOT_FOUND, path);
		return -ENOENT;
	}
	inode_t *inode = &fs.inodes[index];
	if (inode->type != FILE_TYPE) {
		return -EISDIR;
	}
	if (offset >= inode->size) {
		return NO_DATA_READ;
	}
	size_t len = inode->size - offset;
	if (len > size) {
		len = size;
	}
	memcpy(buffer, inode->data + offset, len);
	inode->access_time = time(NULL);
	return len;
}

// Check the depth of the path to ensure it does not exceed MAX_DEPTH
static int
path_depth(const char *path)
{
	int depth = 0;
	const char *p = path;
	if (*p == SLASH) {
		p++;
	}
	while (*p) {
		if (*p == SLASH) {
			depth++;
		}
		p++;
	}
	if (strcmp(path, SLASH_STR) != 0 && strlen(path) > 0) {
		depth++;
	}
	return depth;
}

static int
fisopfs_mkdir(const char *path, mode_t mode)
{
	printf(LOG_MKDIR, path, mode);
	if (path_depth(path) > MAX_DEPTH) {
		fprintf(stderr, ERR_DEPTH);
		return -ENAMETOOLONG;
	}
	if (fs_lookup(path) >= 0) {
		return -EEXIST;
	}
	return fs_create_entry(path, mode, DIR_TYPE);
}

static int
fisopfs_create(const char *path, mode_t mode, struct fuse_file_info *fi)
{
	printf(LOG_CREATE, path, mode);
	if (fs_lookup(path) >= 0)
		return -EEXIST;
	return fs_create_entry(path, mode, FILE_TYPE);
}

static int
fisopfs_rmdir(const char *path)
{
	printf(LOG_RMDIR, path);
	int index = fs_lookup(path);
	if (index == BAD_INDEX) {
		printf(ERR_RM_NOT_FOUND, path);
		return -ENOENT;
	}
	inode_t *inode = &fs.inodes[index];
	if (inode->type != DIR_TYPE) {
		fprintf(stderr, ERR_NOT_DIR_RMDIR);
		return -ENOTDIR;
	}
	if (inode->nlink > MIN_DIR_NLINKS) {
		fprintf(stderr, ERR_NOT_EMPTY);
		return -ENOTEMPTY;
	}
	if (strcmp(inode->path, SLASH_STR) == 0) {
		fprintf(stderr, ERR_RM_ROOT);
		return -EPERM;
	}
	fs.inodes_bitmap[index] = NOT_USED_INODE;
	modify_nlink_path(inode->prev_path, false);
	memset(inode, 0, sizeof(inode_t));
	return EXIT_SUCCESS;
}

static int
fisopfs_write(const char *path,
              const char *buffer,
              size_t size,
              off_t offset,
              struct fuse_file_info *fi)
{
	printf(LOG_WRITE, path, buffer, size, offset);
	int index = fs_lookup(path);
	if (index == BAD_INDEX) {
		printf(ERR_WRITE_NOT_FOUND, path);
		return -ENOENT;
	}
	inode_t *inode = &fs.inodes[index];
	if (inode->type != FILE_TYPE) {
		fprintf(stderr, ERR_WRITE_TYPE);
		return -EISDIR;
	}
	if (inode->size < offset) {
		memset(inode->data + inode->size, 0, offset - inode->size);
	}
	if (offset + size > MAX_DATA) {
		fprintf(stderr, ERR_WRITE_SPACE);
		return -ENOSPC;
	}
	struct fuse_context *context = fuse_get_context();
	if (inode->uid != context->uid) {
		fprintf(stderr, ERR_WRITE_PERM);
		return -EACCES;
	}
	memcpy(inode->data + offset, buffer, size);
	inode->access_time = time(NULL);
	inode->modification_time = time(NULL);
	if (inode->size < offset + size) {
		inode->size = offset + size;
	}
	return (int) size;
}

static int
fisopfs_truncate(const char *path, off_t size)
{
	printf(LOG_TRUNCATE, path, size);
	if (size > MAX_DATA) {
		fprintf(stderr, ERR_TRUNC_SIZE);
		return -EINVAL;
	}

	int index = fs_lookup(path);
	if (index == BAD_INDEX) {
		fprintf(stderr, ERR_TRUNC_NOT_FOUND, path);
		return -ENOENT;
	}
	inode_t *inode = &fs.inodes[index];
	if (inode->type != FILE_TYPE) {
		fprintf(stderr, ERR_TRUNC_TYPE);
		return -EISDIR;
	}
	struct fuse_context *context = fuse_get_context();
	if (inode->uid != context->uid) {
		fprintf(stderr, ERR_TRUNC_PERM);
		return -EACCES;
	}
	inode->modification_time = time(NULL);
	if (size < inode->size) {
		memset(inode->data + size,
		       0,
		       inode->size - size);  // Clear the data beyond the new size
	}
	inode->size = size;
	return EXIT_SUCCESS;
}

static int
fisopfs_unlink(const char *path)
{
	printf(LOG_UNLINK, path);
	int index = fs_lookup(path);
	if (index == BAD_INDEX) {
		return -ENOENT;
	}
	inode_t *inode = &fs.inodes[index];
	if (inode->type != FILE_TYPE) {
		fprintf(stderr, ERR_UNLINK_TYPE);
		return -EISDIR;
	}
	struct fuse_context *context = fuse_get_context();
	if (context->uid != 0 && inode->uid != context->uid) {
		fprintf(stderr, ERR_UNLINK_PERM);
		return -EACCES;
	}
	fs.inodes_bitmap[index] = NOT_USED_INODE;
	memset(inode, 0, sizeof(inode_t));
	return EXIT_SUCCESS;
}

static int
fisopfs_utimens(const char *path, const struct timespec tv[2])
{
	printf(LOG_UTIMENS, path);
	int index = fs_lookup(path);
	if (index == BAD_INDEX) {
		return -ENOENT;
	}
	inode_t *inode = &fs.inodes[index];
	if (tv == NULL) {  // By FUSE documentation, tv can be NULL
		time_t now = time(NULL);
		inode->access_time = now;
		inode->modification_time = now;
	} else {
		inode->access_time = tv[0].tv_sec;
		inode->modification_time = tv[1].tv_sec;
	}
	return EXIT_SUCCESS;
}

// Check if the user has read permission for the inode
static int
has_read_permission(const inode_t *inode, uid_t uid, gid_t gid)
{
	if (uid == 0) {
		return IS_ROOT;
	}
	if (uid == inode->uid) {
		return (inode->mode & S_IRUSR);
	}
	if (gid == inode->gid) {
		return (inode->mode & S_IRGRP);
	}
	return (inode->mode & S_IROTH);
}

static int
fisopfs_chown(const char *path, uid_t uid, gid_t gid)
{
	printf(LOG_CHOWN, path, uid, gid);
	int index = fs_lookup(path);
	if (index == BAD_INDEX) {
		return -ENOENT;
	}
	inode_t *inode = &fs.inodes[index];
	struct fuse_context *context = fuse_get_context();
	if (context->uid != 0) {
		return -EPERM;
	}
	if (uid != (uid_t) -1) {
		inode->uid = uid;
	}
	if (gid != (gid_t) -1) {
		inode->gid = gid;
	}
	inode->modification_time = time(NULL);
	return EXIT_SUCCESS;
}

static int
fisopfs_chmod(const char *path, mode_t mode)
{
	printf(LOG_CHMOD, path, mode);
	int index = fs_lookup(path);
	if (index == BAD_INDEX) {
		return -ENOENT;
	}
	inode_t *inode = &fs.inodes[index];
	struct fuse_context *context = fuse_get_context();
	if (context->uid != 0 && context->uid != inode->uid) {
		return -EPERM;
	}
	inode->mode = (inode->mode & ~07777) | (mode & 07777);
	inode->modification_time = time(NULL);
	return EXIT_SUCCESS;
}


static struct fuse_operations operations = {
	.getattr = fisopfs_getattr,
	.readdir = fisopfs_readdir,
	.read = fisopfs_read,
	.write = fisopfs_write,
	.mkdir = fisopfs_mkdir,
	.init = fisopfs_init,
	.unlink = fisopfs_unlink,
	.create = fisopfs_create,
	.rmdir = fisopfs_rmdir,
	.truncate = fisopfs_truncate,
	.utimens = fisopfs_utimens,
	.destroy = fisopfs_destroy,
	.flush = fisopfs_flush,
	.chown = fisopfs_chown,
	.chmod = fisopfs_chmod,
};

int
main(int argc, char *argv[])
{
	for (int i = 1; i < argc - 1; i++) {
		if (strcmp(argv[i], "--filedisk") == 0) {
			filedisk = argv[i + 1];
			// We remove the argument so that fuse doesn't use our
			// argument or name as folder.
			// Equivalent to a pop.
			for (int j = i; j < argc - 1; j++) {
				argv[j] = argv[j + 2];
			}
			argc = argc - 2;
			break;
		}
	}
	return fuse_main(argc, argv, &operations, NULL);
}