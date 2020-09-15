#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <limits.h>

char *
remove_leading_chars (char *string, const char from_char)
{
    string = strrchr(string, from_char);
    string += 1;
    return string;
}

char *
remove_trailing_chars (char *string, const char from_char)
{
    char *initial_ptr = string;
    string = strrchr(string, from_char);
    *string = '\0';
    return initial_ptr;
}

char *
remove_substring (char *string, const char *substring)
{
    char *match = string;
    size_t sublength = strlen(substring);
    while ((match = strstr(match, substring)))
    {
	*match = '\0';
	strcat(string, match + sublength);
    }
    return string;
}

char *
vector_strcat (const char *string1, ...)
{
    va_list arg_list;
    const char *arg = string1;
    int list_length = strlen(arg) + 1;
    char *cated_string = malloc(list_length);
    if (!cated_string) {
	perror("Error allocating memory");
	return NULL;
    }
    strcpy(cated_string, arg);
    va_start(arg_list, string1);
	while ((arg = va_arg(arg_list, char*)) != NULL) {
	    list_length += strlen(arg);
	    cated_string = realloc(cated_string, list_length);
	    if (!cated_string) {
		perror("Error reallocating memory");
		return NULL;
	    }
	    strcat(cated_string, arg);
	}
    va_end(arg_list);
    return cated_string;
}

int
recursive_mkdir (char *path, unsigned int mode)
{
    char *buffer = malloc(strlen(path) + 1);
    if (!buffer) {
	perror("Error allocating memory");
	return -1;
    }
    strcpy(buffer, path);
    char *position = NULL;
    for (position = buffer + 1; *position; position++)
    {
	if (*position == '/') {
	    *position = '\0';
	    if (mkdir(buffer, mode) && errno != EEXIST) {
		perror("Error making directory");
		return -1;
	    }
	    *position = '/';
	}
    }
    if (mkdir(buffer, mode) && errno != EEXIST) {
	perror("Error making directory");
	return -1;
    }
    free(buffer);
    return 0;
}

int
recursive_rmdir(const char *path)
{
    DIR *directory = opendir(path);
    if (!directory) {
	perror("Error opening directory");
	return -1;
    }
    size_t path_length = strlen(path);
    struct dirent *item;
    int return1 = 0;
    while (!return1 && (item = readdir(directory))) {
	if (!strcmp(item->d_name, ".") || !strcmp(item->d_name, ".."))
	    continue;
	int return2 = -1;
	size_t total_length = path_length + strlen(item->d_name) + 2;
	char *buffer = malloc(total_length);
	if (!buffer) {
	    perror("Error allocating memory");
	    return -1;
	}
	struct stat stat_buffer;
	sprintf(buffer, "%s/%s", path, item->d_name);
	if (!stat(buffer, &stat_buffer)) {
	    if (S_ISDIR(stat_buffer.st_mode))
		return2 = recursive_rmdir(buffer);
	    else
		return2 = unlink(buffer);
	}
	free(buffer);
	return1 = return2;
    }
    closedir(directory);
    if (!return1)
	return1 = rmdir(path);
    return return1;
}

int
copy_file (const char *source, const char *destination)
{
    FILE *source_file = fopen(source, "rb");
    if (!source_file) {
	perror("Error opening source file");
	return -1;
    }
    FILE *destination_file = fopen(destination, "wb");
    if (!destination_file) {
	perror("Error opening destination file");
	return -1;
    }
    fseek(source_file, 0, SEEK_END);
    size_t buffer_size = ftell(source_file);
    fseek(source_file, 0, SEEK_SET);
    char *buffer = malloc(buffer_size);
    if (!buffer) {
	perror("Error allocating memory");
	return -1;
    }
    fread(buffer, 1, buffer_size, source_file);
    fwrite(buffer, 1, buffer_size, destination_file);
    free(buffer);
    fclose(source_file);
    fclose(destination_file);
    return 0;
}

int
redirect_output (char *argv[], const char *file)
{
    pid_t pid = fork();
    if (pid == -1) {
	perror("Error forking main process");
	return -1;
    } else if (pid == 0) {
	int fd = open(file, O_WRONLY | O_CREAT, 0644);
	if (!fd) {
	    perror("Error opening output file");
	    return -1;
	}
	if (!dup2(fd, STDOUT_FILENO)) {
	    perror("Error changing file descriptor");
	    return -1;
	}
	if (execvp(argv[0], argv) == -1) {
	    perror("Error executing redirected command");
	    return -1;
	}
	close(fd);
    }
    return 0;
}
