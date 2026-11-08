#include "Historico.h"

#include <limits>

const std::vector<Entrada>& Historico::obterEntradas() const {
    return entradas;
}

bool Historico::adicionar(const Entrada& entrada) {
    if (!validarEntrada(entrada)) return false;
    for (const Entrada& existente : entradas) {
        if (existente.identificador == entrada.identificador) return false;
    }
    entradas.push_back(entrada);
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
