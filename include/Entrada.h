#ifndef ENTRADA_H
#define ENTRADA_H

#include <string>

enum class TipoMovimentacao {
    entrada,
    saida
};

struct Entrada {
    long long identificador = 0;
    std::string codigoProduto;
    std::string nomeProduto;
    TipoMovimentacao tipo = TipoMovimentacao::entrada;
    int quantidade = 0;
    std::string dataHoraUtc;
    int quantidadeAnterior = 0;
    int quantidadePosterior = 0;
};

bool validarEntrada(const Entrada& entrada);
std::string dataHoraUtcAtual();

#endif
