#define NOMINMAX
#include <windows.h>

#include "Estoque.h"
#include "Historico.h"
#include "Persistencia.h"
#include "Produto.h"
#include "Relatorios.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <string>
#include <system_error>
#include <vector>

namespace {
void verificar(bool condicao, const char* expressao, int linha) {
    if (!condicao) {
        throw std::runtime_error("Falha na linha " + std::to_string(linha) +
                                 ": " + expressao);
    }
}

#define CHECK(...) verificar(static_cast<bool>((__VA_ARGS__)), #__VA_ARGS__, __LINE__)

struct HandleGuard {
    HANDLE handle;
    ~HandleGuard() {
        if (handle != INVALID_HANDLE_VALUE) CloseHandle(handle);
    }
};

std::filesystem::path criarDiretorioTemporario() {
    const std::filesystem::path base = std::filesystem::temp_directory_path();
    const unsigned long processo = GetCurrentProcessId();
    for (unsigned int tentativa = 0; tentativa < 1000; ++tentativa) {
        const std::filesystem::path candidato = base /
            ("StockManagerTests-" + std::to_string(processo) + "-" +
             std::to_string(tentativa));
        std::error_code erro;
        if (std::filesystem::create_directory(candidato, erro)) return candidato;
        if (erro) {
            throw std::filesystem::filesystem_error(
                "Nao foi possivel criar diretorio temporario", candidato, erro);
        }
    }
    throw std::runtime_error("Nao foi possivel reservar um diretorio temporario exclusivo.");
}

std::string lerArquivo(const std::filesystem::path& caminho) {
    std::ifstream arquivo(caminho, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(arquivo), {});
}

void escreverArquivo(const std::filesystem::path& caminho, const std::string& conteudo) {
    std::ofstream arquivo(caminho, std::ios::binary | std::ios::trunc);
    arquivo.write(conteudo.data(), static_cast<std::streamsize>(conteudo.size()));
    arquivo.close();
    CHECK(arquivo.good());
}

Entrada movimento(long long id, const std::string& codigo, const std::string& nome,
                  TipoMovimentacao tipo, int quantidade, int antes, int depois) {
    return {id, codigo, nome, tipo, quantidade, "2026-11-08T12:30:00Z", antes, depois};
}

void testarProdutoEstoque() {
    CHECK(validarProduto(Produto{"P1", "Teclado", 15990, 4}));
    CHECK(!validarProduto(Produto{"", "Teclado", 15990, 4}));
    CHECK(!validarProduto(Produto{"P1", "", 15990, 4}));
    CHECK(!validarProduto(Produto{"P1", "Teclado", -1, 4}));
    CHECK(!validarProduto(Produto{"P1", "Teclado", 10, -1}));
    CHECK(!validarProduto(Produto{"P1", "Teclado", 10, 1, -1}));

    Estoque estoque;
    CHECK(estoque.cadastrarProduto(Produto{"P1", "Teclado", 15990, 4, 2}));
    CHECK(!estoque.cadastrarProduto(Produto{"P1", "Duplicado", 1, 1}));
    CHECK(estoque.buscarProdutoPorCodigo("P1") != nullptr);
    CHECK(estoque.buscarProdutoPorCodigo("ausente") == nullptr);

    CHECK(estoque.editarProduto("P1", "P2", "Teclado novo", 20000, 3) ==
           ResultadoProduto::sucesso);
    const Produto* editado = estoque.buscarProdutoPorCodigo("P2");
    CHECK(editado && editado->quantidade == 4 && editado->quantidadeMinima == 3);
    CHECK(estoque.cadastrarProduto(Produto{"P3", "Mouse", 500, 1}));
    CHECK(estoque.editarProduto("P2", "P3", "Nome", 100, 0) ==
           ResultadoProduto::codigoDuplicado);
    CHECK(estoque.buscarProdutoPorCodigo("P2")->nome == "Teclado novo");
    CHECK(estoque.editarProduto("P2", "", "Nome", 100, 0) ==
           ResultadoProduto::dadosInvalidos);
    CHECK(estoque.definirQuantidadeMinima("P2", 5) == ResultadoProduto::sucesso);
    CHECK(estoque.definirQuantidadeMinima("P2", -1) ==
           ResultadoProduto::quantidadeInvalida);
    CHECK(estoque.buscarProdutoPorCodigo("P2")->quantidadeMinima == 5);
    CHECK(estoque.excluirProduto("ausente") == ResultadoProduto::produtoNaoEncontrado);
    CHECK(estoque.excluirProduto("P3") == ResultadoProduto::sucesso);
    CHECK(estoque.buscarProdutoPorCodigo("P3") == nullptr);

    CHECK(estoque.registrarEntrada("P2", 3) == ResultadoMovimentacao::sucesso);
    CHECK(estoque.buscarProdutoPorCodigo("P2")->quantidade == 7);
    CHECK(estoque.registrarEntrada("P2", 0) == ResultadoMovimentacao::quantidadeInvalida);
    CHECK(estoque.registrarEntrada("P2", -1) == ResultadoMovimentacao::quantidadeInvalida);
    CHECK(estoque.registrarSaida("P2", 7) == ResultadoMovimentacao::sucesso);
    CHECK(estoque.buscarProdutoPorCodigo("P2")->quantidade == 0);
    CHECK(estoque.registrarSaida("P2", 1) == ResultadoMovimentacao::estoqueInsuficiente);
    CHECK(estoque.registrarSaida("P2", 0) == ResultadoMovimentacao::quantidadeInvalida);
    CHECK(estoque.registrarSaida("P2", -2) == ResultadoMovimentacao::quantidadeInvalida);
    CHECK(estoque.registrarEntrada("inexistente", 1) ==
           ResultadoMovimentacao::produtoNaoEncontrado);
    CHECK(estoque.registrarSaida("inexistente", 1) ==
           ResultadoMovimentacao::produtoNaoEncontrado);

    Estoque limite;
    const int maximo = std::numeric_limits<int>::max();
    CHECK(limite.cadastrarProduto(Produto{"MAX", "Limite", 0, maximo - 1}));
    CHECK(limite.registrarEntrada("MAX", 1) == ResultadoMovimentacao::sucesso);
    CHECK(limite.registrarEntrada("MAX", 1) ==
           ResultadoMovimentacao::limiteEstoqueExcedido);
    CHECK(limite.buscarProdutoPorCodigo("MAX")->quantidade == maximo);
}

void testarHistoricoERelatorios() {
    Historico historico;
    const Entrada primeira = movimento(1, "A", "Nome antigo", TipoMovimentacao::entrada,
                                       5, 0, 5);
    const Entrada segunda = movimento(2, "A", "Nome antigo", TipoMovimentacao::saida,
                                      2, 5, 3);
    CHECK(validarEntrada(primeira));
    CHECK(historico.adicionar(primeira));
    CHECK(!historico.adicionar(primeira));
    CHECK(historico.adicionar(segunda));
    CHECK(historico.proximoIdentificador() == 3);
    CHECK(!validarEntrada(movimento(3, "A", "X", TipoMovimentacao::entrada, 1, 0, 9)));
    CHECK(!validarEntrada(movimento(3, "A", "X", TipoMovimentacao::entrada,
                                     1, std::numeric_limits<int>::max(), 0)));

    Historico ordenado;
    CHECK(ordenado.adicionar(movimento(3, "FORA", "Produto", TipoMovimentacao::entrada,
                                       3, 12, 15)));
    CHECK(ordenado.adicionar(movimento(2, "OUTRO", "Outro", TipoMovimentacao::entrada,
                                       1, 0, 1)));
    CHECK(ordenado.adicionar(movimento(1, "FORA", "Produto", TipoMovimentacao::entrada,
                                       2, 10, 12)));
    CHECK(ordenado.obterEntradas()[0].identificador == 1);
    CHECK(ordenado.obterEntradas()[1].identificador == 2);
    CHECK(ordenado.obterEntradas()[2].identificador == 3);
    CHECK(ordenado.possuiCodigo("FORA"));
    CHECK(!ordenado.possuiCodigo("AUSENTE"));
    CHECK(!ordenado.adicionar(movimento(4, "FORA", "Produto",
                                        TipoMovimentacao::entrada, 1, 20, 21)));

    Estoque removido;
    CHECK(removido.cadastrarProduto(Produto{"FORA", "Produto", 100, 15}));
    CHECK(removido.excluirProduto("FORA") == ResultadoProduto::sucesso);
    CHECK(removido.buscarProdutoPorCodigo("FORA") == nullptr);
    CHECK(ordenado.possuiCodigo("FORA"));

    Estoque estoque;
    CHECK(estoque.cadastrarProduto(Produto{"A", "Atual", 100, 3, 4}));
    CHECK(estoque.cadastrarProduto(Produto{"B", "Zerado", 500, 0, 0}));
    const RelatorioEstoque r = gerarRelatorioEstoque(estoque, historico);
    CHECK(r.totalProdutos == 2 && r.totalUnidades == 3);
    CHECK(r.valorTotalCentavos == 300 && !r.valorTotalOverflow);
    CHECK(r.abaixoDoMinimo.size() == 1 && r.abaixoDoMinimo[0].codigo == "A");
    CHECK(r.noMinimo.size() == 1 && r.noMinimo[0].codigo == "B");
    CHECK(r.semUnidades.size() == 1 && r.maiorQuantidade.size() == 1);
    CHECK(r.totalEntradas == 1 && r.totalSaidas == 1);
    CHECK(r.unidadesEntradas == 5 && r.unidadesSaidas == 2);

    Estoque overflow;
    CHECK(overflow.cadastrarProduto(Produto{"BIG", "Grande",
        std::numeric_limits<long long>::max(), 2}));
    CHECK(gerarRelatorioEstoque(overflow, Historico{}).valorTotalOverflow);
}

void testarPersistencia(const std::filesystem::path& diretorio) {
    std::string erro;
    const std::filesystem::path produtos = diretorio / "produtos.csv";
    const std::filesystem::path historicoPath = diretorio / "movimentacoes.csv";
    const std::string acento = std::string("Caf") + "\xC3\xA9";
    const std::string codigo = "COD,\"\r\n002";
    const std::string nome = "Item, \"especial\"\r\n" + acento;

    Estoque estoque;
    CHECK(estoque.cadastrarProduto(Produto{codigo, nome, 15990, 12, 4}));
    CHECK(estoque.cadastrarProduto(Produto{"P2", "Outro", 5, 0, 0}));
    CHECK(Persistencia::salvar(produtos.string(), estoque, erro));
    const std::string bytesProduto = lerArquivo(produtos);
    CHECK(bytesProduto.find("quantidadeMinima") != std::string::npos);

    Estoque carregado;
    CHECK(carregado.cadastrarProduto(Produto{"OLD", "Antigo", 1, 1}));
    CHECK(Persistencia::carregar(produtos.string(), carregado, erro));
    CHECK(carregado.obterProdutos().size() == 2);
    const Produto* recuperado = carregado.buscarProdutoPorCodigo(codigo);
    CHECK(recuperado && recuperado->nome == nome && recuperado->precoCentavos == 15990);
    CHECK(recuperado->quantidade == 12 && recuperado->quantidadeMinima == 4);
    CHECK(carregado.buscarProdutoPorCodigo("OLD") == nullptr);

    const auto legado = diretorio / "legado.csv";
    escreverArquivo(legado, "codigo,nome,precoCentavos,quantidade\nL1,Legado,10,3\n");
    Estoque legadoCarregado;
    CHECK(Persistencia::carregar(legado.string(), legadoCarregado, erro));
    CHECK(legadoCarregado.buscarProdutoPorCodigo("L1")->quantidadeMinima == 0);
    CHECK(Persistencia::salvar(legado.string(), legadoCarregado, erro));
    CHECK(lerArquivo(legado).find("quantidadeMinima") != std::string::npos);
    bool encontrouBackup = false;
    for (const auto& item : std::filesystem::directory_iterator(diretorio)) {
        if (item.path().filename().string().find("legado.csv.bak") == 0) {
            encontrouBackup = true;
            CHECK(lerArquivo(item.path()).find("codigo,nome,precoCentavos,quantidade\n") == 0);
        }
    }
    CHECK(encontrouBackup);

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
        CHECK(preservado.cadastrarProduto(Produto{"KEEP", "Mantido", 3, 2}));
        CHECK(!Persistencia::carregar(corrupto.string(), preservado, erro));
        CHECK(erro.find("linha") != std::string::npos);
        CHECK(preservado.obterProdutos().size() == 1);
        CHECK(preservado.buscarProdutoPorCodigo("KEEP")->quantidade == 2);
    }

