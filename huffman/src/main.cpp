#include "../include/huffmanlib.hpp"
#include <map>
#include <pthread.h>
#include <queue>
#include <string>
#include <unistd.h>
#include <unordered_map>

// для читаемости
using namespace std;
using uchar = unsigned char;
using Code = std::vector<bool>;
using CodeTable = std::unordered_map<uchar, Code>;

void decoder(string &);
void encoder(string &);

struct Node {
    uchar symbol;
    int   freq;
    Node *left;
    Node *right;

    Node(uchar s, int f) : symbol(s), freq(f), left(nullptr), right(nullptr) {};
    Node(int f, Node *l, Node *r) : symbol(0), freq(f), left(l), right(r) {};
};
void DFS(Node *r, WriteBitset &bset, Code &path, CodeTable &table);

void freeTree(Node *n) {
    if (!n)
        return;
    freeTree(n->left);
    freeTree(n->right);
    delete n;
}

// компаратор для priority_queue
struct Compare {
    bool operator()(Node *a, Node *b) {
        return a->freq > b->freq;
    }
};

int main(int argc, char *argv[]) {
    std::string FILENAME;
    bool        DECODE = false;
    int         opt;

    while ((opt = getopt(argc, argv, "df:h")) != -1) {
        switch (opt) {
        case 'd':
            DECODE = true;
            break;
        case 'f':
            FILENAME = optarg;
            break;
        case 'h':
            cout << "Использование: program [-d для декодирования] [-f файл - обязательный параметр]\n";
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
        decoder(FILENAME);
    } else {
        encoder(FILENAME);
    }
    return 0;
}

// Воссоздает дерево по обходу из файла используя для чтения методы ReadBitset
Node *buildTreeFromFile(ReadBitset &bset) {
    if (bset.isEnd()) {
        throw std::runtime_error("Файл внезапно закончился");
    }
    if (bset.readBit()) {
        return new Node(bset.readChar(), 0);
    } else {
        Node *left = buildTreeFromFile(bset);
        Node *right = buildTreeFromFile(bset);
        return new Node(0, left, right);
    }
}

void decoder(string &FILENAME) {
    fstream    f = FileReader(FILENAME);              // сжатый файл (например file.txt.bin)
    fstream    DFile = FileWriter(FILENAME + ".txt"); // декодированный файл (file.txt.bin.txt)
    int        indent = getFileInfo(f);
    ReadBitset bset(f, indent);

    Node *root = buildTreeFromFile(bset);

    Node *temp = root;
    while (!bset.isEnd()) {
        if (temp->left && temp->right) {
            if (bset.readBit()) {
                temp = temp->right;
            } else {
                temp = temp->left;
            }
        } else {
            DFile.put(temp->symbol);
            temp = root;
        }
    }
}

std::priority_queue<Node *, std::vector<Node *>, Compare> buildPriorityQueue(fstream &file) {
    map<uchar, int> freq;
    uchar           nextChar;
    while (file.read(reinterpret_cast<char *>(&nextChar), 1)) {
        freq[nextChar]++;
    }
    file.clear();
    file.seekg(0, std::ios::beg);

    std::priority_queue<Node *, std::vector<Node *>, Compare> pq;

    for (auto &[symbol, f] : freq) {
        pq.push(new Node(symbol, f));
    }
    if (pq.empty())
        throw std::runtime_error("Пустой входной файл");
    return pq;
}

void encoder(string &FILENAME) {
    fstream     f = FileWriter(FILENAME + ".bin");
    fstream     sourceFile = FileReader(FILENAME);
    WriteBitset bset(f);
    auto        pq = buildPriorityQueue(sourceFile);
    while (pq.size() > 1) {
        Node *a = pq.top();
        pq.pop();
        Node *b = pq.top();
        pq.pop();
        Node *parent = new Node(a->freq + b->freq, a, b);
        pq.push(parent);
    }
    Node     *root = pq.top();
    Code      path;
    CodeTable table;

    DFS(root, bset, path, table);
    freeTree(root);

    uchar c;
    while (sourceFile.read(reinterpret_cast<char *>(&c), 1)) {
        const auto value = table.find(c);

        if (value == table.end()) {
            cout << "Нет кода для символа - " << reinterpret_cast<char *>(&c) << "\n";
            break;
        }
        for (bool bit : value->second) {
            bset += bit;
        }
    }
}

// Обход дерева в глубину
// 1. Составляет таблицу кодов
// 2. Параллельно записывает в bset обход, чтобы при декодировании построить дерево
void DFS(Node *n, WriteBitset &bset, Code &path, CodeTable &table) {
    if (!n)
        return;
    if (n->left && n->right) {
        bset += false;
    } else {
        bset += true;
        bset += n->symbol;
        if (path.empty()) // если символ всего один, присваиваем ему 0
            path.push_back(false);
        table[n->symbol] = path;
        return;
    }
    path.push_back(false);
    DFS(n->left, bset, path, table);
    path.pop_back();

    path.push_back(true);
    DFS(n->right, bset, path, table);
    path.pop_back();
}
