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
#include <fstream>


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
/**
 * puts strings into destination in order. no terminator characters are copied.
*/
static char* strscpy(char* dest,std::vector<std::string> messages)
{   
    char* pointer = dest;
    for (std::string msg : messages)
    {
        strncpy(pointer , msg.c_str(), msg.size());
        pointer = pointer + msg.size();
    }
    return pointer;
}

/**
 * puts amount of [size] bytes of file content into destination. 
*/
// static char* fcpy(char* dest,std::ifstream ifile,int size)
// {
//     ifile.read(dest,size);
//     return dest+size;
// }

// static int fsize(std::ifstream ifile){
//     ifile.seekg(std::ios::end);
//     int file_size = ifile.tellg();
//     ifile.seekg(std::ios::beg);
// }


// long long GetFileSize(std::string filename)
// {
//     struct stat stat_buf;
//     int rc = stat(filename.c_str(), &stat_buf);
//     return rc == 0 ? stat_buf.st_size : -1;
// }


// long long FdGetFileSize(int fd)
// {
//     struct stat stat_buf;
//     int rc = fstat(fd, &stat_buf);
//     return rc == 0 ? stat_buf.st_size : -1;
// }


#endif //NETWORKS_ASSIGNMENT_1_COMMON_H
