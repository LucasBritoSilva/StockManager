$ErrorActionPreference = 'Stop'

$projectRoot = Split-Path -Parent $PSScriptRoot
$compiler = 'C:\msys64\ucrt64\bin\g++.exe'
if (-not (Test-Path -LiteralPath $compiler)) {
    throw "G++ nao encontrado em $compiler"
}

$temporaryRoot = Join-Path ([System.IO.Path]::GetTempPath()) (
    'stockmanager-tests-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $temporaryRoot | Out-Null

function Assert-Contains([string] $text, [string] $expected, [string] $context) {
    if (-not $text.Contains($expected)) {
        throw "Falha em '$context': texto esperado nao encontrado: $expected`n$text"
    }
}

function Invoke-Process([string] $executable, [string] $workingDirectory,
                        [string] $inputText, [int] $timeoutMilliseconds = 15000,
                        [switch] $AllowNonZeroExit) {
    $startInfo = New-Object System.Diagnostics.ProcessStartInfo
    $startInfo.FileName = $executable
    $startInfo.WorkingDirectory = $workingDirectory
    $startInfo.UseShellExecute = $false
    $startInfo.RedirectStandardInput = $true
    $startInfo.RedirectStandardOutput = $true
    $startInfo.RedirectStandardError = $true
    $process = New-Object System.Diagnostics.Process
    $process.StartInfo = $startInfo
    if (-not $process.Start()) { throw 'Nao foi possivel iniciar o StockManager.' }
    $stdoutTask = $process.StandardOutput.ReadToEndAsync()
    $stderrTask = $process.StandardError.ReadToEndAsync()
    $process.StandardInput.Write($inputText)
    $process.StandardInput.Close()
    $timedOut = -not $process.WaitForExit($timeoutMilliseconds)
    if ($timedOut) {
        $process.Kill()
        $process.WaitForExit()
    }
    $stdout = $stdoutTask.GetAwaiter().GetResult()
    $stderr = $stderrTask.GetAwaiter().GetResult()
    if (-not $timedOut -and $process.ExitCode -ne 0 -and -not $AllowNonZeroExit) {
        throw "Processo terminou com codigo $($process.ExitCode).`n$stderr`n$stdout"
    }
    return [PSCustomObject]@{
        TimedOut = $timedOut
        ExitCode = $process.ExitCode
        Output = $stdout + $stderr
    }
}

try {
    $sources = @(
        'src/Produto.cpp', 'src/Estoque.cpp', 'src/Entrada.cpp',
        'src/Persistencia.cpp', 'src/Historico.cpp', 'src/Relatorios.cpp'
    ) | ForEach-Object { Join-Path $projectRoot $_ }
    $testExecutable = Join-Path $temporaryRoot 'test_stockmanager.exe'
    $releaseTestExecutable = Join-Path $temporaryRoot 'test_stockmanager_ndebug.exe'
    $appExecutable = Join-Path $temporaryRoot 'StockManager.exe'
    $timeoutSource = Join-Path $temporaryRoot 'timeout.cpp'
    $timeoutExecutable = Join-Path $temporaryRoot 'timeout.exe'

    Push-Location $projectRoot
    try {
        & $compiler -std=c++17 -Wall -Wextra -pedantic -Iinclude `
            (Join-Path $projectRoot 'tests/test_stockmanager.cpp') @sources `
            -o $testExecutable
        if ($LASTEXITCODE -ne 0) { throw "Compilacao da suite falhou ($LASTEXITCODE)." }

        & $compiler -DNDEBUG -std=c++17 -Wall -Wextra -pedantic -Iinclude `
            (Join-Path $projectRoot 'tests/test_stockmanager.cpp') @sources `
            -o $releaseTestExecutable
        if ($LASTEXITCODE -ne 0) { throw "Compilacao da suite NDEBUG falhou ($LASTEXITCODE)." }

        & $compiler -std=c++17 -Wall -Wextra -pedantic -Iinclude `
            (Join-Path $projectRoot 'src/main.cpp') @sources -o $appExecutable
        if ($LASTEXITCODE -ne 0) { throw "Compilacao do programa falhou ($LASTEXITCODE)." }
    }
    finally { Pop-Location }

    $normalResult = (& $testExecutable | Out-String).Trim()
    if ($LASTEXITCODE -ne 0) { throw "Testes de dominio/persistencia falharam ($LASTEXITCODE)." }
    $releaseResult = (& $releaseTestExecutable | Out-String).Trim()
    if ($LASTEXITCODE -ne 0) { throw "Testes NDEBUG falharam ($LASTEXITCODE)." }
    if ($normalResult -ne $releaseResult) {
        throw "Resultados diferentes entre build normal e NDEBUG: '$normalResult' / '$releaseResult'"
    }
    Write-Output 'OK: testes de dominio/persistencia iguais em build normal e NDEBUG.'

    $externalDirectory = Join-Path $temporaryRoot 'diretorio-protegido'
    New-Item -ItemType Directory -Path $externalDirectory | Out-Null
    $externalSentinel = Join-Path $externalDirectory 'preservar.txt'
    [System.IO.File]::WriteAllText($externalSentinel, 'nao remover')
    & $testExecutable $externalDirectory
    if ($LASTEXITCODE -ne 0) { throw "Teste sem argumentos falhou ($LASTEXITCODE)." }
    if (-not (Test-Path -LiteralPath $externalSentinel)) {
        throw 'O teste removeu um diretorio externo protegido.'
    }
    Write-Output 'OK: caminho externo passado por engano nao e excluido pelo executavel de testes.'

    [System.IO.File]::WriteAllText($timeoutSource, @'
#include <chrono>
#include <iostream>
#include <thread>

int main() {
    std::cout << "started" << std::flush;
    std::this_thread::sleep_for(std::chrono::seconds(20));
}
'@, [System.Text.Encoding]::ASCII)
    & $compiler -std=c++17 $timeoutSource -o $timeoutExecutable
    if ($LASTEXITCODE -ne 0) { throw "Compilacao do processo de timeout falhou ($LASTEXITCODE)." }
    $timeoutResult = Invoke-Process $timeoutExecutable $temporaryRoot '' 1000
    if (-not $timeoutResult.TimedOut -or -not $timeoutResult.Output.Contains('started')) {
        throw 'O teste de timeout nao encerrou/controlou o processo como esperado.'
    }
    Write-Output 'OK: timeout mata processo com stdout aberto e captura a saida parcial.'

    $integration = Join-Path $temporaryRoot 'integracao'
    New-Item -ItemType Directory -Path $integration | Out-Null
    $inputScript = @(
        '1', 'K1', 'Teclado', '10,50', '10',
        '4', 'K1', '5',
        '5', 'K1', '5',
        '6', 'K1', 'K1-NOVO', 'Teclado Gamer', '12,00', '12',
        '4', 'K1-NOVO', '1',
        '5', 'K1-NOVO', '1',
        '6', 'K1-NOVO', 'K1',
        '7', 'K1-NOVO', 'n',
        '8', '9', '10',
        '7', 'K1-NOVO', 'S',
        '1', 'K1-NOVO',
        '10', '0'
    ) -join "`n"
    $output = (Invoke-Process $appExecutable $integration ($inputScript + "`n")).Output
    Assert-Contains $output 'Produto cadastrado e salvo com sucesso.' 'cadastro terminal'
    Assert-Contains $output 'Entrada registrada e salva com sucesso.' 'entrada terminal'
    Assert-Contains $output 'Saida registrada e salva com sucesso.' 'saida terminal'
    Assert-Contains $output 'Produto atualizado e salvo.' 'edicao terminal'
    Assert-Contains $output 'Exclusao cancelada.' 'cancelamento de exclusao'
    Assert-Contains $output 'Teclado Gamer' 'listagem apos edicao'
    Assert-Contains $output '10 unidades' 'quantidade preservada na edicao'
    Assert-Contains $output 'Abaixo do minimo: 1' 'alerta de estoque minimo'
    Assert-Contains $output 'Unidades de entrada: 6' 'relatorio de entrada'
    Assert-Contains $output 'Unidades de saida: 6' 'relatorio de saida'
    Assert-Contains $output 'Produto excluido.' 'exclusao confirmada'
    Assert-Contains $output 'Esse codigo ja possui movimentacoes e nao pode ser reutilizado.' `
        'bloqueio de reuso de codigo historico em edicao e cadastro'
    if ([regex]::Matches($output, 'Esse codigo ja possui movimentacoes e nao pode ser reutilizado\.').Count -lt 2) {
        throw 'O cadastro tambem deve rejeitar o codigo apos a exclusao.'
    }
    if (-not (Test-Path (Join-Path $integration 'data/produtos.csv'))) {
        throw 'Arquivo persistido de produtos nao foi criado.'
    }
    if (-not (Test-Path (Join-Path $integration 'data/movimentacoes.csv'))) {
        throw 'Arquivo persistido de historico nao foi criado.'
    }
    Write-Output 'OK: fluxo de terminal, minimo, relatorios, edicao/exclusao e persistencia.'

    $restart = (Invoke-Process $appExecutable $integration "3`n10`n0`n").Output
    Assert-Contains $restart 'Estoque vazio.' 'persistencia apos excluir produto'
    Assert-Contains $restart 'ENTRADA | K1 | Teclado | 10 -> 15' 'historico preservado'
    Assert-Contains $restart 'SAIDA | K1 | Teclado | 15 -> 10' 'historico apos reiniciar'
    Write-Output 'OK: recarga apos reiniciar; historico mantido mesmo com produto excluido.'

    $recoveryDirectory = Join-Path $temporaryRoot 'recuperacao-fora-de-ordem'
    $recoveryData = Join-Path $recoveryDirectory 'data'
    New-Item -ItemType Directory -Path $recoveryData -Force | Out-Null
    [System.IO.File]::WriteAllText(
        (Join-Path $recoveryData 'produtos.csv'),
        "codigo,nome,precoCentavos,quantidade,quantidadeMinima`nP,Produto,100,12,0`n",
        [System.Text.Encoding]::UTF8)
    [System.IO.File]::WriteAllText(
        (Join-Path $recoveryData 'movimentacoes.csv'),
        "identificador,codigoProduto,nomeProduto,tipo,quantidade,dataHoraUtc,quantidadeAnterior,quantidadePosterior`n" +
        "2,P,Produto,entrada,3,2026-11-08T12:30:00Z,12,15`n" +
        "1,P,Produto,entrada,2,2026-11-08T12:00:00Z,10,12`n",
        [System.Text.Encoding]::UTF8)
    $recoveryOutput = (Invoke-Process $appExecutable $recoveryDirectory "3`n0`n").Output
    Assert-Contains $recoveryOutput 'Movimentacao pendente recuperada do historico.' `
        'recuperacao de historico fora de ordem'
    if (-not (Get-Content (Join-Path $recoveryData 'produtos.csv') -Raw).Contains(',15,0')) {
        throw 'A movimentacao pendente nao foi recuperada em ordem crescente de identificador.'
    }
    Write-Output 'OK: recuperacao ordena eventos por identificador e aplica somente o pendente.'

    $invalidHistoryDirectory = Join-Path $temporaryRoot 'historico-incompativel'
    $invalidHistoryData = Join-Path $invalidHistoryDirectory 'data'
    New-Item -ItemType Directory -Path $invalidHistoryData -Force | Out-Null
    $invalidProducts = Join-Path $invalidHistoryData 'produtos.csv'
    $invalidMovements = Join-Path $invalidHistoryData 'movimentacoes.csv'
    [System.IO.File]::WriteAllText($invalidProducts,
        "codigo,nome,precoCentavos,quantidade,quantidadeMinima`nP,Produto,100,9,0`n",
        [System.Text.Encoding]::UTF8)
    [System.IO.File]::WriteAllText($invalidMovements,
        "identificador,codigoProduto,nomeProduto,tipo,quantidade,dataHoraUtc,quantidadeAnterior,quantidadePosterior`n" +
        "1,P,Produto,entrada,2,2026-11-08T12:00:00Z,0,2`n" +
        "2,P,Produto,entrada,1,2026-11-08T12:30:00Z,8,9`n",
        [System.Text.Encoding]::UTF8)
    $productsBeforeError = [System.IO.File]::ReadAllBytes($invalidProducts)
    $historyBeforeError = [System.IO.File]::ReadAllBytes($invalidMovements)
    $invalidStart = Invoke-Process $appExecutable $invalidHistoryDirectory '' 15000 -AllowNonZeroExit
    if ($invalidStart.ExitCode -eq 0) {
        throw 'O programa aceitou historico com saldos incompatíveis.'
    }
    Assert-Contains $invalidStart.Output 'Erro ao carregar o historico' 'historico inconsistente'
    $productsAfterError = [System.IO.File]::ReadAllBytes($invalidProducts)
    $historyAfterError = [System.IO.File]::ReadAllBytes($invalidMovements)
    if ([System.Convert]::ToBase64String($productsBeforeError) -ne
            [System.Convert]::ToBase64String($productsAfterError) -or
        [System.Convert]::ToBase64String($historyBeforeError) -ne
            [System.Convert]::ToBase64String($historyAfterError)) {
        throw 'Os arquivos originais foram alterados ao rejeitar o historico inconsistente.'
    }
    Write-Output 'OK: saldos incompatíveis são rejeitados sem alterar os CSVs originais.'

    $failureDirectory = Join-Path $temporaryRoot 'falha-de-gravacao'
    $failureData = Join-Path $failureDirectory 'data'
    New-Item -ItemType Directory -Path $failureData -Force | Out-Null
    $productsFile = Join-Path $failureData 'produtos.csv'
    [System.IO.File]::WriteAllText(
        $productsFile,
        "codigo,nome,precoCentavos,quantidade,quantidadeMinima`n",
        [System.Text.Encoding]::UTF8)
    $fileLock = [System.IO.File]::Open(
        $productsFile,
        [System.IO.FileMode]::Open,
        [System.IO.FileAccess]::Read,
        [System.IO.FileShare]::ReadWrite)
    try {
        $failedSave = (Invoke-Process $appExecutable $failureDirectory `
            ("1`nLOCK`nItem`n1,00`n2`n3`n0`n")
        ).Output
    }
    finally { $fileLock.Dispose() }
    Assert-Contains $failedSave 'Cadastro cancelado; estoque anterior restaurado.' `
        'rollback de cadastro apos falha de persistencia'
    Assert-Contains $failedSave 'Estoque vazio.' 'estoque em memoria revertido'
    if ($failedSave.Contains('Produto cadastrado e salvo com sucesso.')) {
        throw 'Uma gravacao recusada foi apresentada como sucesso.'
    }
    Write-Output 'OK: falha de substituicao recusa sucesso e reverte o estado em memoria.'
}
finally {
    if (Test-Path -LiteralPath $temporaryRoot) {
        Remove-Item -LiteralPath $temporaryRoot -Recurse -Force
    }
}
