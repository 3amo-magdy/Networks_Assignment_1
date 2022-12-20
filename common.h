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


#endif //NETWORKS_ASSIGNMENT_1_COMMON_H
