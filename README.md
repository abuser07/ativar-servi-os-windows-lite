# Windows Lite Restore (Restaurador Windows Lite)

Ferramenta nativa para Windows 10/11 (x64) que restaura serviços, políticas de
registro, tarefas agendadas e recursos opcionais que imagens "Lite" costumam
desativar ou remover — sempre com revisão do usuário e usando os padrões de
referência da Microsoft.

Além da restauração, o aplicativo oferece **perfis de otimização** (Jogos, IA e
Escritório) que desativam ou colocam em manual apenas os serviços desnecessários
para cada situação. Toda alteração — ativar **ou** desativar — passa por revisão,
simulação, relatório e backup reversível.

---

## Recursos

- Interface gráfica em português, sem dependências externas (Win32 nativo).
- Abas: **Recursos**, **Serviços**, **Políticas**, **Tarefas**, **Diagnóstico** e **Registro**.
- Seletor de **perfil** (Padrão Microsoft, Jogos, IA, Escritório).
- Seleção individual por checkbox, com busca, filtro por categoria e perfis
  ("Recomendados", "Todos pendentes", "Limpar").
- **Simulação (dry-run)** para ver exatamente o que será alterado antes de aplicar.
- **Ponto de restauração do sistema** e **backup reversível** (serviços, políticas
  e tarefas) antes de aplicar.
- **Desfazer última execução** (menu Diagnóstico).
- Relatórios em texto e registro (log) detalhado.
- Auto-elevação via UAC (`runas`) com preservação da seleção.

### Perfis de otimização

| Perfil | O que faz |
|--------|-----------|
| **Padrão Microsoft** | Restaura serviços, políticas, tarefas e recursos para o padrão de uma instalação completa. |
| **Jogos** | Desativa telemetria, relatórios e entrega em segundo plano; coloca em manual SysMain, Windows Search, impressão, notificações etc. |
| **Inteligência Artificial** | O mesmo de Jogos, e ainda desativa os serviços Xbox; libera CPU/RAM/disco para cargas de IA. |
| **Escritório** | Desativa apenas jogos (Xbox) e telemetria; mantém impressão, pesquisa e atualizações. |

Os perfis de otimização **nunca alteram serviços essenciais do sistema** (inicialização,
logon, segurança e rede básica). Esses serviços aparecem marcados como
**"Essencial (protegido)"** e não podem ser desativados — evitando que uma
otimização impeça o logon ou a inicialização do Windows. Os demais serviços não
listados no perfil aparecem como "Sem alteração".

### O que é restaurado

| Categoria | Exemplos |
|-----------|----------|
| Serviços | Windows Update, BITS, Store/AppX, áudio, temas, segurança, rede |
| Políticas de registro | UAC, telemetria desativada por política, bloqueios de serviços |
| Tarefas agendadas | Manutenção, Windows Update, diagnóstico |
| Recursos opcionais | .NET Framework 3.5, recursos de mídia/idioma, ferramentas |

O catálogo completo fica em `src/catalog.cpp` (com descrição, categoria, tipo de
inicialização de referência e se o item é recomendado/crítico/opcional).

---

## Requisitos

- Windows 10 ou Windows 11, **64 bits**.
- Nenhum runtime, framework ou biblioteca adicional: o executável é compilado
  estaticamente (MinGW-w64) e roda sem instalação.

> Para **instalar recursos opcionais removidos**, pode ser necessária uma
> mídia/ISO de instalação do Windows com a mesma edição/arquitetura (pasta de
> origem configurável em *Configurações*).

---

## Uso

1. Execute `WindowsLiteRestore.exe`.
2. Se o Windows pedir elevação (UAC), aceite — muitas alterações exigem
   privilégios de administrador. Sem elevação o aplicativo funciona em **modo
   somente leitura**.
3. A análise é executada automaticamente. Escolha o **perfil** desejado no
   rodapé (Padrão Microsoft, Jogos, IA ou Escritório) e revise os itens marcados.
4. Opcionalmente marque **Simular** para validar o plano sem alterar nada.
5. Clique em **Aplicar selecionados** (itens marcados) ou **Aplicar perfil** /
   **Restaurar padrão Microsoft** (recomendados do perfil).
6. Confirme o plano e escolha se deseja criar ponto de restauração e backup.
7. Para reverter serviços/políticas/tarefas, use *Diagnóstico → Desfazer última
   execução*.

