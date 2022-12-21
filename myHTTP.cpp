#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <vector>
#include <cstring>
#include "common.h"
#include <iostream>
class myHTTP{
    public:
    std::string command;
    std::string argument;
    std::string http_version;
    std::vector<std::string> headers_lines;
    int content_length = -1;
    char* body_start;
    int body_size;

    void parseHTTP(char* http_msg, int msgsize){
        int buffer_offset = 0;
        std::string header_line = "";
        while(!(http_msg[buffer_offset]=='\r' && (http_msg[buffer_offset+2]=='\r')) && (buffer_offset < msgsize)){
            char next_char = http_msg[buffer_offset++];
            if (next_char == '\r') {
                buffer_offset++;//skip '\n'
                headers_lines.push_back(header_line);
                if(content_length == -1){
                    std::vector<std::string> header_segements = get_words(header_line);
                    if(header_segements.at(0)=="content-Length:"){
                        content_length = std::stoi(header_segements.at(1));
                    }
                }
                header_line = "";
                continue;
            }
            header_line += next_char;
        }
        buffer_offset+=3;
        body_start = http_msg + buffer_offset;
        body_size = msgsize - buffer_offset;
        std::vector<std::string> cmd_words = get_words(headers_lines.at(0));
        command = cmd_words.at(0);
        argument = cmd_words.at(1);
        http_version = cmd_words.at(2);
        return;
    }
};