#ifndef PRODUTO_H
#define PRODUTO_H

#include <string>

struct Produto {
    std::string codigo;
    std::string nome;
    long long precoCentavos;
    int quantidade;
    int quantidadeMinima = 0;
};

bool validarProduto(const Produto& produto);

#endif