    const auto vazio = diretorio / "vazio.csv";
    escreverArquivo(vazio, "");
    CHECK(Persistencia::carregar(vazio.string(), carregado, erro));
    CHECK(carregado.obterProdutos().empty());
    CHECK(Persistencia::carregar((diretorio / "ausente" / "dados.csv").string(),
                                  carregado, erro));
    CHECK(carregado.obterProdutos().empty());

    Historico movs;
    CHECK(movs.adicionar(movimento(1, codigo, nome, TipoMovimentacao::entrada, 12, 0, 12)));
    CHECK(movs.adicionar(movimento(2, codigo, nome, TipoMovimentacao::saida, 3, 12, 9)));
    CHECK(Persistencia::salvarHistorico(historicoPath.string(), movs, erro));
    Historico movsCarregadas;
    CHECK(Persistencia::carregarHistorico(historicoPath.string(), movsCarregadas, erro));
    CHECK(movsCarregadas.obterEntradas().size() == 2);
    CHECK(movsCarregadas.obterEntradas()[0].nomeProduto == nome);
    CHECK(movsCarregadas.obterEntradas()[0].codigoProduto == codigo);
    CHECK(movsCarregadas.proximoIdentificador() == 3);

    const std::string historicoHeader =
        "identificador,codigoProduto,nomeProduto,tipo,quantidade,dataHoraUtc,"
        "quantidadeAnterior,quantidadePosterior\n";
    const auto foraDeOrdem = diretorio / "historico-fora-de-ordem.csv";
    escreverArquivo(foraDeOrdem, historicoHeader +
        "2,P,Produto,entrada,3,2026-11-08T12:30:00Z,12,15\n"
        "1,P,Produto,entrada,2,2026-11-08T12:00:00Z,10,12\n");
    Historico historicoOrdenado;
    CHECK(Persistencia::carregarHistorico(foraDeOrdem.string(), historicoOrdenado, erro));
    CHECK(historicoOrdenado.obterEntradas().size() == 2);
    CHECK(historicoOrdenado.obterEntradas()[0].identificador == 1);
    CHECK(historicoOrdenado.obterEntradas()[1].identificador == 2);

