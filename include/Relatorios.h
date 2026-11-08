#ifndef RELATORIOS_H
#define RELATORIOS_H

#include <cstddef>
#include <vector>

#include "Estoque.h"
#include "Historico.h"

struct RelatorioEstoque {
    std::size_t totalProdutos = 0;
    long long totalUnidades = 0;
    bool totalUnidadesOverflow = false;
    long long valorTotalCentavos = 0;
    bool valorTotalOverflow = false;
    std::vector<Produto> abaixoDoMinimo;
    std::vector<Produto> noMinimo;
    std::vector<Produto> semUnidades;
    std::vector<Produto> maiorQuantidade;
    std::size_t totalEntradas = 0;
    std::size_t totalSaidas = 0;
    long long unidadesEntradas = 0;
    long long unidadesSaidas = 0;
    bool unidadesEntradasOverflow = false;
    bool unidadesSaidasOverflow = false;
};

RelatorioEstoque gerarRelatorioEstoque(const Estoque& estoque,
                                       const Historico& historico);

#endif
