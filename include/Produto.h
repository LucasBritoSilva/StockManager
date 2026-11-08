#ifndef PRODUTO_H
#define PRODUTO_H

#include <string>

struct Produto {
    std::string codigo;
    std::string nome;
    long long precoCentavos;
    int quantidade;
};

#endif