    const auto saldoIncompativel = diretorio / "historico-saldo-incompativel.csv";
    escreverArquivo(saldoIncompativel, historicoHeader +
        "1,P,Produto,entrada,2,2026-11-08T12:00:00Z,0,2\n"
        "2,Q,Outro,entrada,1,2026-11-08T12:15:00Z,0,1\n"
        "3,P,Produto,entrada,1,2026-11-08T12:30:00Z,8,9\n");
    Historico historicoAntesDoErro;
    CHECK(historicoAntesDoErro.adicionar(movimento(7, "KEEP", "Keep",
        TipoMovimentacao::entrada, 1, 0, 1)));
    CHECK(!Persistencia::carregarHistorico(saldoIncompativel.string(),
                                            historicoAntesDoErro, erro));
    CHECK(historicoAntesDoErro.obterEntradas().size() == 1);
    CHECK(historicoAntesDoErro.obterEntradas()[0].codigoProduto == "KEEP");

    const auto historicoRuim = diretorio / "historico-ruim.csv";
    escreverArquivo(historicoRuim,
        "identificador,codigoProduto,nomeProduto,tipo,quantidade,dataHoraUtc,"
        "quantidadeAnterior,quantidadePosterior\n"
        "1,A,Nome,entrada,2,2026-11-08T12:30:00Z,0,2\n"
        "1,A,Nome,entrada,2,2026-11-08T12:30:00Z,0,2\n");
    Historico historicoPreservado;
    CHECK(historicoPreservado.adicionar(movimento(9, "KEEP", "Keep",
        TipoMovimentacao::entrada, 1, 0, 1)));
    CHECK(!Persistencia::carregarHistorico(historicoRuim.string(), historicoPreservado, erro));
    CHECK(historicoPreservado.obterEntradas().size() == 1);
    CHECK(erro.find("linha") != std::string::npos);

