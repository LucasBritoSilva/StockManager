#ifndef PERSISTENCIA_H
#define PERSISTENCIA_H

#include <string>

#include "Estoque.h"
#include "Historico.h"

class Persistencia {
public:
    static bool salvar(
        const std::string& caminho,
        const Estoque& estoque,
        std::string& erro);

    static bool carregar(
        const std::string& caminho,
        Estoque& estoque,
        std::string& erro);

    static bool salvarHistorico(
        const std::string& caminho,
        const Historico& historico,
        std::string& erro);

    static bool carregarHistorico(
        const std::string& caminho,
        Historico& historico,
        std::string& erro);
};

#endif
