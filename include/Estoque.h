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

enum class ResultadoProduto {
    sucesso,
    produtoNaoEncontrado,
    dadosInvalidos,
    codigoDuplicado,
    quantidadeInvalida
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

    ResultadoProduto editarProduto(const std::string& codigoAtual,
                                   const std::string& novoCodigo,
                                   const std::string& novoNome,
                                   long long novoPrecoCentavos,
                                   int novaQuantidadeMinima);
    ResultadoProduto excluirProduto(const std::string& codigo);
    ResultadoProduto definirQuantidadeMinima(const std::string& codigo, int quantidadeMinima);
};

#endif
