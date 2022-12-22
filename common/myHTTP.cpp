#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <vector>
#include <cstring>
#include "common.h"
#include <iostream>
class myHTTP{
    // [request]_or_[response] 
    // after or is meant if parsing a valid response msg
    // before or is meant if parsing a request msg
    public:
    std::string command_or_httpversion; //the command (GET OR POST) if parsing a request, but it's httpversion incase of response ..
    std::string argument_or_code;
    std::string httpversion_or_codemsg;

    std::vector<std::string> headers_lines;
    int content_length = -1;
    char* body_start;
    int body_size;

    bool parseHTTP(char* http_msg, int msgsize){
        int buffer_offset = 0;
        std::string header_line = "";
        while((buffer_offset < msgsize) && !(http_msg[buffer_offset]=='\r' && (http_msg[buffer_offset+2]=='\r'))){
            char current_char = http_msg[buffer_offset++];
            if (current_char == '\r') {
                buffer_offset++;//skip '\n'
                headers_lines.push_back(header_line);
                if(content_length == -1){
                    std::vector<std::string> header_segements = get_words(header_line);
                    if(header_segements.at(0)=="Content-Length:"){
                        content_length = std::stoi(header_segements.at(1));
                    }
                }
                header_line = "";
                continue;
            }
            header_line += current_char;
        }
        if(header_line.size()>0){
            headers_lines.push_back(header_line);
        }
        if(content_length == -1){
            std::vector<std::string> header_segements = get_words(header_line);
            if(header_segements.at(0)=="Content-Length:"){
                content_length = std::stoi(header_segements.at(1));
            }
        }
        buffer_offset+=4;
        body_start = http_msg + buffer_offset;
        body_size = msgsize - buffer_offset;
        if(headers_lines.empty()||headers_lines.at(0).size() < 3){
            return false;
        }
        std::vector<std::string> cmd_words = get_words(headers_lines.at(0));
        command_or_httpversion = cmd_words.at(0);
        argument_or_code = cmd_words.at(1);
        httpversion_or_codemsg = cmd_words.at(2);

        return true;
    }
};