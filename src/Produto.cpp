#include "Produto.h"

bool validarProduto(const Produto& produto) {
    if (produto.codigo.empty()) {
        return false;
    }

    if (produto.nome.empty()) {
        return false;
    }

    if (produto.precoCentavos < 0) {
        return false;
    }

    if (produto.quantidade < 0) {
        return false;
    }

    if (produto.quantidadeMinima < 0) {
        return false;
    }

    return true;
}
