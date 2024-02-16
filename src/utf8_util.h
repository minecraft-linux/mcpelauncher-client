#pragma once

#include <cstddef>

class UTF8Util {
public:
    static int getCharByteSize(char c) {
        if((c & 0b11110000) == 0b11100000)
            return 3;
        else if((c & 0b11100000) == 0b11000000)
            return 2;
        return 1;
    }

    static size_t getLength(const char* str, size_t len) {
        size_t ret = 0;
        for(size_t i = 0; i < len;) {
            char c = str[i];
            i += getCharByteSize(c);
            ret++;
        }
        return ret;
    }

    static size_t getBytePosFromUTF(const char* str, size_t len, size_t utflen) {
        size_t ret = 0;
        for(size_t i = 0; i < len;) {
            char c = str[i];
            i += getCharByteSize(c);
            ret++;
            if(utflen == ret) {
                return i;
            }
        }
        return ret;
    }
};
