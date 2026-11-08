#ifndef HISTORICO_H
#define HISTORICO_H

#include <vector>

#include "Entrada.h"

class Historico {
private:
    std::vector<Entrada> entradas;

public:
    const std::vector<Entrada>& obterEntradas() const;
    bool possuiCodigo(const std::string& codigo) const;
    bool adicionar(const Entrada& entrada);
    long long proximoIdentificador() const;
};

#endif
