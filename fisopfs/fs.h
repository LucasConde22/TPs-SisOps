#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <fuse.h>
#include <time.h>
#include <stdbool.h>

#define SLASH '/' // Slash character for path separation
#define SLASH_STR "/" // String representation of slash
#define ROOT_PREV_PATH "" // Previous path for root directory
#define STRING_END '\0'
#define BINARY_WRITE "wb"
#define BINARY_READ "rb"

#define FS_ERROR -1 // Error code for file system operations
#define BAD_INDEX -1 // Invalid index for inode lookup
#define ROOT_INDEX 0
#define NO_DATA_READ 0 // No data read from file
#define NOT_USED_INODE 0 
#define USED_INODE 1
#define IS_ROOT 1 // Root user has all permissions
#define MIN_FILE_NLINKS 1 // Minimum number of links for a file
#define MIN_DIR_NLINKS 2 // Minimum number of links for a directory
#define MAX_DEPTH 4 // Maximum depth of directories in the file system
#define MAX_PATH_NAME 256 // Maximum length of a path name
#define MAX_DATA 1024 // Maximum size of data in an inode
#define MAX_INODES 256 // Maximum number of inodes in the file system

typedef enum {FILE_TYPE, DIR_TYPE} inode_type_t;

// Inode struct
typedef struct inode {
    char name[MAX_PATH_NAME]; 
	size_t size;
	int uid;           
	int gid;           
	inode_type_t type;
    char path[MAX_PATH_NAME];
	int nlink;
	char prev_path[MAX_PATH_NAME];
    char data[MAX_DATA];   
    mode_t mode;       
	time_t access_time; 
	time_t modification_time;  
	time_t creation_time; 
} inode_t;

// File system struct
typedef struct filesystem_t {
	struct inode inodes[MAX_INODES]; 
	int inodes_bitmap[MAX_INODES];
	size_t inodes_amount;
} filesystem_t;

extern filesystem_t fs;

// File system functions
void fs_initialize();
int fs_add_inode(inode_t *inode);
int fs_create_entry(const char *path, mode_t mode, int type);
int fs_lookup(const char *path);
void modify_nlink_path(const char *path, bool add);

void extract_filename(const char *path, char *out);
void extract_prev_path(const char *path, char *out);
static int find_free_inode_slot(filesystem_t *fs);
int fs_serialize(const char *filename);
int fs_deserialize(const char *filename);

// Constants for messages all around fisopfs and persistence file:
// Persistence namefile:
#define DEFAULT_FILE_DISK "persistence_file.fisopfs"

// Debug messages:
#define LOG_INIT_START "[debug] fisopfs_init - Starting init\n"
#define LOG_NO_PERSIST "[debug] No persistence file found, initializing new FS\n"
#define LOG_FS_LOADED "[debug] Filesystem loaded from disk\n"
#define LOG_DESTROY "[debug] fisopfs_destroy - Saving FS data\n"
#define LOG_FLUSH "[debug] fisopfs_flush - path: %s\n"
#define LOG_GETATTR "[debug] fisopfs_getattr - path: %s\n"
#define LOG_GETATTR_NOT_FOUND "[debug] fisopfs_getattr - path: \"%s\" not found\n"
#define LOG_READDIR "[debug] fisopfs_readdir - path: %s\n"
#define LOG_READDIR_FOUND "[debug] fisopfs_readdir - found: %s\n"
#define LOG_READ "[debug] fisopfs_read - path: %s, offset: %lu, size: %lu\n"
#define LOG_MKDIR "[debug] fisopfs_mkdir - path: %s - mode: %d\n"
#define LOG_CREATE "[debug] fisopfs_create - path: %s - mode: %d\n"
#define LOG_RMDIR "[debug] fisopfs_rmdir - path: %s\n"
#define LOG_WRITE "[debug] fisopfs_write - path: %s - buffer: %s - size: %zu - offset: %ld\n"
#define LOG_TRUNCATE "[debug] fisopfs_truncate - path: %s - size: %ld\n"
#define LOG_UNLINK "[debug] fisopfs_unlink - path: %s"
#define LOG_UTIMENS "[debug] fisopfs_ultimens - path: %s"
#define LOG_ENTRY "[debug] fs_create_entry: path=%s mode=%d type=%d\n"
#define LOG_INIT "[debug] fs_initialize: setting up root directory\n"
#define LOG_SERIALIZE "[debug] fs_serialize - File system saved to '%s'\n"
#define LOG_DESERIALIZE "[debug] fs_deserialize - File system loaded from '%s'\n"
#define LOG_CHOWN "[debug] fisopfs_chown - path: %s, uid: %d, gid: %d\n"
#define LOG_CHMOD "[debug] fisopfs_chmod - path: %s, mode: %o\n"

// Error messages:
#define ERR_SERIALIZE "[debug] Error fisopfs_destroy: Failed to save FS during destroy\n"
#define ERR_FLUSH "[debug] Error fisopfs_flush: Failed to save FS during flush\n"
#define ERR_NOT_DIR "[debug] Error readdir: Not a directory\n"
#define ERR_READ_NOT_FOUND "[debug] fisopfs_read - path: \"%s\" not found\n"
#define ERR_DEPTH "[debug] Error mkdir: max directory depth exceeded\n"
#define ERR_NOT_DIR_RMDIR "[debug] Error rmdir: Not a directory\n"
#define ERR_NOT_EMPTY "[debug] Error rmdir: Directory not empty\n"
#define ERR_RM_ROOT "[debug] Error rmdir: Cannot remove root directory\n"
#define ERR_RM_NOT_FOUND "[debug] fisopfs_rmdir - path: \"%s\" not found\n"
#define ERR_WRITE_TYPE "[debug] Error write: Not a file\n"
#define ERR_WRITE_SPACE "[debug] Error write: not enough space \n"
#define ERR_WRITE_PERM "[debug] Error write: Permission denied\n"
#define ERR_WRITE_NOT_FOUND "[debug] fisopfs_write - path: \"%s\" not found\n"
#define ERR_TRUNC_SIZE "[debug] Error truncate: size exceeded"
#define ERR_TRUNC_NOT_FOUND "[debug] fisopfs_truncate - path: \"%s\" not found\n"
#define ERR_TRUNC_TYPE "[debug] Error truncate: Not a file\n"
#define ERR_TRUNC_PERM "[debug] Error truncate: Permission denied\n"
#define ERR_UNLINK_TYPE "[debug] Error unlink: Not a file\n"
#define ERR_UNLINK_PERM "[debug] Error unlink: Permission denied\n"
#define ERR_UTIMENS_TYPE "[debug] Error ultimens: Not a file\n"
#define ERR_CREATE_INODE "[debug] Error create: Can`t create more inodes"
#define ERR_FS_FOPEN "[debug] fs_serialize - fopen '%s': "
#define ERR_FS_FWRITE "[debug] fs_serialize - fwrite '%s': "
#define ERR_FS_FREAD "[debug] fs_deserialize - fread '%s': "