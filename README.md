# StockManager

Sistema de gerenciamento de estoque em C++17, executado no terminal.

## Funcionalidades

- Cadastro e consulta de produtos.
- Listagem do estoque com preço em reais.
- Registro e consulta do histórico de entradas e saídas.
- Edição de código, nome, preço e estoque mínimo; a quantidade atual é preservada.
- Exclusão confirmada de produtos; o histórico de movimentações permanece disponível.
- Códigos que já aparecem no histórico ficam reservados: não podem ser reutilizados em outro cadastro ou em uma troca de código.
- Alertas de estoque abaixo do mínimo, no mínimo e sem unidades.
- Relatórios de produtos, unidades, valor estimado e movimentações.
- Salvamento automático dos dados em CSV e recuperação de uma movimentação interrompida.

## Requisitos

- Windows 10 ou posterior.
- MSYS2 UCRT64 com G++ compatível com C++17.
- PowerShell para os comandos abaixo.

O projeto não utiliza bibliotecas externas. O salvamento substitui arquivos de forma segura pelas APIs de arquivo do Windows.

## Compilar e executar

No diretório do projeto:

```powershell
& 'C:\msys64\ucrt64\bin\g++.exe' -std=c++17 -Wall -Wextra -pedantic `
  -Iinclude src/main.cpp src/Produto.cpp src/Estoque.cpp src/Entrada.cpp `
  src/Persistencia.cpp src/Historico.cpp src/Relatorios.cpp `
  -o StockManager.exe
./StockManager.exe
```

O programa cria `data/` quando for necessário salvar. O diretório é ignorado pelo Git para manter os dados locais fora do repositório.

## Dados CSV

`data/produtos.csv` armazena `codigo`, `nome`, `precoCentavos`, `quantidade` e `quantidadeMinima`. Preços são inteiros em centavos. A leitura também aceita o formato anterior de quatro colunas; nesse formato, o estoque mínimo começa em zero. A próxima gravação usa o formato atual.

`data/movimentacoes.csv` registra identificador, código e nome do produto, tipo de movimento, quantidade movimentada, data/hora UTC e quantidades anterior e posterior. Campos de texto seguem o escape CSV, incluindo vírgulas, aspas e quebras de linha; os arquivos são gravados em UTF-8.

Um código associado a movimentações não pode ser usado em um novo cadastro nem escolhido como novo código em uma edição. Excluir o produto continua permitido e não apaga seu histórico. Essa reserva simples evita que a recuperação associe eventos antigos a um produto diferente; códigos históricos não podem ser liberados pelo programa.

Cada gravação primeiro escreve um arquivo temporário e, no Windows, substitui o arquivo de destino usando `ReplaceFileW` ou `MoveFileExW`. Quando já existe um destino, a versão anterior é mantida em um arquivo `.bak...` ao lado dele. Backups não são removidos automaticamente. O histórico é organizado por identificador, rejeita identificadores duplicados e saldos que não formam uma sequência para o mesmo código. Se o histórico de uma movimentação for gravado antes de uma interrupção, a inicialização confere o saldo anterior/posterior e tenta completar a gravação do estoque. Divergências são reportadas e impedem que o aplicativo sobrescreva os dados silenciosamente.

O carregamento é transacional: arquivos malformados, registros inválidos ou códigos duplicados não substituem o estado em memória. Arquivo inexistente ou vazio representa uma coleção vazia. O histórico não é apagado quando um produto é excluído.

## Testes

Execute a suíte no PowerShell. O parâmetro de política vale somente para esse processo e não altera as configurações do Windows:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\tests\run_tests.ps1
```

O script compila a aplicação e os testes com `-std=c++17 -Wall -Wextra -pedantic`, executa a suíte tanto normalmente quanto com `NDEBUG`, testa o limite de tempo e faz fluxos de terminal e recuperação com reinicialização. O executável C++ cria seu próprio diretório temporário exclusivo e não aceita caminhos de limpeza. Binários e dados dos testes são criados em uma pasta temporária do Windows e removidos ao final.

## Exemplo de uso

Ao iniciar, escolha `1` para cadastrar e informe código, nome, preço com vírgula decimal (por exemplo, `159,90`) e quantidade. Depois, use `4` ou `5` para registrar entrada ou saída; `6` edita os dados e o estoque mínimo, e `7` exclui após confirmação. A opção `8` consulta alertas, `9` exibe relatórios e `10` consulta as movimentações. Digite `0` para sair. Os dados são salvos automaticamente em `data/` no diretório de execução.

## Estrutura

- `include/`: interfaces das estruturas e classes.
- `src/`: implementação e interface de terminal.
- `tests/`: testes automatizados e roteiro PowerShell.
- `data/`: arquivos locais de produtos e movimentações (criados em execução, não versionados).

## Limitações

- O arquivo é local à pasta de execução; não há sincronização entre várias cópias ou usuários.
- Não há bloqueio para múltiplas instâncias simultâneas do programa.
- A substituição atômica e os backups utilizam APIs do Windows.
- A data gravada no histórico é UTC.
