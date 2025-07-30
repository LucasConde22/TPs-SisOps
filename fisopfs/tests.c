#define _GNU_SOURCE
#include "fs.h"
#include "tester.h"

#define MOUNT_POINT "prueba"
#define TEST_ROOT "prueba/test_env"
#define DIR_PERM 0755
#define PERM_ALL 0777
#define PERM_PRIVATE 0600
#define MODE_0644 (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)

void
test_fisopfs_mkdir_and_rmdir()
{
	head("Tests mkdir");
	char path[MAX_PATH_NAME];
	snprintf(path, sizeof(path), "%s/dir_prueba", TEST_ROOT);
	int res = mkdir(path, DIR_PERM);
	assert(res == 0, "mkdir crea un directorio con éxito");
	DIR *d = opendir(path);
	assert(d != NULL, "Directorio accesible");
	res = mkdir(path, DIR_PERM);
	assert(res != 0, "mkdir no puede crear un directorio que ya existe");
	char sub_path[MAX_PATH_NAME];
	snprintf(sub_path, sizeof(sub_path), "%s/sd", path);
	res = mkdir(sub_path, DIR_PERM);
	assert(res == 0, "mkdir puede crear un directorio dentro de otro");

	// RMDIR

	head("Tests rmdir");
	char path_file[MAX_PATH_NAME];
	snprintf(path_file, sizeof(path_file), "%s/archivo.txt", TEST_ROOT);
	open(path_file, O_CREAT | O_WRONLY, 'w');
	res = rmdir(path_file);
	assert(res != 0,
	       "rmdir no puede eliminar un directorio que no es un directorio");
	res = rmdir(path);
	assert(res != 0,
	       "rmdir no puede eliminar un directorio que no esta vacio");
	res = rmdir(sub_path);
	assert(res == 0, "rmdir elimina el sub_path creado");
	res = rmdir(path);
	assert(res == 0, "rmdir elimina el path creado");
	res = rmdir(path);
	assert(res != 0, "rmdir ya elimino el path");
	res = rmdir("No existe");
	assert(res != 0, "rmdir falla al eliminar un directorio que no existe");
}

