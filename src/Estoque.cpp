#include "Estoque.h"

#include <limits>
#include <algorithm>
#include <utility>

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

ResultadoProduto Estoque::editarProduto(const std::string& codigoAtual,
                                         const std::string& novoCodigo,
                                         const std::string& novoNome,
                                         long long novoPrecoCentavos,
                                         int novaQuantidadeMinima) {
    auto encontrado = std::find_if(produtos.begin(), produtos.end(),
        [&](const Produto& produto) { return produto.codigo == codigoAtual; });
    if (encontrado == produtos.end()) {
        return ResultadoProduto::produtoNaoEncontrado;
    }

    Produto atualizado = *encontrado;
    atualizado.codigo = novoCodigo;
    atualizado.nome = novoNome;
    atualizado.precoCentavos = novoPrecoCentavos;
    atualizado.quantidadeMinima = novaQuantidadeMinima;
    if (!validarProduto(atualizado)) {
        return ResultadoProduto::dadosInvalidos;
    }

    for (const Produto& produto : produtos) {
        if (&produto != &*encontrado && produto.codigo == novoCodigo) {
            return ResultadoProduto::codigoDuplicado;
        }
    }

    *encontrado = std::move(atualizado);
    return ResultadoProduto::sucesso;
}

ResultadoProduto Estoque::excluirProduto(const std::string& codigo) {
    const auto encontrado = std::find_if(produtos.begin(), produtos.end(),
        [&](const Produto& produto) { return produto.codigo == codigo; });
    if (encontrado == produtos.end()) {
        return ResultadoProduto::produtoNaoEncontrado;
    }
    produtos.erase(encontrado);
    return ResultadoProduto::sucesso;
}

ResultadoProduto Estoque::definirQuantidadeMinima(const std::string& codigo,
                                                  int quantidadeMinima) {
    if (quantidadeMinima < 0) {
        return ResultadoProduto::quantidadeInvalida;
    }
    for (Produto& produto : produtos) {
        if (produto.codigo == codigo) {
            produto.quantidadeMinima = quantidadeMinima;
            return ResultadoProduto::sucesso;
        }
    }
    return ResultadoProduto::produtoNaoEncontrado;
}
