#include "exec.h"

#define ERROR_PIPE "Error creating pipe"
#define ERROR_OPEN "Error opening file"
#define ERROR_DUP2 "Error executing redirection"
#define ERROR_FORK "Error creating child process"
#define ERROR_SETENV "Error setting environment variables"
#define ERROR_WAITPID "Error waiting for child process"

// sets "key" with the key part of "arg"
// and null-terminates it
//
// Example:
//  - KEY=value
//  arg = ['K', 'E', 'Y', '=', 'v', 'a', 'l', 'u', 'e', '\0']
//  key = "KEY"
//
static void
get_environ_key(char *arg, char *key)
{
	int i;
	for (i = 0; arg[i] != '='; i++)
		key[i] = arg[i];

	key[i] = END_STRING;
}

// sets "value" with the value part of "arg"
// and null-terminates it
// "idx" should be the index in "arg" where "=" char
// resides
//
// Example:
//  - KEY=value
//  arg = ['K', 'E', 'Y', '=', 'v', 'a', 'l', 'u', 'e', '\0']
//  value = "value"
//
static void
get_environ_value(char *arg, char *value, int idx)
{
	size_t i, j;
	for (i = (idx + 1), j = 0; i < strlen(arg); i++, j++)
		value[j] = arg[i];

	value[j] = END_STRING;
}

// sets the environment variables received
// in the command line
//
// Hints:
// - use 'block_contains()' to
// 	get the index where the '=' is
// - 'get_environ_*()' can be useful here
static void
set_environ_vars(char **eargv, int eargc)
{
	char key[BUFLEN];
	char value[BUFLEN];

	for (int i = 0; i < eargc; i++) {
		get_environ_key(eargv[i], key);
		get_environ_value(eargv[i], value, block_contains(eargv[i], '='));

		if (setenv(key, value, 1) < 0) {
			perror(ERROR_SETENV);
		}
	}
}

// opens the file in which the stdin/stdout/stderr
// flow will be redirected, and returns
// the file descriptor
//
// Find out what permissions it needs.
// Does it have to be closed after the execve(2) call?
//
// Hints:
// - if O_CREAT is used, add S_IWUSR and S_IRUSR
// 	to make it a readable normal file
static int
open_redir_fd(char *file, int flags)
{
	int fd;
	if (flags & O_CREAT)  // If one of its flags it's O_CREAT
		fd = open(file, flags, S_IWUSR | S_IRUSR);
	else
		fd = open(file, flags);
	return fd;
}

// Handles redirections from stdin/stdout/stderr to a previously opened file.
static void
handle_redirection(char *file, int flags, int fd_std)
{
	if (strlen(file) > 0) {
		int fd = open_redir_fd(file, flags);
		if (fd < 0) {
			perror(ERROR_OPEN);
			_exit(EXIT_FAILURE);
		}
		if (dup2(fd, fd_std) < 0) {
			perror(ERROR_DUP2);
		}
		close(fd);
	}
}

// executes a command - does not return
//
// Hint:
// - check how the 'cmd' structs are defined
// 	in types.h
// - casting could be a good option
void
exec_cmd(struct cmd *cmd)
{
	// To be used in the different cases
	struct execcmd *e;
	struct backcmd *b;
	struct execcmd *r;
	struct pipecmd *p;

	switch (cmd->type) {
	case EXEC:
		// spawns a command

		e = (struct execcmd *) cmd;
		set_environ_vars(e->eargv, e->eargc);
		execvp(e->argv[0], e->argv);
		fprintf(stderr, "Command '%s' not found\n", e->argv[0]);
		_exit(EXIT_FAILURE);
		break;

	case BACK: {
		// runs a command in background
		b = (struct backcmd *) cmd;
		exec_cmd(b->c);
		exit(EXIT_FAILURE);
		break;
	}

	case REDIR: {
		// changes the input/output/stderr flow
		//
		// To check if a redirection has to be performed
		// verify if file name's length (in the execcmd struct)
		// is greater than zero

		r = (struct execcmd *) cmd;
		if (strlen(r->in_file) > 0) {
			handle_redirection(r->in_file, O_RDONLY, STDIN_FILENO);
		}
		if (strlen(r->out_file)) {
			handle_redirection(r->out_file,
			                   O_WRONLY | O_CREAT | O_TRUNC,
			                   STDOUT_FILENO);
		}
		if (strlen(r->err_file) > 0) {
			if (strcmp(r->err_file, "&1") == 0) {
				if (dup2(STDOUT_FILENO, STDERR_FILENO) <
				    0) {  // The STDOUT alredy redirected
					perror(ERROR_DUP2);
					_exit(EXIT_FAILURE);
				}
			} else {
				handle_redirection(r->err_file,
				                   O_WRONLY | O_CREAT,
				                   STDERR_FILENO);
			}
		}
		r->type = EXEC;
		cmd = (struct cmd *) r;
		exec_cmd(cmd);
		_exit(EXIT_FAILURE);
		break;
	}

	case PIPE: {
		// pipes two commands
		p = (struct pipecmd *) cmd;

		int fd[2];
		if (pipe(fd) < 0) {
			perror(ERROR_PIPE);
			_exit(EXIT_FAILURE);
		}

		int pid = fork();
		if (pid < 0) {
			perror(ERROR_FORK);
			close(fd[READ]);
			close(fd[WRITE]);
			_exit(EXIT_FAILURE);
		} else if (pid == 0) {
			// Leftside process
			setpgid(0, pid);
			close(fd[READ]);
			if (dup2(fd[WRITE], STDOUT_FILENO) < 0) {
				perror(ERROR_DUP2);
				close(fd[WRITE]);
				_exit(EXIT_FAILURE);
			}
			close(fd[WRITE]);
			exec_cmd(p->leftcmd);
			_exit(EXIT_FAILURE);
		}

		int pid2 = fork();
		if (pid2 < 0) {
			perror(ERROR_FORK);
			close(fd[READ]);
			close(fd[WRITE]);
			_exit(EXIT_FAILURE);
		} else if (pid2 == 0) {
			// Rightside process
			setpgid(0, pid);
			close(fd[WRITE]);
			if (dup2(fd[READ], STDIN_FILENO) < 0) {
				perror(ERROR_DUP2);
				close(fd[READ]);
				_exit(EXIT_FAILURE);
			}
			close(fd[READ]);
			exec_cmd(p->rightcmd);
			_exit(EXIT_FAILURE);
		}
		close(fd[READ]);
		close(fd[WRITE]);
		int status1, status2;
		if (waitpid(pid, &status1, 0) < 0 || waitpid(pid2, &status2, 0) < 0) {
			perror(ERROR_WAITPID);
		}
		int exit_code2 = WIFEXITED(status2) ? WEXITSTATUS(status2) : 1;
		_exit(exit_code2);
		break;
		}
	}
}
