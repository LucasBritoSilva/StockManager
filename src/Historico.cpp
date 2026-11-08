#include "Historico.h"

#include <algorithm>
#include <limits>

const std::vector<Entrada>& Historico::obterEntradas() const {
    return entradas;
}

bool Historico::possuiCodigo(const std::string& codigo) const {
    for (const Entrada& entrada : entradas) {
        if (entrada.codigoProduto == codigo) return true;
    }
    return false;
}

bool Historico::adicionar(const Entrada& entrada) {
    if (!validarEntrada(entrada)) return false;

    const auto posicao = std::lower_bound(
        entradas.begin(), entradas.end(), entrada.identificador,
        [](const Entrada& existente, long long identificador) {
            return existente.identificador < identificador;
        });
    if (posicao != entradas.end() &&
        posicao->identificador == entrada.identificador) return false;

    auto anterior = posicao;
    while (anterior != entradas.begin()) {
        --anterior;
        if (anterior->codigoProduto == entrada.codigoProduto) {
            if (anterior->quantidadePosterior != entrada.quantidadeAnterior) return false;
            break;
        }
    }

    auto proxima = posicao;
    while (proxima != entradas.end()) {
        if (proxima->codigoProduto == entrada.codigoProduto) {
            if (entrada.quantidadePosterior != proxima->quantidadeAnterior) return false;
            break;
        }
        ++proxima;
    }

    entradas.insert(posicao, entrada);
    return true;
}

long long Historico::proximoIdentificador() const {
    long long maior = 0;
    for (const Entrada& entrada : entradas) {
        if (entrada.identificador > maior) maior = entrada.identificador;
    }
    if (maior == std::numeric_limits<long long>::max()) return 0;
    return maior + 1;
}
