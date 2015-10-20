#include <string.h>
#include <stdbool.h>

#include "mime.h"

const int ext_cmp_len = 5;

int in(const char** array, int arr_len, const char* str) {
  for (int i = 0; i < arr_len; i++)
    if (!strncmp(array[i], str, ext_cmp_len))
      return true;
  return false;
}

void ext_to_mime(const char* _ext, char* result) {
  char ext[10] = "";
  strncpy(ext, _ext, 8);
  if (!strcmp(ext, "jpg"))
    strcpy(ext, "jpeg");

  const char* images[] = {"jpeg", "png", "gif"};
  const char* texts[] = {"html", "css"};

  if (ext != NULL) {
    if (in(images, sizeof(images)/sizeof(images[0]), ext)) {
      strncpy(result, "image/", 20);
      strncat(result, ext, ext_cmp_len);
    } else if (in(texts, sizeof(texts)/sizeof(texts[0]), ext)) {
      strncpy(result, "text/", 6);
      strncat(result, ext, ext_cmp_len);
    } else if (!strncmp(ext, "swf", ext_cmp_len)) {
      const char swf_mime[] = "application/x-shockwave-flash";
      strncpy(result, swf_mime, sizeof(swf_mime));
    } else if (!strncmp(ext, "js", ext_cmp_len)){
      const char js_mime[] = "application/javascript";
      strncpy(result, js_mime, sizeof(js_mime));
    } else {
      const char plain_mime[] = "text/plain";
      strncpy(result, plain_mime, sizeof(plain_mime));
    }
  } else {
    const char plain_mime[] = "text/plain";
    strncpy(result, plain_mime, sizeof(plain_mime));
  }
}

void get_mime_for_path(char* file_path, char* mime_buf) {
  char* name = basename(file_path);
  const char* ext = strrchr(name, '.');

  if ((ext != NULL)) {
    ext_to_mime(ext+1, mime_buf);
  }
}
