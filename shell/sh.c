#include "defs.h"
#include "types.h"
#include "readline.h"
#include "runcmd.h"
#define ERROR_MALOC "Error while assigning dynamic memory"
#define ERROR_SIGNALSTACK "Error while executing 'signalstack'"
#define ERROR_SIGACTION "Error while executing 'sigaction'"
#define MSG_PROCESO_TERMINADO "\r==> terminado: PID=%d\n$"
#define STACK_SIZE SIGSTKSZ

char prompt[PRMTLEN] = { 0 };

static void
sigchild_handler()
{
	pid_t pid;
	int status;
	if ((pid = waitpid(0, &status, WNOHANG)) > 0) {
		char str[BUFLEN] = { 0 };
		snprintf(str, sizeof(str), MSG_PROCESO_TERMINADO, pid);
		ssize_t _ = write(STDOUT_FILENO, str, strlen(str));
		(void) _;  // leave warnings
	}
}

static void
init_sigaltstack()
{
	stack_t sig_stack;
	sig_stack.ss_sp = malloc(STACK_SIZE);
	if (sig_stack.ss_sp == NULL) {
		perror(ERROR_MALOC);
		exit(EXIT_FAILURE);
	}
	sig_stack.ss_size = STACK_SIZE;
	sig_stack.ss_flags = 0;
	// Alt stack
	if (sigaltstack(&sig_stack, NULL) == -1) {
		perror(ERROR_SIGNALSTACK);
		exit(EXIT_FAILURE);
	}
}

static void
init_sigaction()
{
	struct sigaction sig_act;
	sig_act.sa_handler = sigchild_handler;
	sigemptyset(&sig_act.sa_mask);
	sig_act.sa_flags = SA_RESTART | SA_NOCLDSTOP | SA_ONSTACK;
	// Executes it
	if (sigaction(SIGCHLD, &sig_act, NULL) == -1) {
		perror(ERROR_SIGACTION);
		exit(EXIT_FAILURE);
	}
}

// runs a shell command
static void
run_shell()
{
	char *cmd;

	while ((cmd = read_line(prompt)) != NULL)
		if (run_cmd(cmd) == EXIT_SHELL)
			return;
}

// initializes the shell
// with the "HOME" directory
static void
init_shell()
{
	// Starts alt stack
	init_sigaltstack();
	// Starts sigaction
	init_sigaction();

	char buf[BUFLEN] = { 0 };
	char *home = getenv("HOME");

	if (chdir(home) < 0) {
		snprintf(buf, sizeof buf, "cannot cd to %s ", home);
		perror(buf);
	} else {
		snprintf(prompt, sizeof prompt, "(%s)", home);
	}
}

int
main(void)
{
	init_shell();

	run_shell();

	return 0;
}
