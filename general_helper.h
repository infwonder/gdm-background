#ifndef GENERAL_HELPER_H
#define GENERAL_HELPER_H

char *remove_leading_chars (char *string, const char from_char);

char *remove_trailing_chars (char *string, char from_char);

char *remove_substring (char *string, const char *substring);

char *vector_strcat (const char *string1, ...);

int recursive_mkdir (char *path, unsigned int mode);

int recursive_rmdir(const char *path);

int copy_file (const char *source, const char *destination);

int redirect_output (char *argv[], const char *file);

#endif