### Opções de linha de comando

| Opção | Descrição |
|-------|-----------|
| `--scan-report <arquivo>` | Gera um relatório de análise e sai (modo sem interface). |
| `--profile <nome>` | Perfil inicial: `padrao`/`default`, `jogos`/`gaming`, `ia`/`ai`, `escritorio`/`office`. |
| `--no-elevate` / `--read-only` | Inicia sem solicitar elevação (somente leitura). |
| `--elevated` | Indica que o processo já está elevado (uso interno). |
| `--resume-selection <arquivo>` | Restaura a seleção salva (uso interno). |

Exemplo:

```powershell
WindowsLiteRestore.exe --scan-report relatorio.txt --profile jogos
```

---

## Compilação

O único requisito de build é um compilador **MinGW-w64** (recomendado
[w64devkit](https://github.com/skeeto/w64devkit), que inclui `g++` e `windres`).

```powershell
# Opção A: definir o caminho do toolchain e compilar
$env:W64DEVKIT = "C:\caminho\para\w64devkit"
.\build.ps1

# Opção B: baixar o toolchain automaticamente (requer internet)
.\build.ps1 -DownloadToolchain
```

O script `build.ps1`:

1. Localiza `g++.exe`/`windres.exe` (via `$env:W64DEVKIT`, `tools\w64devkit` ou `PATH`).
2. Compila os recursos (`resources.rc`, manifesto, ícone, versão).
3. Compila `main.cpp`, `ui.cpp`, `core.cpp` e `catalog.cpp`.
4. Vincula estaticamente e grava `dist\WindowsLiteRestore.exe`, o `README.md` e o
   `SHA256.txt`.

O ícone pode ser regenerado com `tools\make-icon.ps1` (usa System.Drawing, sem
dependências externas).

### Estrutura do projeto

```
src/
  app.h            tipos, modelo, classe App, contratos entre módulos
  catalog.cpp      catálogo de serviços, features, políticas e tarefas
  core.cpp         scan, apply, backup, undo, relatório, elevação, DISM
  ui.cpp           interface gráfica (Win32 nativo)
  main.cpp         entrypoint (wWinMain)
  resources.rc     diálogos, ícone, manifesto, informações de versão
  resource.h       IDs de recursos/comandos
  app.manifest     asInvoker, DPI aware, controles comuns v6
dist/              executável final + README + SHA256
tools/             make-icon.ps1
```

---

## Segurança e limitações

- **Padrões de referência** correspondem a uma instalação completa do Windows 10/11.
  Edições, builds e hardware podem variar; revise antes de aplicar.
- **Perfis de otimização desativam serviços** (telemetria, Xbox, etc.) e colocam
  outros em manual. Eles **não** alteram serviços críticos ou de segurança, e
  tudo pode ser revertido pelo backup/Desfazer.
- **Serviços ausentes não são "instalados"** pelo aplicativo — a imagem Lite pode
  ter removido o componente de origem. Nesses casos o item aparece como
  indisponível.
- **Recursos removidos** podem exigir mídia/ISO compatível (erros comuns do DISM:
  `0x800F081F`, `0x800F0954`, `0x800F0906` — informe a pasta de origem).
- **Recursos opcionais não são revertidos** pelo "Desfazer", para evitar remoção
  de componentes que poderia desestabilizar o sistema.
- **Alguns serviços são protegidos** (ex.: Windows Update Medic, Defender). Se a
  alteração falhar, verifique a proteção contra adulteração.
- O aplicativo é independente e **não possui vínculo com a Microsoft**. Use por
  sua conta e risco, de preferência com um ponto de restauração e um backup.

### Onde ficam os dados

| Dado | Local |
|------|-------|
| Configurações | `%LOCALAPPDATA%\WindowsLiteRestore\settings.ini` |
| Logs (modo leitura) | `%LOCALAPPDATA%\WindowsLiteRestore\logs` |
| Logs (elevado) | `%PROGRAMDATA%\WindowsLiteRestore\logs` |
| Backups | `%PROGRAMDATA%\WindowsLiteRestore\Backups` |
| Relatórios | `%PROGRAMDATA%\WindowsLiteRestore\reports` |

---

## Aviso

Este software modifica configurações do sistema e instala componentes do Windows.
Sempre crie um ponto de restauração e um backup reversível antes de aplicar, e
faça uma simulação para revisar o plano.