    const auto bloqueado = diretorio / "bloqueado.csv";
    Estoque original;
    CHECK(original.cadastrarProduto(Produto{"OLD", "Original", 100, 1}));
    CHECK(Persistencia::salvar(bloqueado.string(), original, erro));
    const std::string bytesAntigos = lerArquivo(bloqueado);
    Estoque atualizado;
    CHECK(atualizado.cadastrarProduto(Produto{"NEW", "Novo", 200, 2}));
    HandleGuard handle{CreateFileW(bloqueado.c_str(), GENERIC_READ,
                                   FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                                   OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr)};
    if (handle.handle == INVALID_HANDLE_VALUE) {
        throw std::runtime_error("Nao foi possivel abrir o arquivo para o teste de bloqueio.");
    }
    const bool gravacaoBloqueada = Persistencia::salvar(bloqueado.string(), atualizado, erro);
    CHECK(!gravacaoBloqueada);
    CHECK(lerArquivo(bloqueado) == bytesAntigos);

    const auto historicoBloqueado = diretorio / "historico-bloqueado.csv";
    Historico historicoOriginal;
    CHECK(historicoOriginal.adicionar(movimento(1, "H", "Historico",
        TipoMovimentacao::entrada, 2, 0, 2)));
    CHECK(Persistencia::salvarHistorico(historicoBloqueado.string(),
                                        historicoOriginal, erro));
    const std::string bytesHistoricoAntigos = lerArquivo(historicoBloqueado);
    Historico historicoAtualizado = historicoOriginal;
    CHECK(historicoAtualizado.adicionar(movimento(2, "H", "Historico",
        TipoMovimentacao::saida, 1, 2, 1)));
    bool historicoSalvoComBloqueio = false;
    {
        HandleGuard travaHistorico{CreateFileW(historicoBloqueado.c_str(), GENERIC_READ,
            FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL, nullptr)};
        if (travaHistorico.handle == INVALID_HANDLE_VALUE) {
            throw std::runtime_error("Nao foi possivel bloquear o arquivo de historico.");
        }
        historicoSalvoComBloqueio = Persistencia::salvarHistorico(
            historicoBloqueado.string(), historicoAtualizado, erro);
    }
    CHECK(!historicoSalvoComBloqueio);
    CHECK(lerArquivo(historicoBloqueado) == bytesHistoricoAntigos);

