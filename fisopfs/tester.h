#ifndef TESTER_H_
#define TESTER_H_
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <fcntl.h>
#include <dirent.h>

#define DEFAULT "\x1b[0m"
#define WHITE "\x1b[37;1m"
#define GREEN "\x1b[32;1m"
#define RED "\x1b[31;1m"
#define PURPLE "\x1b[35;1m"

#define PASS "👍👍👍"
#define FAIL "💀💀💀"

int cant_pruebas = 0;
int cant_errores = 0;

void
assert(bool valido, const char *name_test)
{
	cant_pruebas++;
	if (valido) {
		printf(GREEN "> %s ", name_test);
		printf(PASS " ");
	} else {
		printf(RED "> %s ", name_test);
		cant_errores += 1;
		printf(FAIL " ");
	}
	printf("\n");
}
void
head(const char *head_name)
{
	printf(PURPLE "\n    %s\n", head_name);
	for (int i = 0; i < strlen(head_name) + 10; i++)
		printf(">");
	printf("\n");
}
void
end_tests()
{
	printf(WHITE "\n-------\n");
	printf(WHITE "Pruebas corridas: %i, errores: %i \n",
	       cant_pruebas,
	       cant_errores);
	printf("%s\n",
	       cant_errores == 0 ? GREEN "Pruebas pasadas"
	                         : RED "Fallaron las pruebas");
	printf(DEFAULT);
}
#endif  // TESTER_H_