void
test_fisopfs_getattr()
{
	head("Tests getattr");

	char file_path[MAX_PATH_NAME];
	snprintf(file_path, sizeof(file_path), "%s/archivo_getattr.txt", TEST_ROOT);
	time_t antes = time(NULL);
	int fd = open(file_path,
	              O_CREAT | O_WRONLY | O_EXCL,
	              S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	close(fd);
	struct stat st;
	int res = stat(file_path, &st);
	time_t despues = time(NULL);
	assert(res == 0, "stat obtiene atributos de archivo existente");
	assert(S_ISREG(st.st_mode), "stat reconoce archivo regular");
	assert(st.st_size == 0, "archivo nuevo tiene tamaño 0");
	assert(st.st_ctime >= antes && st.st_ctime <= despues,
	       "ctime archivo en rango esperado");
	assert(st.st_mtime >= antes && st.st_mtime <= despues,
	       "mtime archivo en rango esperado");
	assert(st.st_atime >= antes && st.st_atime <= despues,
	       "atime archivo en rango esperado");
	char dir_path[MAX_PATH_NAME];
	snprintf(dir_path, sizeof(dir_path), "%s/dir_getattr", TEST_ROOT);
	antes = time(NULL);
	res = mkdir(dir_path, DIR_PERM);
	despues = time(NULL);
	assert(res == 0, "mkdir crea el directorio para getattr");
	res = stat(dir_path, &st);
	assert(res == 0, "stat obtiene atributos de directorio existente");
	assert(S_ISDIR(st.st_mode), "stat reconoce directorio");
	assert(st.st_ctime >= antes && st.st_ctime <= despues,
	       "ctime directorio en rango esperado");
	assert(st.st_mtime >= antes && st.st_mtime <= despues,
	       "mtime directorio en rango esperado");
	assert(st.st_atime >= antes && st.st_atime <= despues,
	       "atime directorio en rango esperado");

	unlink(file_path);
	rmdir(dir_path);
}

void
test_fisopfs_create_unlink()
{
	head("Tests Create");
	char path[MAX_PATH_NAME];
	snprintf(path, sizeof(path), "%s/archivo_creado.txt", TEST_ROOT);
	int res = open(path, O_CREAT | O_WRONLY, 'w');
	assert(res >= 0, "create crea un archivo exitosamente");
	close(res);
	res = open(path, O_CREAT | O_WRONLY | O_EXCL, 'w');
	assert(res < 0, "no se puede crear un archivo que ya existe");

	head("Tests unlink");
	char path_dir[MAX_PATH_NAME];
	snprintf(path_dir, sizeof(path_dir), "%s/directorio", TEST_ROOT);
	mkdir(path_dir, 'w');
	res = unlink(path_dir);
	assert(res < 0, "no se puede borra un arhivo que es directorio");
	res = unlink(path);
	assert(res == 0, "se borra correctamente un arhivo existente");
	res = unlink(path);
	assert(res < 0, "no se puede borra un arhivo que no existe");
}


void
test_utimens()
{
	head("Tests utimens");
	char path[MAX_PATH_NAME];
	snprintf(path, sizeof(path), "%s/archivo_creado.txt", TEST_ROOT);
	struct utimbuf times;
	times.actime = 5;
	times.modtime = 15;
	int res = utime("no existe", &times);
	assert(res != 0, "utimens no actualiza un archivo inexistente");
	char path_aux[MAX_PATH_NAME];
	snprintf(path_aux, sizeof(path_aux), "%s/directoriodsadsadasooo", TEST_ROOT);
	mkdir(path_aux, 'w');
	res = utime(path, &times);
	assert(res != 0, "utimens no actualiza un archivo que es directorio");
	rmdir(path_aux);
	open(path, O_CREAT | O_WRONLY, 'w');
	res = utime(path, &times);
	assert(res == 0, "utimens actualiza tiempos correctamente");
}

void
test_fisopfs_write_and_read()
{
	head("Tests write & read");

	char path[MAX_PATH_NAME];
	snprintf(path, sizeof(path), "%s/archivo_escrito.txt", TEST_ROOT);
	int fd = open(path, O_CREAT | O_WRONLY | O_EXCL, MODE_0644);
	assert(fd >= 0, "open crea un archivo para escritura exitosamente");
	const char *content = "Contenido de prueba";
	ssize_t escrito = write(fd, content, strlen(content));
	assert(escrito == (ssize_t) strlen(content),
	       "write escribe el contenido correctamente");
	close(fd);
	fd = open(path, O_RDONLY);
	assert(fd >= 0, "open abre el archivo para lectura exitosamente");
	char buffer[64] = { 0 };
	ssize_t leido = read(fd, buffer, sizeof(buffer) - 1);
	assert(leido == (ssize_t) strlen(content),
	       "read lee la cantidad correcta de bytes");
	assert(strcmp(buffer, content) == 0, "read lee el contenido correcto");
	close(fd);
	// trunc
	char path_trunc[MAX_PATH_NAME];
	snprintf(path_trunc, sizeof(path_trunc), "%s/archivo_trunc.txt", TEST_ROOT);
	fd = open(path_trunc, O_CREAT | O_WRONLY | O_EXCL, MODE_0644);
	assert(fd >= 0, "open crea archivo para trunc exitosamente");
	write(fd, "algo", 4);
	close(fd);
	fd = open(path_trunc, O_WRONLY | O_TRUNC);
	assert(fd >= 0, "open con O_TRUNC abre el archivo exitosamente");
	close(fd);
	fd = open(path_trunc, O_RDONLY);
	char buffer_trunc[8] = { 0 };
	ssize_t leido_trunc = read(fd, buffer_trunc, sizeof(buffer_trunc));
	assert(leido_trunc == 0, "read tras O_TRUNC devuelve 0 bytes");
	close(fd);
	unlink(path_trunc);
	// append
	char path_append[MAX_PATH_NAME];
	snprintf(path_append, sizeof(path_append), "%s/archivo_append.txt", TEST_ROOT);
	fd = open(path_append, O_CREAT | O_WRONLY | O_EXCL, MODE_0644);
	assert(fd >= 0, "open crea archivo para append exitosamente");
	close(fd);

	fd = open(path_append, O_WRONLY | O_APPEND);
	assert(fd >= 0, "open con O_APPEND abre el archivo exitosamente");
	const char *parte1 = "Hola ";
	const char *parte2 = "Mundo";
	write(fd, parte1, strlen(parte1));
	write(fd, parte2, strlen(parte2));
	close(fd);

	fd = open(path_append, O_RDONLY);
	char buffer_append[32] = { 0 };
	ssize_t leido_append = read(fd, buffer_append, sizeof(buffer_append) - 1);
	assert(leido_append == (ssize_t) (strlen(parte1) + strlen(parte2)),
	       "read lee la cantidad correcta tras append");
	assert(strcmp(buffer_append, "Hola Mundo") == 0,
	       "append escribe correctamente al final");
	close(fd);
	unlink(path);
	unlink(path_append);
}

void
test_types_read()
{
	head("Tests types read");

	char path[MAX_PATH_NAME];
	snprintf(path, sizeof(path), "%s/archivo_escrito.txt", TEST_ROOT);
	int fd = open(path, O_CREAT | O_WRONLY | O_EXCL, MODE_0644);
	assert(fd >= 0, "open crea un archivo para escritura exitosamente");
	const char *content = "Contenido de prueba";
	ssize_t escrito = write(fd, content, strlen(content));

	// cat more less
}

void
test_fisopfs_readdir()
{
	head("Tests readdir");
	char dir_path[MAX_PATH_NAME];
	snprintf(dir_path, sizeof(dir_path), "%s/dir_readdir", TEST_ROOT);
	int res = mkdir(dir_path, DIR_PERM);
	char file1[MAX_PATH_NAME], file2[MAX_PATH_NAME], subdir[MAX_PATH_NAME];
	snprintf(file1, sizeof(file1), "%s/archivo1.txt", dir_path);
	snprintf(file2, sizeof(file2), "%s/archivo2.txt", dir_path);
	snprintf(subdir, sizeof(subdir), "%s/subdir", dir_path);
	int fd1 = creat(file1, MODE_0644);
	int fd2 = creat(file2, MODE_0644);
	close(fd1);
	close(fd2);
	res = mkdir(subdir, DIR_PERM);
	DIR *d = opendir(dir_path);
	struct dirent *entry;
	int tiene_archivo1 = 0, tiene_archivo2 = 0, tiene_subdir = 0;
	while ((entry = readdir(d)) != NULL) {
		if (strcmp(entry->d_name, "archivo1.txt") == 0)
			tiene_archivo1 = 1;
		if (strcmp(entry->d_name, "archivo2.txt") == 0)
			tiene_archivo2 = 1;
		if (strcmp(entry->d_name, "subdir") == 0)
			tiene_subdir = 1;
	}
	assert(tiene_archivo1, "readdir encuentra archivo1.txt");
	assert(tiene_archivo2, "readdir encuentra archivo2.txt");
	assert(tiene_subdir, "readdir encuentra subdir");
	closedir(d);
	unlink(file1);
	unlink(file2);
	rmdir(subdir);
	rmdir(dir_path);
}

/*---------------------PRUEBAS CHALLENGES---------------------*/

void
test_fisopfs_mkdir_limit()
{
	head("Tests mkdir limitado a 3 niveles");

	char p1[MAX_PATH_NAME], p2[MAX_PATH_NAME], p3[MAX_PATH_NAME],
	        p4[MAX_PATH_NAME];
	snprintf(p1, sizeof(p1), "%s/p1", TEST_ROOT);
	snprintf(p2, sizeof(p2), "%s/p1/p2", TEST_ROOT);
	snprintf(p3, sizeof(p3), "%s/p1/p2/p3", TEST_ROOT);
	snprintf(p4, sizeof(p4), "%s/p1/p2/p3/p4", TEST_ROOT);

	rmdir(p4);
	rmdir(p3);
	rmdir(p2);
	rmdir(p1);

	assert(mkdir(p1, DIR_PERM) == 0, "mkdir crea p1");
	assert(mkdir(p2, DIR_PERM) == 0, "mkdir crea p2 dentro de p1");
	assert(mkdir(p3, DIR_PERM) == 0, "mkdir crea p3 dentro de p2");
	assert(mkdir(p4, DIR_PERM) != 0,
	       "mkdir falla al intentar crear p4 por límite de profundidad");
	assert(opendir(p1) != NULL, "p1 existe");
	assert(opendir(p2) != NULL, "p2 existe");
	assert(opendir(p3) != NULL, "p3 existe");

	DIR *d = opendir(p4);
	assert(d == NULL, "p4 no fue creado");
}

void
test_fisopfs_chmod()
{
	head("Tests chmod");
	char path[MAX_PATH_NAME];
	snprintf(path, sizeof(path), "%s/archivo_chmod.txt", TEST_ROOT);
	unlink(path);
	int fd = open(path, O_CREAT | O_WRONLY | O_EXCL, MODE_0644);
	assert(fd >= 0, "open crea archivo para chmod exitosamente");
	close(fd);
	struct stat st;

	assert(stat(path, &st) == 0,
	       "stat obtiene atributos de archivo para chmod");
	assert((st.st_mode & PERM_ALL) == MODE_0644, "modo inicial es 0644");
	assert(chmod(path, PERM_PRIVATE) == 0, "chmod cambia permisos a 0600");
	assert(stat(path, &st) == 0, "stat tras chmod");
	assert((st.st_mode & PERM_ALL) == PERM_PRIVATE, "modo es 0600 tras chmod");
	assert(chmod(path, PERM_ALL) == 0, "chmod cambia permisos a 0777");
	assert(stat(path, &st) == 0, "stat tras chmod 0777");
	assert((st.st_mode & PERM_ALL) == PERM_ALL, "modo es 0777 tras chmod");
	unlink(path);
}

void
test_fisopfs_chown()
{
	head("Tests chown");
	char path[MAX_PATH_NAME];
	snprintf(path, sizeof(path), "%s/archivo_chown.txt", TEST_ROOT);
	unlink(path);
	int fd = open(path, O_CREAT | O_WRONLY | O_EXCL, MODE_0644);
	assert(fd >= 0, "open crea archivo para chown exitosamente");
	close(fd);
	struct stat st;
	assert(stat(path, &st) == 0,
	       "stat obtiene atributos de archivo para chown");
	uid_t old_uid = st.st_uid;
	gid_t old_gid = st.st_gid;
	int res = chown(path, 0, 0);
	if (res == 0) {
		assert(stat(path, &st) == 0, "stat tras chown");
		assert(st.st_uid == 0, "uid es root tras chown");
		assert(st.st_gid == 0, "gid es root tras chown");
	} else {
		assert(res != 0, "chown falla si no sos root");
	}
	if (res == 0) {
		assert(chown(path, old_uid, old_gid) == 0,
		       "chown vuelve a usuario original");
		assert(stat(path, &st) == 0, "stat tras chown revertido");
		assert(st.st_uid == old_uid, "uid original tras revertir");
		assert(st.st_gid == old_gid, "gid original tras revertir");
	}

	unlink(path);
}

int
main()
{
	fs_initialize();
	mkdir(TEST_ROOT, DIR_PERM);
	head("=== TESTS DE FISOPFS ===");
	test_fisopfs_mkdir_and_rmdir();
	test_fisopfs_getattr();
	test_fisopfs_readdir();
	test_fisopfs_create_unlink();
	test_utimens();
	test_fisopfs_write_and_read();
	head("----------------------------------");
	head("=== TESTS DESAFÍOS DE FISOPFS ===");
	test_fisopfs_mkdir_limit();
	test_fisopfs_chown();
	test_fisopfs_chmod();
	end_tests();
	return 0;
}