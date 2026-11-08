#include "Estoque.h"

#include <limits>

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

const std::vector<Produto>& Estoque::obterProdutos() const {
    return produtos;
}


ResultadoMovimentacao Estoque::registrarEntrada(const std::string& codigo, int quantidade) {
    for (Produto& produto : produtos) {
        if (produto.codigo == codigo) {
            if (quantidade <= 0) {
                return ResultadoMovimentacao::quantidadeInvalida;
            }

            const int limite = std::numeric_limits<int>::max();
            if (produto.quantidade > limite - quantidade) {
                return ResultadoMovimentacao::limiteEstoqueExcedido;
            }

            produto.quantidade += quantidade;
            return ResultadoMovimentacao::sucesso;
        }
    }

    return ResultadoMovimentacao::produtoNaoEncontrado;
}

ResultadoMovimentacao Estoque::registrarSaida(const std::string& codigo, int quantidade) {
    for (Produto& produto : produtos) {
        if (produto.codigo == codigo) {
            if (quantidade <= 0) {
                return ResultadoMovimentacao::quantidadeInvalida;
            }

            if (quantidade > produto.quantidade) {
                return ResultadoMovimentacao::estoqueInsuficiente;
            }

            produto.quantidade -= quantidade;
            return ResultadoMovimentacao::sucesso;
        }
    }

    return ResultadoMovimentacao::produtoNaoEncontrado;
}
