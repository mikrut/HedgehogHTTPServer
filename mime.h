#ifndef MIME_H
#define MIME_H

void ext_to_mime(const char* ext, char* result);
void get_mime_for_path(char* file_path, char* mime_buf);

#endif
