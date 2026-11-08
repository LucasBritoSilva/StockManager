#ifndef ESTOQUE_H
#define ESTOQUE_H

#include <string>
#include <vector>

#include "Produto.h"

class Estoque {
private:
    std::vector<Produto> produtos;

public:
    bool cadastrarProduto(const Produto& produto);
    const Produto* buscarProdutoPorCodigo(const std::string& codigo) const;
};

#endif
