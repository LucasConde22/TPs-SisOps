#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

#ifndef NARGS
#define NARGS 4
#endif

void
ejecutar_programa(char *argumentos[])
{
	int pid = fork();

	if (pid == 0) {
		execvp(argumentos[0], argumentos);
	} else if (pid > 0) {
		wait((int *) 0);
	} else {
		fprintf(stderr, "Se produjo un error en la creación del hijo del proceso %d.", pid);
	}
}

void
reemplazar_n(char *cadena)
{
	for (int i = 0; cadena[i] != '\0'; i++) {
		if (cadena[i] == '\n') {
			cadena[i] = '\0';
		}
	}
}

int
main(int argc, char *argv[])
{
	if (argc < 2) {
		fprintf(stderr, "El parámetro no fue pasado correctamente.");
	}

	size_t tam = 0;
	char *argumentos[NARGS + 2] = {
		argv[1]
	};  // Buffer. En la primera posición se guarda el nombre del programa,
	    // en las NARGS del medio los argumentos y en la última un 0.
	int i = 1;

	while (getline(&argumentos[i], &tam, stdin) != -1) {
		reemplazar_n(argumentos[i]);
		i++;

		if (i == NARGS + 1) {
			argumentos[i] = 0;
			ejecutar_programa(argumentos);
			i = 1;
		}
	}

	if (i != 1) {
		argumentos[i] = 0;
		ejecutar_programa(argumentos);
	}

	for (int i = 1; i < NARGS + 1; i++) {
		free(argumentos[i]);
	}
	return 0;
}
