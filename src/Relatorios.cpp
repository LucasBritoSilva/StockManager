#include "Relatorios.h"

#include <limits>

namespace {
bool somarComLimite(long long atual, long long valor, long long& resultado) {
    if (valor < 0 || atual > std::numeric_limits<long long>::max() - valor) {
        return false;
    }
    resultado = atual + valor;
    return true;
}
}

RelatorioEstoque gerarRelatorioEstoque(const Estoque& estoque,
                                       const Historico& historico) {
    RelatorioEstoque relatorio;
    const auto& produtos = estoque.obterProdutos();
    relatorio.totalProdutos = produtos.size();

    int maiorQuantidade = -1;
    for (const Produto& produto : produtos) {
        long long novoTotal = 0;
        if (!relatorio.totalUnidadesOverflow) {
            if (somarComLimite(relatorio.totalUnidades, produto.quantidade, novoTotal)) {
                relatorio.totalUnidades = novoTotal;
            } else {
                relatorio.totalUnidadesOverflow = true;
            }
        }

        if (!relatorio.valorTotalOverflow) {
            if (produto.quantidade > 0 &&
                produto.precoCentavos > std::numeric_limits<long long>::max() /
                                             produto.quantidade) {
                relatorio.valorTotalOverflow = true;
            } else {
                const long long valorProduto =
                    produto.precoCentavos * produto.quantidade;
                if (somarComLimite(relatorio.valorTotalCentavos, valorProduto, novoTotal)) {
                    relatorio.valorTotalCentavos = novoTotal;
                } else {
                    relatorio.valorTotalOverflow = true;
                }
            }
        }

        if (produto.quantidade < produto.quantidadeMinima) {
            relatorio.abaixoDoMinimo.push_back(produto);
        } else if (produto.quantidade == produto.quantidadeMinima) {
            relatorio.noMinimo.push_back(produto);
        }
        if (produto.quantidade == 0) relatorio.semUnidades.push_back(produto);

        if (produto.quantidade > maiorQuantidade) {
            maiorQuantidade = produto.quantidade;
            relatorio.maiorQuantidade.clear();
            relatorio.maiorQuantidade.push_back(produto);
        } else if (produto.quantidade == maiorQuantidade) {
            relatorio.maiorQuantidade.push_back(produto);
        }
    }

    for (const Entrada& entrada : historico.obterEntradas()) {
        long long novoTotal = 0;
        if (entrada.tipo == TipoMovimentacao::entrada) {
            ++relatorio.totalEntradas;
            if (!relatorio.unidadesEntradasOverflow) {
                if (somarComLimite(relatorio.unidadesEntradas, entrada.quantidade, novoTotal)) {
                    relatorio.unidadesEntradas = novoTotal;
                } else {
                    relatorio.unidadesEntradasOverflow = true;
                }
            }
        } else {
            ++relatorio.totalSaidas;
            if (!relatorio.unidadesSaidasOverflow) {
                if (somarComLimite(relatorio.unidadesSaidas, entrada.quantidade, novoTotal)) {
                    relatorio.unidadesSaidas = novoTotal;
                } else {
                    relatorio.unidadesSaidasOverflow = true;
                }
            }
        }
    }
    return relatorio;
}
