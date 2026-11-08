#define NOMINMAX
#include <windows.h>

#include "Estoque.h"
#include "Historico.h"
#include "Persistencia.h"
#include "Produto.h"
#include "Relatorios.h"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <limits>
#include <string>

namespace {
std::string lerArquivo(const std::filesystem::path& caminho) {
    std::ifstream arquivo(caminho, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(arquivo), {});
}

void escreverArquivo(const std::filesystem::path& caminho, const std::string& conteudo) {
    std::ofstream arquivo(caminho, std::ios::binary | std::ios::trunc);
    arquivo.write(conteudo.data(), static_cast<std::streamsize>(conteudo.size()));
    arquivo.close();
    assert(arquivo.good());
}

Entrada movimento(long long id, const std::string& codigo, const std::string& nome,
                  TipoMovimentacao tipo, int quantidade, int antes, int depois) {
    return {id, codigo, nome, tipo, quantidade, "2026-11-08T12:30:00Z", antes, depois};
}

void testarProdutoEstoque() {
    assert(validarProduto(Produto{"P1", "Teclado", 15990, 4}));
    assert(!validarProduto(Produto{"", "Teclado", 15990, 4}));
    assert(!validarProduto(Produto{"P1", "", 15990, 4}));
    assert(!validarProduto(Produto{"P1", "Teclado", -1, 4}));
    assert(!validarProduto(Produto{"P1", "Teclado", 10, -1}));
    assert(!validarProduto(Produto{"P1", "Teclado", 10, 1, -1}));

    Estoque estoque;
    assert(estoque.cadastrarProduto(Produto{"P1", "Teclado", 15990, 4, 2}));
    assert(!estoque.cadastrarProduto(Produto{"P1", "Duplicado", 1, 1}));
    assert(estoque.buscarProdutoPorCodigo("P1") != nullptr);
    assert(estoque.buscarProdutoPorCodigo("ausente") == nullptr);

    assert(estoque.editarProduto("P1", "P2", "Teclado novo", 20000, 3) ==
           ResultadoProduto::sucesso);
    const Produto* editado = estoque.buscarProdutoPorCodigo("P2");
    assert(editado && editado->quantidade == 4 && editado->quantidadeMinima == 3);
    assert(estoque.cadastrarProduto(Produto{"P3", "Mouse", 500, 1}));
    assert(estoque.editarProduto("P2", "P3", "Nome", 100, 0) ==
           ResultadoProduto::codigoDuplicado);
    assert(estoque.buscarProdutoPorCodigo("P2")->nome == "Teclado novo");
    assert(estoque.editarProduto("P2", "", "Nome", 100, 0) ==
           ResultadoProduto::dadosInvalidos);
    assert(estoque.definirQuantidadeMinima("P2", 5) == ResultadoProduto::sucesso);
    assert(estoque.definirQuantidadeMinima("P2", -1) ==
           ResultadoProduto::quantidadeInvalida);
    assert(estoque.buscarProdutoPorCodigo("P2")->quantidadeMinima == 5);
    assert(estoque.excluirProduto("ausente") == ResultadoProduto::produtoNaoEncontrado);
    assert(estoque.excluirProduto("P3") == ResultadoProduto::sucesso);
    assert(estoque.buscarProdutoPorCodigo("P3") == nullptr);

    assert(estoque.registrarEntrada("P2", 3) == ResultadoMovimentacao::sucesso);
    assert(estoque.buscarProdutoPorCodigo("P2")->quantidade == 7);
    assert(estoque.registrarEntrada("P2", 0) == ResultadoMovimentacao::quantidadeInvalida);
    assert(estoque.registrarEntrada("P2", -1) == ResultadoMovimentacao::quantidadeInvalida);
    assert(estoque.registrarSaida("P2", 7) == ResultadoMovimentacao::sucesso);
    assert(estoque.buscarProdutoPorCodigo("P2")->quantidade == 0);
    assert(estoque.registrarSaida("P2", 1) == ResultadoMovimentacao::estoqueInsuficiente);
    assert(estoque.registrarSaida("P2", 0) == ResultadoMovimentacao::quantidadeInvalida);
    assert(estoque.registrarSaida("P2", -2) == ResultadoMovimentacao::quantidadeInvalida);
    assert(estoque.registrarEntrada("inexistente", 1) ==
           ResultadoMovimentacao::produtoNaoEncontrado);
    assert(estoque.registrarSaida("inexistente", 1) ==
           ResultadoMovimentacao::produtoNaoEncontrado);

    Estoque limite;
    const int maximo = std::numeric_limits<int>::max();
    assert(limite.cadastrarProduto(Produto{"MAX", "Limite", 0, maximo - 1}));
    assert(limite.registrarEntrada("MAX", 1) == ResultadoMovimentacao::sucesso);
    assert(limite.registrarEntrada("MAX", 1) ==
           ResultadoMovimentacao::limiteEstoqueExcedido);
    assert(limite.buscarProdutoPorCodigo("MAX")->quantidade == maximo);
}

void testarHistoricoERelatorios() {
    Historico historico;
    const Entrada primeira = movimento(1, "A", "Nome antigo", TipoMovimentacao::entrada,
                                       5, 0, 5);
    const Entrada segunda = movimento(2, "A", "Nome antigo", TipoMovimentacao::saida,
                                      2, 5, 3);
    assert(validarEntrada(primeira));
    assert(historico.adicionar(primeira));
    assert(!historico.adicionar(primeira));
    assert(historico.adicionar(segunda));
    assert(historico.proximoIdentificador() == 3);
    assert(!validarEntrada(movimento(3, "A", "X", TipoMovimentacao::entrada, 1, 0, 9)));
    assert(!validarEntrada(movimento(3, "A", "X", TipoMovimentacao::entrada,
                                     1, std::numeric_limits<int>::max(), 0)));

    Estoque estoque;
    assert(estoque.cadastrarProduto(Produto{"A", "Atual", 100, 3, 4}));
    assert(estoque.cadastrarProduto(Produto{"B", "Zerado", 500, 0, 0}));
    const RelatorioEstoque r = gerarRelatorioEstoque(estoque, historico);
    assert(r.totalProdutos == 2 && r.totalUnidades == 3);
    assert(r.valorTotalCentavos == 300 && !r.valorTotalOverflow);
    assert(r.abaixoDoMinimo.size() == 1 && r.abaixoDoMinimo[0].codigo == "A");
    assert(r.noMinimo.size() == 1 && r.noMinimo[0].codigo == "B");
    assert(r.semUnidades.size() == 1 && r.maiorQuantidade.size() == 1);
    assert(r.totalEntradas == 1 && r.totalSaidas == 1);
    assert(r.unidadesEntradas == 5 && r.unidadesSaidas == 2);

    Estoque overflow;
    assert(overflow.cadastrarProduto(Produto{"BIG", "Grande",
        std::numeric_limits<long long>::max(), 2}));
    assert(gerarRelatorioEstoque(overflow, Historico{}).valorTotalOverflow);
}

void testarPersistencia(const std::filesystem::path& diretorio) {
    std::string erro;
    const std::filesystem::path produtos = diretorio / "produtos.csv";
    const std::filesystem::path historicoPath = diretorio / "movimentacoes.csv";
    const std::string acento = std::string("Caf") + "\xC3\xA9";
    const std::string codigo = "COD,\"\r\n002";
    const std::string nome = "Item, \"especial\"\r\n" + acento;

    Estoque estoque;
    assert(estoque.cadastrarProduto(Produto{codigo, nome, 15990, 12, 4}));
    assert(estoque.cadastrarProduto(Produto{"P2", "Outro", 5, 0, 0}));
    assert(Persistencia::salvar(produtos.string(), estoque, erro));
    const std::string bytesProduto = lerArquivo(produtos);
    assert(bytesProduto.find("quantidadeMinima") != std::string::npos);

    Estoque carregado;
    assert(carregado.cadastrarProduto(Produto{"OLD", "Antigo", 1, 1}));
    assert(Persistencia::carregar(produtos.string(), carregado, erro));
    assert(carregado.obterProdutos().size() == 2);
    const Produto* recuperado = carregado.buscarProdutoPorCodigo(codigo);
    assert(recuperado && recuperado->nome == nome && recuperado->precoCentavos == 15990);
    assert(recuperado->quantidade == 12 && recuperado->quantidadeMinima == 4);
    assert(carregado.buscarProdutoPorCodigo("OLD") == nullptr);

    const auto legado = diretorio / "legado.csv";
    escreverArquivo(legado, "codigo,nome,precoCentavos,quantidade\nL1,Legado,10,3\n");
    Estoque legadoCarregado;
    assert(Persistencia::carregar(legado.string(), legadoCarregado, erro));
    assert(legadoCarregado.buscarProdutoPorCodigo("L1")->quantidadeMinima == 0);
    assert(Persistencia::salvar(legado.string(), legadoCarregado, erro));
    assert(lerArquivo(legado).find("quantidadeMinima") != std::string::npos);
    bool encontrouBackup = false;
    for (const auto& item : std::filesystem::directory_iterator(diretorio)) {
        if (item.path().filename().string().find("legado.csv.bak") == 0) {
            encontrouBackup = true;
            assert(lerArquivo(item.path()).find("codigo,nome,precoCentavos,quantidade\n") == 0);
        }
    }
    assert(encontrouBackup);

    const std::string header = "codigo,nome,precoCentavos,quantidade,quantidadeMinima\n";
    const auto corrupto = diretorio / "corrupto.csv";
    const std::vector<std::string> csvRuins = {
        "cabecalho,ruim\n",
        header + "P,Nome,1,2\n",
        header + "P,Nome,1,2,0,extra\n",
        header + "P,Nome,x,2,0\n",
        header + "P,Nome,-1,2,0\n",
        header + "P,Nome,1,-2,0\n",
        header + "P,Nome,1,2,-1\n",
        header + "P,Nome,9223372036854775808,2,0\n",
        header + "P,Nome,1,2147483648,0\n",
        header + "P,Nome,1,2,0\nP,Duplicado,1,2,0\n",
        header + "P,\"sem fechar,1,2,0\n",
        header + "P,\"fechado\"x,1,2,0\n"
    };
    for (const std::string& csvRuim : csvRuins) {
        escreverArquivo(corrupto, csvRuim);
        Estoque preservado;
        assert(preservado.cadastrarProduto(Produto{"KEEP", "Mantido", 3, 2}));
        assert(!Persistencia::carregar(corrupto.string(), preservado, erro));
        assert(erro.find("linha") != std::string::npos);
        assert(preservado.obterProdutos().size() == 1);
        assert(preservado.buscarProdutoPorCodigo("KEEP")->quantidade == 2);
    }

    const auto vazio = diretorio / "vazio.csv";
    escreverArquivo(vazio, "");
    assert(Persistencia::carregar(vazio.string(), carregado, erro));
    assert(carregado.obterProdutos().empty());
    assert(Persistencia::carregar((diretorio / "ausente" / "dados.csv").string(),
                                  carregado, erro));
    assert(carregado.obterProdutos().empty());

    Historico movs;
    assert(movs.adicionar(movimento(1, codigo, nome, TipoMovimentacao::entrada, 12, 0, 12)));
    assert(movs.adicionar(movimento(2, codigo, nome, TipoMovimentacao::saida, 3, 12, 9)));
    assert(Persistencia::salvarHistorico(historicoPath.string(), movs, erro));
    Historico movsCarregadas;
    assert(Persistencia::carregarHistorico(historicoPath.string(), movsCarregadas, erro));
    assert(movsCarregadas.obterEntradas().size() == 2);
    assert(movsCarregadas.obterEntradas()[0].nomeProduto == nome);
    assert(movsCarregadas.obterEntradas()[0].codigoProduto == codigo);
    assert(movsCarregadas.proximoIdentificador() == 3);

    const auto historicoRuim = diretorio / "historico-ruim.csv";
    escreverArquivo(historicoRuim,
        "identificador,codigoProduto,nomeProduto,tipo,quantidade,dataHoraUtc,"
        "quantidadeAnterior,quantidadePosterior\n"
        "1,A,Nome,entrada,2,2026-11-08T12:30:00Z,0,2\n"
        "1,A,Nome,entrada,2,2026-11-08T12:30:00Z,0,2\n");
    Historico historicoPreservado;
    assert(historicoPreservado.adicionar(movimento(9, "KEEP", "Keep",
        TipoMovimentacao::entrada, 1, 0, 1)));
    assert(!Persistencia::carregarHistorico(historicoRuim.string(), historicoPreservado, erro));
    assert(historicoPreservado.obterEntradas().size() == 1);
    assert(erro.find("linha") != std::string::npos);

    const auto bloqueado = diretorio / "bloqueado.csv";
    Estoque original;
    assert(original.cadastrarProduto(Produto{"OLD", "Original", 100, 1}));
    assert(Persistencia::salvar(bloqueado.string(), original, erro));
    const std::string bytesAntigos = lerArquivo(bloqueado);
    HANDLE handle = CreateFileW(bloqueado.c_str(), GENERIC_READ,
                                FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                                OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    assert(handle != INVALID_HANDLE_VALUE);
    Estoque atualizado;
    assert(atualizado.cadastrarProduto(Produto{"NEW", "Novo", 200, 2}));
    assert(!Persistencia::salvar(bloqueado.string(), atualizado, erro));
    CloseHandle(handle);
    assert(lerArquivo(bloqueado) == bytesAntigos);

    const auto arquivoPai = diretorio / "nao-diretorio";
    escreverArquivo(arquivoPai, "nao alterar");
    assert(!Persistencia::salvar((arquivoPai / "produtos.csv").string(), atualizado, erro));
    assert(lerArquivo(arquivoPai) == "nao alterar");

    const auto diretorioComoArquivo = diretorio / "diretorio.csv";
    std::filesystem::create_directory(diretorioComoArquivo);
    Estoque estoqueAntesLeitura;
    assert(estoqueAntesLeitura.cadastrarProduto(Produto{"SAFE", "Seguro", 1, 2}));
    assert(!Persistencia::carregar(diretorioComoArquivo.string(), estoqueAntesLeitura, erro));
    assert(estoqueAntesLeitura.buscarProdutoPorCodigo("SAFE") != nullptr);
}
}  // namespace

int main(int argc, char** argv) {
    assert(argc == 2);
    const std::filesystem::path diretorio(argv[1]);
    std::filesystem::create_directories(diretorio);
    testarProdutoEstoque();
    testarHistoricoERelatorios();
    testarPersistencia(diretorio);
    std::filesystem::remove_all(diretorio);
}
