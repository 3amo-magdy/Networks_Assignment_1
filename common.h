//
// Created by amr on 12/20/22.
//

#ifndef NETWORKS_ASSIGNMENT_1_COMMON_H
#define NETWORKS_ASSIGNMENT_1_COMMON_H
#include <mutex>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <vector>
#include <sys/stat.h>

#define CHUNK_SIZE 1024

std::vector<std::string> get_words(std::string line){
    std::vector<std::string> words;
    std::string word;
    for (char character : line){
        if (character == ' '){
            if (word.length()!=0) {
                words.push_back(word);
                word = "";
            }
        }
        else{
            word += character;
        }
    }
    if (word.length()!=0)
        words.push_back(word);
    return words;
}

static void send_onto_socket(std::vector<std::string> messages, int fd)
{
    char *chunk = (char *)(malloc(CHUNK_SIZE));
    int used_bytes = 0;
    for (std::string msg : messages)
    {
        strncpy(chunk + used_bytes, msg.c_str(), msg.size());
        used_bytes += msg.size();
    }
    write(fd, chunk, used_bytes);
    free(chunk);
}


long long GetFileSize(std::string filename)
{
    struct stat stat_buf;
    int rc = stat(filename.c_str(), &stat_buf);
    return rc == 0 ? stat_buf.st_size : -1;
}


long long FdGetFileSize(int fd)
{
    struct stat stat_buf;
    int rc = fstat(fd, &stat_buf);
    return rc == 0 ? stat_buf.st_size : -1;
}


#endif //NETWORKS_ASSIGNMENT_1_COMMON_H
