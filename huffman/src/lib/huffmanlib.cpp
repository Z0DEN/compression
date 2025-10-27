#include "../../include/huffmanlib.hpp"

using namespace std;

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

std::array<int, 2> getFileInfo(fstream &f) { // файл должен быть открыт на чтение
    unsigned char byte, result;
    f.read(reinterpret_cast<char *>(&byte), 1);
    return {(byte >> 4), (byte & 0x0F)};
}
