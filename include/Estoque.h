#ifndef ESTOQUE_H
#define ESTOQUE_H

#include <string>
#include <vector>

#include "Produto.h"

enum class ResultadoMovimentacao {
    sucesso,
    produtoNaoEncontrado,
    quantidadeInvalida,
    limiteEstoqueExcedido,
    estoqueInsuficiente
};

class Estoque {
private:
    std::vector<Produto> produtos;

public:
    bool cadastrarProduto(const Produto& produto);
    const Produto* buscarProdutoPorCodigo(const std::string& codigo) const;
    const std::vector<Produto>& obterProdutos() const;

    ResultadoMovimentacao registrarEntrada(const std::string& codigo, int quantidade);
    ResultadoMovimentacao registrarSaida(const std::string& codigo, int quantidade);
};

#endif
