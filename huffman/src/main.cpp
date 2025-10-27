#include "../include/huffmanlib.hpp"
#include <map>
#include <pthread.h>
#include <queue>
#include <string>
#include <unistd.h>
using namespace std;

template <typename T> void decoder(fstream &, int);
template <typename T> void encoder(fstream &);

template <typename T> struct Node {
    T     symbol;
    int   freq;
    Node *left;
    Node *right;

    Node(T s, int f) : symbol(s), freq(f), left(nullptr), right(nullptr) {};
    Node(int f, Node *l, Node *r) : symbol(0), freq(f), left(l), right(r) {};
};

template <typename T> struct Compare {
    bool operator()(Node<T> *a, Node<T> *b) {
        return a->freq > b->freq;
    }
};

static std::string FILENAME, TYPENAME;

int main(int argc, char *argv[]) {
    bool DECODE = false;
    int  opt;
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
        auto [blocksize, indent] = getFileInfo(f);
        switch (blocksize) {
        case 1:
            decoder<char>(f, indent);
            break;
        case 2:
            decoder<char16_t>(f, indent);
            break;
        case 4:
            decoder<char32_t>(f, indent);
            break;

        default:
            throw std::runtime_error("Неверный размер блока");
        }
    } else {
        if (TYPENAME.empty()) {
            cout << "Недостаточно параметров. Необходимо указать тип данных\n" << "-h для вызова справки\n";
            return 1;
        }
        fstream f = FileWriter(FILENAME + ".bin");
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

template <typename T>
std::priority_queue<Node<T> *, std::vector<Node<T> *>, Compare<T>> buildPriorityQueue(fstream &file) {
    map<T, int> freq;
    T           c;
    while (file.read(reinterpret_cast<char *>(&c), sizeof(T))) {
        freq[c]++;
    }
    file.clear();
    file.seekg(0, std::ios::beg);

    std::priority_queue<Node<T> *, std::vector<Node<T> *>, Compare<T>> pq;

    for (auto &[symbol, freq] : freq) {
        pq.push(new Node(symbol, freq));
    }

    return pq;
}

template <typename T> void decoder(fstream &f, int indent) {
    fstream       DFile = FileWriter(FILENAME + ".txt");
    ReadBitset<T> bset(f, indent);
}

template <typename T> void encoder(fstream &f) {
    fstream        suorceFile = FileReader(FILENAME);
    WriteBitset<T> bset(f);
    auto           pq = buildPriorityQueue<T>(f);
    while (pq.size() > 1) {
        Node<T> *a = pq.top();
        pq.pop();
        Node<T> *b = pq.top();
        pq.pop();
        Node<T> *parent = new Node(a->freq + b->freq, a->symbol == 0 ? b : a, b->symbol == 0 ? b : a);
        pq.push(parent);
    }
    Node<T> *root = pq.top();
}
