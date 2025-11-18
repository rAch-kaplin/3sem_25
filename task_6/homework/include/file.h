#ifndef FILE_H
#define FILE_H

#include "common.h"

bool is_text_file(const char *filename);
size_t search_files_in_dir(const char *path, char ***files);

#endif //FILE_H


