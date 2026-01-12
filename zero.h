#ifndef __ZERO_PUBLIC_H__
#define __ZERO_PUBLIC_H__

#include <fcntl.h>   
#include <unistd.h>   
#include <sys/stat.h> 
#include <errno.h>    
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
    TOKEN_UNKNOWN = -1,
    TOKEN_NUM,
    TOKEN_ADD,
    TOKEN_MINUS,
    TOKEN_DIV,
    TOKEN_MULTI
}TOKEN_TYPE;

struct token {
    TOKEN_TYPE type;
    union {
        int int_value;
    }value;
};

struct source {
    const char * content;
    unsigned int size;
};

struct source * read_source(const char * file_path);
void free_source(struct source * source_file);

#ifdef ZERO_IMPLEMENTATION

struct source * read_source(const char * file_path){
    if (file_path == NULL || strlen(file_path) == 0) {
        return NULL;
    }

    int fd = open(file_path, O_RDONLY | O_CLOEXEC);
    if (fd == -1) {
        return NULL;
    }

    off_t file_size = lseek(fd, 0, SEEK_END); 
    if (file_size == -1) {
        close(fd); 
        return NULL;
    }
    lseek(fd, 0, SEEK_SET); 

    char* file_content = (char*)malloc(file_size + 1);
    if (file_content == NULL) {
        close(fd);
        return NULL;
    }

    ssize_t read_bytes = read(fd, file_content, file_size);
    if (read_bytes == -1) {
        free(file_content); 
        close(fd);
        return NULL;
    }
    file_content[read_bytes] = '\0';
    close(fd);

    struct source* src = (struct source*)malloc(sizeof(struct source));
    src->content = file_content;
    src->size = read_bytes;
    return src;
}

void free_source(struct source * source_file){
    if(source_file->content != NULL){
        free(source_file->content);
    }
    source_file->size = 0;
    free(source_file);
}

#endif
#endif
