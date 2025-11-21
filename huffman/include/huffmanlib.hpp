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

class WriteBitset {
    fstream     &File;
    int          Length;
    unsigned int Set;

    void print(unsigned char byte) {
        for (int bit = 7; bit >= 0; --bit) {
            std::cout << ((byte >> bit) & 1);
        }
        std::cout << ' ';
    }

    void write() {
        while (Length >= 8) {
            unsigned char byte = static_cast<unsigned char>(Set >> (Length -= 8));
            File.put(byte);
        }
    }

  public:
    WriteBitset(fstream &F) : File(F), Length(8), Set(0) {};

    ~WriteBitset() {
        unsigned char indnt = (8 - Length % 8) % 8;
        for (int i = 0; i < (int)indnt; i++) {
            (*this) += false;
        }
        rewriteByte(File, 0, indnt);
    }

    // добавляет в строку бинарный код 1 или 0
    WriteBitset &operator+=(const bool code) {
        Set = (Set << 1) | code;
        Length++;
        write();
        return *this;
    }

    // добавляет в строку бинарный код символа типа [T]
    WriteBitset &operator+=(const unsigned char &ch) {
        Set = (Set << 8) | static_cast<unsigned int>(ch);
        Length += 8;
        write();
        return *this;
    }

    friend ostream &operator<<(ostream &out, const WriteBitset &obj) {
        for (int i = obj.Length - 1; i >= 0; i--) {
            out << (bool)(obj.Set >> i & 1);
        }
        out << ", length = " << obj.Length << '\n';
        return out;
    }

    WriteBitset(const WriteBitset &) = delete;
    WriteBitset &operator=(const WriteBitset &) = delete;
};

class ReadBitset {
    fstream          &File;
    int               Length, Indent;
    unsigned long int Set;

    void print(unsigned char byte) {
        for (int bit = 7; bit >= 0; --bit) {
            cerr << ((byte >> bit) & 1);
        }
        cerr << ' ';
    }

    void read() {
        if (File.eof()) {
            if (Indent) {
                Set = Set >> Indent;
                Length -= Indent;
                Indent = 0;
            }
        } else {
            while (Length <= 56 && !File.eof()) {
                unsigned char byte;
                if (File.read(reinterpret_cast<char *>(&byte), 1)) {
                    Set = (Set << 8) | static_cast<unsigned long int>(byte);
                    Length += 8;
                }
            }
        }
    }

  public:
    ReadBitset(fstream &F, int indent = 0) : File(F), Length(0), Set(0), Indent(indent) {
        // File.clear();
        File.seekp(1, ios::beg);
        read();
    }

    bool isEnd() {
        read();
        return !(bool)Length;
    }

    int getLength() {
        return Length;
    }

    unsigned char readChar() {
        read();
        unsigned char c = Set >> (Length -= 8);
        return c;
    }

    bool readBit() {
        read();
        Length -= 1;
        // cout << ((Set >> Length) & 1);
        return (Set >> Length) & 1;
    }

    friend ostream &operator<<(ostream &out, const ReadBitset &obj) {
        for (int bit = obj.Length - 1; bit >= 0; bit--) {
            out << ((obj.Set >> bit) & 1);
        }
        out << ", length = " << obj.Length << '\n';
        return out;
    }

    ReadBitset(const ReadBitset &) = delete;
    ReadBitset &operator=(const ReadBitset &) = delete;
};

fstream FileWriter(const std::string &filename) {
    fstream file(filename, ios::binary | ios::out);
    if (!file)
        throw runtime_error("Ошибка открытия для записи");
    return file;
}

fstream FileReader(const std::string &filename) {
    fstream file(filename, ios::binary | ios::in);
    if (!file)
        throw runtime_error("Ошибка открытия для чтения");
    return file;
}

int getFileInfo(fstream &f) { // файл должен быть открыт на чтение
    unsigned char byte;
    f.read(reinterpret_cast<char *>(&byte), 1);
    return (int)byte;
}
