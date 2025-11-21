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

// Вспомогательный класс для побайтовой записи в файл произвольной последовательности битов.
// Стоит заметить что Set содержащий эту последовательность заполняется справа на лево путем сдвига влево и побитового сложения.
// Для оптимизации занимаемой памяти при большом файле "сброс" битов в файл выполняется после каждого вызова метода += и при условии целого числа байт.
// Запись выполняется в обратном порядке - сдвиг вправо на Length - 8 ( -1 для бита) чтобы получить следующий байт (бит) и уменьшение Length -= 8 (-1)
// Да, Set никогда, кроме создания объекта не очищается, актуальная последовательность определяется длиной Length начиная с конца(правой стороны) Set
class WriteBitset {
    fstream     &File;
    int          Length;
    unsigned int Set;

    // По мере заполнения Set записывает в файл целый байт из Set
    void write() {
        while (Length >= 8) {
            unsigned char byte = static_cast<unsigned char>(Set >> (Length -= 8));
            File.put(byte);
        }
    }

  public:
    WriteBitset(fstream &F) : File(F), Length(8), Set(0) {}; // Length(8) для служебного байта с отступом

    ~WriteBitset() {
        unsigned int indent = (8 - Length % 8) % 8;
        for (int i = 0; i < indent; i++) {
            (*this) += false; // перегрузка += для bool
        }
        rewriteByte(
            File, 0,
            indent); // в конце записи определяем сколько нехватает бит до целого байта, дополняем нулями и перезаписываем нулевой байт
    }

    // добавляет в строку бинарный код 1 или 0
    WriteBitset &operator+=(const bool code) {
        Set = (Set << 1) | code;
        Length++;
        write();
        return *this;
    }

    // добавляет в строку бинарный код символа char
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

    // мой класс нельзя копировать или присваивать т.к. есть поле File которое работает с fstream
    WriteBitset(const WriteBitset &) = delete;
    WriteBitset &operator=(const WriteBitset &) = delete;
};

// Аналогичный классу WriteBitset класс для чтения последовательности битов.
// В данном случае Set может содержать 8 байт которые заполняются справа на лево по мере чтения
class ReadBitset {
    fstream          &File;
    int               Length, Indent;
    unsigned long int Set;

    void read() {
        if (File.eof()) { // уберем лишние нули сдвинув Set на Indent и уменьшив длину
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
        File.seekp(1, ios::beg); // Пропускаем "служебный" байт
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
    unsigned char indent;
    f.read(reinterpret_cast<char *>(&indent), 1);
    return (int)indent;
}
