#include "Estoque.h"

bool Estoque::cadastrarProduto(const Produto& produto) {
    if (!validarProduto(produto)) {
        return false;
    }

    for (const Produto& produtoCadastrado : produtos) {
        if (produtoCadastrado.codigo == produto.codigo) {
            return false;
        }
    }

    produtos.push_back(produto);
    return true;
}

const Produto* Estoque::buscarProdutoPorCodigo(const std::string& codigo) const {
    for (const Produto& produto : produtos) {
        if (produto.codigo == codigo) {
            return &produto;
        }
    }

    return nullptr;
}