    const auto arquivoPai = diretorio / "nao-diretorio";
    escreverArquivo(arquivoPai, "nao alterar");
    CHECK(!Persistencia::salvar((arquivoPai / "produtos.csv").string(), atualizado, erro));
    CHECK(lerArquivo(arquivoPai) == "nao alterar");

    const auto diretorioComoArquivo = diretorio / "diretorio.csv";
    std::filesystem::create_directory(diretorioComoArquivo);
    Estoque estoqueAntesLeitura;
    CHECK(estoqueAntesLeitura.cadastrarProduto(Produto{"SAFE", "Seguro", 1, 2}));
    CHECK(!Persistencia::carregar(diretorioComoArquivo.string(), estoqueAntesLeitura, erro));
    CHECK(estoqueAntesLeitura.buscarProdutoPorCodigo("SAFE") != nullptr);
}
}  // namespace

int main() {
    std::filesystem::path diretorio;
    int resultado = 0;
    try {
        diretorio = criarDiretorioTemporario();
        testarProdutoEstoque();
        testarHistoricoERelatorios();
        testarPersistencia(diretorio);
        std::cout << "Todos os testes C++ passaram.\n";
    } catch (const std::exception& erro) {
        std::cerr << erro.what() << '\n';
        resultado = 1;
    }

    if (!diretorio.empty()) {
        std::error_code erroLimpeza;
        std::filesystem::remove_all(diretorio, erroLimpeza);
        if (erroLimpeza) {
            std::cerr << "Nao foi possivel remover a pasta temporaria do teste: "
                      << erroLimpeza.message() << '\n';
            resultado = 1;
        }
    }
    return resultado;
}
