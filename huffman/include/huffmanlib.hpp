#pragma once
#include <cstddef>
#include <fstream>
#include <iostream>
#include <string>
using namespace std;

inline void rewriteByte(fstream &file, size_t position, unsigned char newByte) { // файл должен быть открыт на запись
    file.seekp(position, ios::beg);
    file.write(reinterpret_cast<char *>(&newByte), 1);
}

template <typename T> class Bitset {
    fstream          &file;
    int               length;
    unsigned long int set;

    void write() {
        while (length >= 8) {
            unsigned char byte = static_cast<unsigned char>(set >> (length -= 8));
            file.put(byte);
        }
    }

  public:
    Bitset(fstream &F) : file(F), length(8), set(0) {
        if (sizeof(T) > 8) {
            throw std::runtime_error("Неподходящий тип");
        }
    };

    ~Bitset() {
        unsigned char indent = (8 - length % 8) % 8;
        unsigned char blockSize = static_cast<unsigned char>(sizeof(T)) << 4;
        unsigned char byte = blockSize | indent;
        for (int i = 0; i < (int)indent; i++) {
            (*this) += false;
        }
        rewriteByte(file, 0, byte);
        file.flush();
    }

    // добавляет в строку бинарный код 1 или 0
    Bitset<T> &operator+=(const bool code) {
        set = (set << 1) | code;
        length++;
        write();
        return *this;
    }

    // добавляет в строку бинарный код символа типа [T]
    Bitset<T> &operator+=(const T &ch) {
        set = (set << (sizeof(T) * 8)) | static_cast<unsigned long int>(ch);
        length += sizeof(T) * 8;
        write();
        return *this;
    }

    friend ostream &operator<<(ostream &out, const Bitset &obj) {
        for (int i = obj.length - 1; i >= 0; i--) {
            out << (bool)(obj.set >> i & 1);
        }
        out << ", length = " << obj.length << "sizeof(T)" << sizeof(T) << '\n';
        return out;
    }

    Bitset(const Bitset &) = delete;
    Bitset &operator=(const Bitset &) = delete;
};

fstream FileWriter(const std::string &);
fstream FileReader(const std::string &);
int     getBlockSize(fstream &);
