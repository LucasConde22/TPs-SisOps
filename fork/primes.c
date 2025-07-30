#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

void
error_creando_proceso(int pid)
{
	fprintf(stderr,
	        "Se produjo un error en la creaci%cn del hijo del proceso %d.",
	        99,
	        pid);
}

void
error_escritura(int n, int pfd)
{
	fprintf(stderr, "Se produjo un error en la escritura de %d en %d", n, pfd);
}

void
error_pipe()
{
	fprintf(stderr, "Se produjo un error en creaci%cn del pipe", 99);
}

void
error_parametro()
{
	fprintf(stderr, "El parámetro no fue pasado correctamente.");
}

void
cadena_pipes(int hijo_lee_padre[2])
{
	close(hijo_lee_padre[1]);  // Cierro la escritura en el padre.

	int p;
	if (read(hijo_lee_padre[0], &p, sizeof(p)) <=
	    0) {  // Si ya no lee bytes, termina el "flujo" de creación de procesos.
		exit(0);
	}
	printf("primo %d\n", p);

	int padre_escribe_hijo[2];
	if (pipe(padre_escribe_hijo) < 0) {
		error_pipe();
		exit(1);
	}
	int pid = fork();

	if (pid == 0) {
		close(hijo_lee_padre[0]);  // Como estoy en el hijo, no importa lo que el abuelo le escriba al padre.
		cadena_pipes(padre_escribe_hijo);

	} else if (pid > 0) {
		close(padre_escribe_hijo[0]);  // Como estoy en el padre, no puede leer a su hijo.

		int n;
		while (read(hijo_lee_padre[0], &n, sizeof(n)) > 0) {
			if (n % p != 0) {
				if (write(padre_escribe_hijo[1], &n, sizeof(n)) <
				    0) {
					error_escritura(n, padre_escribe_hijo[1]);
				}
			}
		}

		close(hijo_lee_padre[0]);
		close(padre_escribe_hijo[1]);
		wait((int *) 0);

	} else {
		error_creando_proceso(getpid());
		exit(1);
	}

	return;
}

int
main(int argc, char *argv[])
{
	if (argc < 2) {
		error_parametro();
		return 1;
	}

	int pipe_inicial[2];
	if (pipe(pipe_inicial) < 0) {
		error_pipe();
		return 1;
	}

	int pid = fork();

	if (pid == 0) {
		cadena_pipes(pipe_inicial);

	} else if (pid > 0) {
		close(pipe_inicial[0]);

		int n = atoi(argv[1]);
		for (int i = 2; i <= n; i++) {
			if (write(pipe_inicial[1], &i, sizeof(i)) < 0) {
				error_escritura(i, pipe_inicial[1]);
			}
		}

		close(pipe_inicial[1]);
		wait((int *) 0);

	} else {
		error_creando_proceso(getpid());
		return 1;
	}

	return 0;
}
