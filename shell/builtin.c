#include "builtin.h"

#define ERROR_GETENV "Error getting environment variable"
#define ERROR_CHDIR "Error changing directory"
#define ERROR_GETCWD "Error while executing getcwd"

// returns true if the 'exit' call
// should be performed
//
// (It must not be called from here)
int
exit_shell(char *cmd)
{
	if (strncmp(cmd, "exit", 4) ==
	    0) {  // If cmd starts with 'exit', nevermind what follows
		return 1;
	}
	return 0;
}

// returns true if "chdir" was performed
//  this means that if 'cmd' contains:
// 	1. $ cd directory (change to 'directory')
// 	2. $ cd (change to $HOME)
//  it has to be executed and then return true
//
//  Remember to update the 'prompt' with the
//  	new directory.
//
// Examples:
//  1. cmd = ['c','d', ' ', '/', 'b', 'i', 'n', '\0']
//  2. cmd = ['c','d', '\0']
int
cd(char *cmd)
{
	if (strncmp(cmd, "cd", 2) != 0) {
		return 0;
	}
	if (cmd[2] == '\0') {
		// If cmd is only cd, the directory change is HOME
		char *home = getenv("HOME");
		if (home == NULL) {
			perror(ERROR_GETENV);
			return 0;
		}
		if (chdir(home) < 0) {
			perror(ERROR_GETENV);
			return 0;
		}
	} else {  // This case is another directory
		char *no_cd = cmd + 3;
		if (chdir(no_cd) < 0) {
			perror(ERROR_CHDIR);
			return 0;
		}
	}
	if (getcwd(prompt, BUFLEN) == NULL) {
		perror(ERROR_GETCWD);
		return 0;
	}
	return 1;
}

// returns true if 'pwd' was invoked
// in the command line
//
// (It has to be executed here and then
// 	return true)
int
pwd(char *cmd)
{
	if (strncmp(cmd, "pwd", 3) != 0) {
		return 0;
	}
	char buff[BUFLEN];
	if (getcwd(buff, sizeof(buff)) == NULL) {
		perror(ERROR_GETCWD);
		return 0;
	}
	printf("%s\n", buff);
	return 1;
}

// returns true if `history` was invoked
// in the command line
//
// (It has to be executed here and then
// 	return true)
int
history(char *cmd)
{
	// Your code here

	return 0;
}
