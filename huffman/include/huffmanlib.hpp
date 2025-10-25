#pragma once
#include <array>
#include <cstddef>
#include <fstream>
#include <iostream>
#include <string>
using namespace std;

inline void rewriteByte(fstream &file, size_t position, unsigned char newByte) { // файл должен быть открыт на запись
    file.seekp(position, ios::beg);
    file.write(reinterpret_cast<char *>(&newByte), 1);
}

template <typename T> class WriteBitset {
    fstream          &File;
    int               Length;
    unsigned long int Set;

    void write() {
        while (Length >= 8) {
            unsigned char byte = static_cast<unsigned char>(Set >> (Length -= 8));
            File.put(byte);
        }
    }

  public:
    WriteBitset(fstream &F) : File(F), Length(8), Set(0) {
        if (sizeof(T) > 8) {
            throw std::runtime_error("Неподходящий тип");
        }
    };

    ~WriteBitset() {
        unsigned char indnt = (8 - Length % 8) % 8;
        unsigned char blockSize = static_cast<unsigned char>(sizeof(T)) << 4;
        unsigned char byte = blockSize | indnt;
        for (int i = 0; i < (int)indnt; i++) {
            (*this) += false;
        }
        rewriteByte(File, 0, byte);
        File.flush();
    }

    // добавляет в строку бинарный код 1 или 0
    WriteBitset<T> &operator+=(const bool code) {
        Set = (Set << 1) | code;
        Length++;
        write();
        return *this;
    }

    // добавляет в строку бинарный код символа типа [T]
    WriteBitset<T> &operator+=(const T &ch) {
        Set = (Set << (sizeof(T) * 8)) | (ch);
        Length += sizeof(T) * 8;
        write();
        return *this;
    }

    friend ostream &operator<<(ostream &out, const WriteBitset &obj) {
        for (int i = obj.Length - 1; i >= 0; i--) {
            out << (bool)(obj.Set >> i & 1);
        }
        out << ", length = " << obj.Length << ", sizeof(T) = " << sizeof(T) << '\n';
        return out;
    }

    WriteBitset(const WriteBitset &) = delete;
    WriteBitset &operator=(const WriteBitset &) = delete;
};

template <typename T> class ReadBitset {
    fstream          &File;
    int               Length, Indent;
    unsigned long int Set;

    void read() {
        char byte;
        while (File.read(&byte, 1) && Length <= 56) {
            Set = (Set << 8) | byte;
            Length += 8;
        }
        if (File.eof() && Indent) {
            Set >> Indent;
            Indent = 0;
        }
    }

  public:
    ReadBitset(fstream &F, int indent = 0) : File(F), Length(0), Set(0), Indent(indent) {
        if (sizeof(T) > 8) {
            throw std::runtime_error("Неподходящий тип");
        }
    }

    T readChar() {
        read();
        int shift = Length - sizeof(T) * 8;
        Length -= sizeof(T) * 8;
        return static_cast<T>(Set >> shift);
    }

    bool readBit() {
        read();
        Length -= 1;
        return (Set >> Length) & 1;
    }

    friend ostream &operator<<(ostream &out, const ReadBitset &obj) {
        for (int i = obj.Length - 1; i >= 0; i--) {
            out << (bool)(obj.Set >> i & 1);
        }
        out << ", length = " << obj.Length << ", sizeof(T) =" << sizeof(T) << '\n';
        return out;
    }

    ReadBitset(const ReadBitset &) = delete;
    ReadBitset &operator=(const ReadBitset &) = delete;
};

fstream            FileWriter(const string &);
fstream            FileReader(const string &);
std::array<int, 2> getFileInfo(fstream &);
