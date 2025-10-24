#include "../include/huffmanlib.hpp"
#include <fstream>
#include <iostream>
#include <pthread.h>
#include <string>
#include <unistd.h>
using namespace std;

template <typename T> void decoder(fstream &);
template <typename T> void encoder(fstream &);

int main(int argc, char *argv[]) {
    bool        DECODE = false;
    std::string FILENAME, TYPENAME;

    int opt;
    while ((opt = getopt(argc, argv, "df:t:h")) != -1) {
        switch (opt) {
        case 'd':
            DECODE = true;
            break;
        case 'f':
            FILENAME = optarg;
            break;
        case 't':
            TYPENAME = optarg;
            break;
        case 'h':
            cout << "Использование: program [-d] [-o файл] [-t тип данных]\n";
            return 0;
        default:
            cerr << "Неизвестная опция\n";
            return 1;
        }
    }

    if (FILENAME.empty()) {
        cout << "Недостаточно параметров. Необходимо указать входной/выходной файл\n" << "-h для вызова справки\n";
        return 1;
    }

    if (DECODE) {
        fstream f = FileReader(FILENAME);
        int     bs = getBlockSize(f);
        cout << bs << '\n';
        switch (bs) {
        case 1:
            decoder<char>(f);
            break;
        case 2:
            decoder<char16_t>(f);
            break;
        case 4:
            decoder<char32_t>(f);
            break;

        default:
            throw std::runtime_error("Неверный размер блока");
        }
    } else {
        if (TYPENAME.empty()) {
            cout << "Недостаточно параметров. Необходимо указать тип данных\n" << "-h для вызова справки\n";
            return 1;
        }
        fstream f = FileWriter(FILENAME);
        if (TYPENAME == "char") {
            encoder<char>(f);
        } else if (TYPENAME == "char16") {
            encoder<char16_t>(f);
        } else if (TYPENAME == "char32") {
            encoder<char32_t>(f);
        } else {
            throw std::runtime_error("Неверный тип");
        }

        f.close();
    }
    return 0;
}

template <typename T> void decoder(fstream &f) {
    Bitset<T> bset(f);
}

template <typename T> void encoder(fstream &f) {
    Bitset<T> bset(f);
    // bset += static_cast<T>('d');
    bset += false;
    bset += true;
    bset += false;
    bset += static_cast<T>('s');
    bset += true;
    bset += true;
    bset += false;
    bset += true;
    bset += false;
    // bset += static_cast<T>('i');
    bset += true;
    // bset += false;
    // bset += true;
    // bset += false;
    // bset += static_cast<T>('e');
    // bset += true;
}
