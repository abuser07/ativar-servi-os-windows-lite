#include "app.h"

#include <cwchar>
#include <algorithm>

// ---------------------------------------------------------------------------
// Serviços
//
// Os tipos de inicialização abaixo representam o padrão de referência do
// Windows 10/11 em uma instalação completa. Eles podem variar conforme a
// edição, a build e o hardware. O aplicativo nunca desativa um serviço; ele
// apenas restaura o tipo de inicialização indicado pelo usuário.
// ---------------------------------------------------------------------------

const ServiceDef g_serviceCatalog[] = {
    // Atualizações, Store e componentes sob demanda
    { L"wuauserv", L"Windows Update", L"Atualizações",
      L"Serviço central do Windows Update. Sem ele, o sistema não baixa atualizações e alguns recursos sob demanda não podem ser instalados.",
      SERVICE_DEMAND_START, false, true, true, false },
    { L"BITS", L"Transferência Inteligente em Segundo Plano (BITS)", L"Atualizações",
      L"Usado pelo Windows Update, Microsoft Store e aplicativos para transferir arquivos em segundo plano.",
      SERVICE_DEMAND_START, false, true, false, false },
    { L"UsoSvc", L"Orquestrador de Atualizações", L"Atualizações",
      L"Coordena a instalação de atualizações e a reinicialização do Windows.",
      SERVICE_AUTO_START, true, true, true, false },
    { L"DoSvc", L"Otimização de Entrega", L"Atualizações",
      L"Distribui atualizações e pacotes da Store entre computadores e para a Internet.",
      SERVICE_AUTO_START, true, true, false, false },
    { L"WaaSMedicSvc", L"Windows Update Medic", L"Atualizações",
      L"Repara componentes do Windows Update quando eles são danificados. É um serviço protegido do sistema.",
      SERVICE_DEMAND_START, false, true, true, false },
    { L"CryptSvc", L"Serviços Criptográficos", L"Atualizações",
      L"Assina e verifica assinaturas de atualizações e fornece serviços de criptografia para o Windows.",
      SERVICE_AUTO_START, false, true, true, false },
    { L"TrustedInstaller", L"Instalador de Módulos do Windows", L"Atualizações",
      L"Instala, repara e remove componentes e atualizações do Windows. É iniciado sob demanda pelo DISM/Windows Update.",
      SERVICE_DEMAND_START, false, true, true, false },
    { L"msiserver", L"Windows Installer", L"Atualizações",
      L"Necessário para instalar, reparar e remover programas MSI. É iniciado sob demanda.",
      SERVICE_DEMAND_START, false, true, false, false },
    { L"InstallService", L"Serviço de Instalação da Microsoft Store", L"Loja e aplicativos",
      L"Instala e atualiza aplicativos da Microsoft Store. Sem ele, a Store pode falhar.",
      SERVICE_DEMAND_START, false, true, false, false },
    { L"AppXSvc", L"Implantação AppX (AppXSVC)", L"Loja e aplicativos",
      L"Registra e gerencia aplicativos UWP/MSIX. Em geral é iniciado automaticamente no Windows 11.",
      SERVICE_AUTO_START, false, true, false, false },
    { L"StateRepository", L"Repositório de Estado", L"Loja e aplicativos",
      L"Banco de dados de estado usado por aplicativos da Store, pacotes e pela interface moderna do Windows.",
      SERVICE_AUTO_START, false, true, false, false },
    { L"ClipSVC", L"Serviço de Licenças de Cliente", L"Loja e aplicativos",
      L"Gerencia licenças de aplicativos e conteúdo da Microsoft Store.",
      SERVICE_DEMAND_START, false, true, false, false },
    { L"LicenseManager", L"Gerenciador de Licenças do Windows", L"Loja e aplicativos",
      L"Gerencia licenças do Windows e de aplicativos. É iniciado sob demanda.",
      SERVICE_DEMAND_START, false, true, false, false },
    { L"TokenBroker", L"Gerenciador de Conta da Web", L"Loja e aplicativos",
      L"Fornece tokens de conta da Web para aplicativos da Store, Office e serviços conectados.",
      SERVICE_DEMAND_START, false, true, false, false },
    { L"UserManager", L"Gerenciador de Usuários", L"Loja e aplicativos",
      L"Dá suporte a múltiplos usuários e a aplicativos que precisam de informações de conta.",
      SERVICE_AUTO_START, false, true, false, false },
    { L"CDPSvc", L"Plataforma de Dispositivos Conectados", L"Loja e aplicativos",
      L"Permite descoberta e conexão com dispositivos e experiências entre dispositivos.",
      SERVICE_DEMAND_START, true, true, false, false },
    { L"WpnService", L"Notificações Push do Windows", L"Loja e aplicativos",
      L"Entrega notificações push para aplicativos e para a Central de Ações.",
      SERVICE_AUTO_START, false, true, false, false },
    { L"AppReadiness", L"Preparação de Aplicativos", L"Loja e aplicativos",
      L"Prepara o sistema para instalar e atualizar aplicativos. É iniciado sob demanda.",
      SERVICE_DEMAND_START, false, true, false, false },
    { L"FontCache", L"Cache de Fontes do Windows", L"Interface e áudio",
      L"Armazena em cache fontes usadas pela interface. Desativá-lo deixa a renderização de texto mais lenta.",
      SERVICE_AUTO_START, false, true, false, false },
    { L"Themes", L"Temas", L"Interface e áudio",
      L"Gerencia temas visuais, cores e efeitos da interface do Windows.",
      SERVICE_AUTO_START, false, true, false, false },
    { L"Audiosrv", L"Áudio do Windows", L"Interface e áudio",
      L"Gerencia dispositivos de áudio e o mixer do Windows.",
      SERVICE_AUTO_START, false, true, true, false },
    { L"AudioEndpointBuilder", L"Construtor de Pontos de Extremidade de Áudio", L"Interface e áudio",
      L"Detecta e configura dispositivos de áudio. Depende do serviço de áudio.",
      SERVICE_AUTO_START, false, true, true, false },

    // Segurança
    { L"WinDefend", L"Microsoft Defender Antivírus", L"Segurança",
      L"Serviço principal do Microsoft Defender. Pode ser protegido contra alterações; desative temporariamente a proteção contra adulteração se precisar repará-lo.",
      SERVICE_AUTO_START, false, true, true, false },
    { L"WdNisSvc", L"Inspeção de Rede do Microsoft Defender", L"Segurança",
      L"Inspeção de tráfego de rede do Defender. É iniciado sob demanda.",
      SERVICE_DEMAND_START, false, true, false, false },
    { L"MDCoreSvc", L"Microsoft Defender Core", L"Segurança",
      L"Serviço de núcleo do Microsoft Defender em versões recentes do Windows 11.",
      SERVICE_AUTO_START, false, true, false, false },
    { L"SecurityHealthService", L"Serviço de Segurança do Windows", L"Segurança",
      L"Exibe o estado da Segurança do Windows e integra Defender, firewall e proteção do dispositivo.",
      SERVICE_DEMAND_START, false, true, false, false },
    { L"wscsvc", L"Central de Segurança", L"Segurança",
      L"Monitora o estado de antivírus, firewall e proteção do sistema; alimenta as notificações da Segurança do Windows.",
      SERVICE_AUTO_START, true, true, true, false },
    { L"MpsSvc", L"Firewall do Windows Defender", L"Segurança",
      L"Firewall do Windows. Sem ele, o sistema fica exposto na rede.",
      SERVICE_AUTO_START, false, true, true, false },
    { L"BFE", L"Mecanismo de Filtragem Básica", L"Segurança",
      L"Base do firewall do Windows e da filtragem de tráfego. É obrigatório para o firewall funcionar.",
      SERVICE_AUTO_START, false, true, true, false },
    { L"SgrmBroker", L"System Guard Runtime Monitor", L"Segurança",
      L"Monitora a integridade do sistema e protege recursos de segurança em tempo de execução.",
      SERVICE_AUTO_START, true, true, false, false },
    { L"Sense", L"Defender para Endpoint (Sense)", L"Segurança",
      L"Serviço de proteção avançada contra ameaças do Microsoft Defender. É iniciado sob demanda.",
      SERVICE_DEMAND_START, false, true, false, true },
    { L"AppIDSvc", L"Identidade de Aplicativo", L"Segurança",
      L"Usado pelo AppLocker e por políticas de controle de aplicativos.",
      SERVICE_DEMAND_START, false, true, false, true },
    { L"KeyIso", L"Isolamento de Chave CNG", L"Segurança",
      L"Isola chaves criptográficas usadas por credenciais, navegadores e aplicativos.",
      SERVICE_DEMAND_START, false, true, false, false },
    { L"VaultSvc", L"Gerenciador de Credenciais", L"Segurança",
      L"Armazena e fornece credenciais salvas do Windows.",
      SERVICE_DEMAND_START, false, true, false, false },
    { L"SamSs", L"Gerenciador de Contas de Segurança", L"Segurança",
      L"Gerencia contas locais e tokens de segurança. É essencial para o logon.",
      SERVICE_AUTO_START, false, true, true, false },
    { L"CertPropSvc", L"Propagação de Certificado", L"Segurança",
      L"Propaga certificados de cartões inteligentes e de usuário. É iniciado sob demanda.",
      SERVICE_DEMAND_START, false, true, false, false },
    { L"EFS", L"Sistema de Arquivos Criptografados (EFS)", L"Segurança",
      L"Fornece criptografia de arquivos EFS. É iniciado sob demanda.",
      SERVICE_DEMAND_START, false, true, false, false },
    { L"BDESVC", L"Criptografia de Unidade BitLocker", L"Segurança",
      L"Gerencia o BitLocker e a proteção de unidades. É iniciado sob demanda.",
      SERVICE_DEMAND_START, false, true, false, false },
    { L"PolicyAgent", L"Agente de Política IPsec", L"Segurança",
      L"Aplica políticas de segurança IPsec e regras de firewall corporativas.",
      SERVICE_DEMAND_START, false, true, false, false },
    { L"IKEEXT", L"IKE e AuthIP para IPsec", L"Segurança",
      L"Gerencia chaves de segurança para conexões IPsec.",
      SERVICE_AUTO_START, false, true, false, false },
    { L"EapHost", L"Protocolo de Autenticação Extensível", L"Segurança",
      L"Autenticação 802.1X em redes com fio e sem fio.",
      SERVICE_DEMAND_START, false, true, false, false },
    { L"dot3svc", L"Configuração Automática com Fio", L"Segurança",
      L"Autenticação e configuração de redes com fio 802.1X.",
      SERVICE_DEMAND_START, false, true, false, false },

    // Rede
    { L"Dnscache", L"Cliente DNS", L"Rede",
      L"Resolve nomes DNS e mantém cache. Sem ele, a navegação e a rede podem falhar.",
      SERVICE_AUTO_START, false, true, true, false },
    { L"Dhcp", L"Cliente DHCP", L"Rede",
      L"Obtém endereço IP e configurações de rede automaticamente.",
      SERVICE_AUTO_START, false, true, true, false },
    { L"NlaSvc", L"Reconhecimento de Locais de Rede", L"Rede",
      L"Identifica o tipo de rede (doméstica, corporativa, pública) e aplica o perfil adequado.",
      SERVICE_AUTO_START, false, true, false, false },
    { L"netprofm", L"Serviço da Lista de Redes", L"Rede",
      L"Mantém a lista de redes disponíveis e conectadas.",
      SERVICE_DEMAND_START, false, true, false, false },
    { L"nsi", L"Serviço de Interface de Repositório de Rede", L"Rede",
      L"Fornece informações de configuração de rede para aplicativos e serviços.",
      SERVICE_AUTO_START, false, true, false, false },
    { L"Netman", L"Conexões de Rede", L"Rede",
      L"Gerencia adaptadores, conexões dial-up e VPN. É iniciado sob demanda.",
      SERVICE_DEMAND_START, false, true, false, false },
    { L"NcaSvc", L"Assistente de Conectividade de Rede", L"Rede",
      L"Ajuda a diagnosticar e corrigir problemas de conectividade.",
      SERVICE_DEMAND_START, false, true, false, false },
    { L"NetSetupSvc", L"Serviço de Configuração de Rede", L"Rede",
      L"Permite instalar e configurar adaptadores e drivers de rede.",
      SERVICE_DEMAND_START, false, true, false, false },
    { L"Wcmsvc", L"Gerenciador de Conexões do Windows", L"Rede",
      L"Gerencia conexões de rede e escolhe a melhor conexão disponível.",
      SERVICE_AUTO_START, false, true, false, false },
    { L"WinHttpAutoProxySvc", L"Descoberta Automática de Proxy WinHTTP", L"Rede",
      L"Descobre configurações de proxy para aplicativos que usam WinHTTP.",
      SERVICE_DEMAND_START, false, true, false, false },
    { L"iphlpsvc", L"Auxiliar de IP", L"Rede",
      L"Fornece túneis IPv6, Teredo e funcionalidades de transição de IP.",
      SERVICE_AUTO_START, false, true, false, false },
    { L"WlanSvc", L"Configuração Automática de WLAN", L"Rede",
      L"Gerencia redes Wi-Fi, perfis e conexões sem fio. É essencial em notebooks.",
      SERVICE_DEMAND_START, false, true, false, false },
    { L"WwanSvc", L"Configuração Automática de WWAN", L"Rede",
      L"Gerencia redes móveis (3G/4G/5G) em dispositivos com modem.",
      SERVICE_DEMAND_START, false, true, false, false },
    { L"NcbService", L"Agente de Conexão de Rede", L"Rede",
      L"Notifica aplicativos sobre mudanças de conectividade.",
      SERVICE_DEMAND_START, false, true, false, false },
    { L"SharedAccess", L"Compartilhamento de Conexão com a Internet (ICS)", L"Rede",
      L"Compartilha a conexão de Internet e oferece NAT simples. É iniciado sob demanda.",
      SERVICE_DEMAND_START, false, true, false, false },
    { L"icssvc", L"Hotspot Móvel do Windows", L"Rede",
      L"Gerencia o ponto de acesso móvel do Windows.",
      SERVICE_DEMAND_START, false, true, false, false },
    { L"ALG", L"Gateway de Camada de Aplicativo", L"Rede",
      L"Ajuda aplicativos antigos a atravessar NAT e firewalls. É iniciado sob demanda.",
      SERVICE_DEMAND_START, false, true, false, false },
    { L"TapiSrv", L"Telefonia", L"Rede",
      L"API de telefonia do Windows, usada por aplicativos de fax, chamadas e modems.",
      SERVICE_DEMAND_START, false, true, false, false },
    { L"RmSvc", L"Gerenciamento de Rádio", L"Rede",
      L"Gerencia rádios de Wi-Fi, Bluetooth e banda larga móvel. É iniciado sob demanda.",
      SERVICE_DEMAND_START, false, true, false, true },
    { L"bthserv", L"Suporte a Bluetooth", L"Rede",
      L"Gerencia adaptadores e dispositivos Bluetooth. É iniciado sob demanda.",
      SERVICE_DEMAND_START, false, true, false, false },
    { L"BTAGService", L"Gateway de Áudio Bluetooth", L"Rede",
      L"Permite usar fones e caixas de som Bluetooth.",
      SERVICE_DEMAND_START, false, true, false, false },
    { L"BthAvctpSvc", L"Serviço AVCTP", L"Rede",
      L"Controla perfis de áudio e vídeo Bluetooth (A2DP/AVRCP).",
      SERVICE_DEMAND_START, false, true, false, false },
    { L"LanmanWorkstation", L"Estação de Trabalho", L"Rede",
      L"Acesso a compartilhamentos SMB e impressoras de rede.",
      SERVICE_AUTO_START, false, true, true, false },
    { L"LanmanServer", L"Servidor", L"Rede",
      L"Compartilha arquivos e impressoras deste computador pela rede.",
      SERVICE_AUTO_START, false, true, true, false },
    { L"WebClient", L"Cliente Web", L"Rede",
      L"Permite acessar arquivos e pastas via WebDAV. É iniciado sob demanda.",
      SERVICE_DEMAND_START, false, true, false, false },

    // Dispositivos, impressão, áudio e imagem
    { L"Spooler", L"Spooler de Impressão", L"Impressão",
      L"Gerencia filas e impressoras. Sem ele, não é possível imprimir nem adicionar impressoras.",
      SERVICE_AUTO_START, false, true, true, false },
    { L"PrintNotify", L"Extensões e Notificações da Impressora", L"Impressão",
      L"Notifica aplicativos sobre eventos de impressão e impressoras.",
      SERVICE_DEMAND_START, false, true, false, false },
    { L"StiSvc", L"Aquisição de Imagens do Windows (WIA)", L"Dispositivos",
      L"Suporte a scanners e câmeras que usam WIA.",
      SERVICE_AUTO_START, false, true, false, false },
    { L"WiaRpc", L"Eventos de Aquisição de Imagens", L"Dispositivos",
      L"Notifica aplicativos sobre eventos de scanners e câmeras.",
      SERVICE_DEMAND_START, false, true, false, false },
    { L"FrameServer", L"Servidor de Quadros de Câmera", L"Dispositivos",
      L"Compartilha a câmera entre aplicativos e gerencia captura de vídeo.",
      SERVICE_DEMAND_START, false, true, false, false },
    { L"WbioSrvc", L"Biometria do Windows", L"Dispositivos",
      L"Suporte a leitores de impressão digital e reconhecimento facial.",
      SERVICE_DEMAND_START, false, true, false, true },
    { L"NgcSvc", L"Microsoft Passport", L"Dispositivos",
      L"Gerencia credenciais do Windows Hello e PIN.",
      SERVICE_DEMAND_START, false, true, false, false },
    { L"NgcCtnrSvc", L"Contêiner do Microsoft Passport", L"Dispositivos",
      L"Armazena credenciais do Windows Hello em contêiner seguro.",
      SERVICE_DEMAND_START, false, true, false, false },
    { L"TabletInputService", L"Serviço de Entrada de Tablet", L"Dispositivos",
      L"Suporte a caneta, toque e teclado virtual.",
      SERVICE_DEMAND_START, false, true, false, false },
    { L"TextInputManagementService", L"Gerenciamento de Entrada de Texto", L"Dispositivos",
      L"Gerencia teclados, métodos de entrada e experiência de digitação no Windows 11.",
      SERVICE_AUTO_START, false, true, false, false },
    { L"DeviceAssociationService", L"Associação de Dispositivo", L"Dispositivos",
      L"Associa dispositivos e sensores ao sistema. É iniciado sob demanda.",
      SERVICE_DEMAND_START, false, true, false, false },
    { L"DeviceInstall", L"Instalação de Dispositivo", L"Dispositivos",
      L"Instala drivers e dispositivos quando conectados. É iniciado sob demanda.",
      SERVICE_DEMAND_START, false, true, false, false },
    { L"DsmSvc", L"Gerenciador de Instalação de Dispositivo", L"Dispositivos",
      L"Coordena instalação e configuração de dispositivos. É iniciado sob demanda.",
      SERVICE_DEMAND_START, false, true, false, false },
    { L"PlugPlay", L"Plug and Play", L"Dispositivos",
      L"Detecta hardware e carrega drivers. Essencial para dispositivos.",
      SERVICE_DEMAND_START, false, true, true, false },
    { L"ShellHWDetection", L"Detecção de Hardware do Shell", L"Dispositivos",
      L"Notifica o Windows sobre dispositivos removíveis e mídia inserida.",
      SERVICE_AUTO_START, false, true, false, false },

    // Sistema, manutenção e diagnóstico
    { L"EventLog", L"Log de Eventos do Windows", L"Sistema",
      L"Registra eventos do sistema e dos aplicativos. Essencial para diagnóstico e segurança.",
      SERVICE_AUTO_START, false, true, true, false },
    { L"Winmgmt", L"Windows Management Instrumentation (WMI)", L"Sistema",
      L"Infraestrutura de gerenciamento usada por ferramentas, inventário e scripts.",
      SERVICE_AUTO_START, false, true, true, false },
    { L"Schedule", L"Agendador de Tarefas", L"Sistema",
      L"Executa tarefas agendadas do Windows e de aplicativos.",
      SERVICE_AUTO_START, false, true, true, false },
    { L"ProfSvc", L"Serviço de Perfil de Usuário", L"Sistema",
      L"Carrega e salva perfis de usuário. Essencial para logon.",
      SERVICE_AUTO_START, false, true, true, false },
    { L"SENS", L"Notificação de Eventos do Sistema", L"Sistema",
      L"Distribui eventos do sistema para aplicativos e serviços.",
      SERVICE_AUTO_START, false, true, false, false },
    { L"EventSystem", L"COM+ Event System", L"Sistema",
      L"Gerencia eventos COM+ e notificações de sistema.",
      SERVICE_AUTO_START, false, true, false, false },
    { L"SystemEventsBroker", L"Agente de Eventos do Sistema", L"Sistema",
      L"Coordena eventos em segundo plano e tarefas do sistema.",
      SERVICE_AUTO_START, false, true, true, false },
    { L"BrokerInfrastructure", L"Infraestrutura de Tarefas em Segundo Plano", L"Sistema",
      L"Infraestrutura para tarefas em segundo plano de aplicativos da Store e do sistema.",
      SERVICE_AUTO_START, false, true, true, false },
    { L"Power", L"Energia", L"Sistema",
      L"Gerencia políticas de energia, suspensão e bateria.",
      SERVICE_AUTO_START, false, true, true, false },
    { L"TimeBrokerSvc", L"Agente de Tempo", L"Sistema",
      L"Fornece eventos de tempo para tarefas e aplicativos.",
      SERVICE_DEMAND_START, false, true, false, false },
    { L"SysMain", L"SysMain", L"Sistema",
      L"Pré-carrega aplicativos e gerencia memória. Em SSDs modernos o impacto é pequeno, mas o padrão do Windows é automático.",
      SERVICE_AUTO_START, false, true, false, false },
    { L"W32Time", L"Horário do Windows", L"Sistema",
      L"Sincroniza o relógio com servidores de horário. O padrão é manual e ele é iniciado sob demanda.",
      SERVICE_DEMAND_START, false, true, false, false },
    { L"seclogon", L"Logon Secundário", L"Sistema",
      L"Permite executar programas como outro usuário e é usado por alguns instaladores.",
      SERVICE_DEMAND_START, false, true, false, false },
    { L"Appinfo", L"Informações sobre Aplicativos", L"Sistema",
      L"Suporte a elevação de privilégios (UAC) e execução de aplicativos como administrador.",
      SERVICE_DEMAND_START, false, true, true, false },
    { L"VSS", L"Cópia de Sombra de Volume", L"Sistema",
      L"Cria cópias de sombra usadas por backup, restauração e pontos de restauração.",
      SERVICE_DEMAND_START, false, true, false, false },
    { L"swprv", L"Provedor de Cópia de Sombra de Software", L"Sistema",
      L"Gerencia cópias de sombra de software para backup e restauração.",
      SERVICE_DEMAND_START, false, true, false, false },
    { L"SDRSVC", L"Backup do Windows", L"Sistema",
      L"Executa backups e restaurações do Windows. É iniciado sob demanda.",
      SERVICE_DEMAND_START, false, true, false, false },
    { L"wbengine", L"Backup em Nível de Bloco", L"Sistema",
      L"Motor de backup completo e de imagem do sistema. É iniciado sob demanda.",
      SERVICE_DEMAND_START, false, true, false, false },
    { L"WerSvc", L"Relatório de Erros do Windows", L"Sistema",
      L"Coleta relatórios de falhas para diagnóstico. É iniciado sob demanda.",
      SERVICE_DEMAND_START, false, true, false, true },
    { L"WSearch", L"Windows Search", L"Pesquisa",
      L"Indexa arquivos, e-mails e conteúdo para a pesquisa do Windows e do Outlook.",
      SERVICE_AUTO_START, true, true, false, false },

    // Área de trabalho remota e gerenciamento remoto
    { L"TermService", L"Serviços de Área de Trabalho Remota", L"Área de trabalho remota",
      L"Permite conexões de Área de Trabalho Remota. O padrão é manual; a RDP só funciona quando habilitada.",
      SERVICE_DEMAND_START, false, true, false, false },
    { L"UmRdpService", L"Redirecionador de Portas RDP", L"Área de trabalho remota",
      L"Redireciona impressoras, unidades e dispositivos em sessões RDP.",
      SERVICE_DEMAND_START, false, true, false, false },
    { L"SessionEnv", L"Configuração da Área de Trabalho Remota", L"Área de trabalho remota",
      L"Gerencia configurações de sessão RDP. É iniciado sob demanda.",
      SERVICE_DEMAND_START, false, true, false, false },
    { L"WinRM", L"Windows Remote Management (WinRM)", L"Gerenciamento remoto",
      L"Permite gerenciamento remoto via WS-Management/PowerShell Remoting. O padrão é manual; não é iniciado automaticamente.",
      SERVICE_DEMAND_START, false, false, false, true },

    // Jogos
    { L"XblAuthManager", L"Autenticação Xbox Live", L"Jogos",
      L"Autenticação de conta Xbox Live para jogos e aplicativos Xbox.",
      SERVICE_DEMAND_START, false, false, false, true },
    { L"XblGameSave", L"Salvar Jogos no Xbox Live", L"Jogos",
      L"Sincroniza saves de jogos com o Xbox Live.",
      SERVICE_DEMAND_START, false, false, false, true },
    { L"XboxGipSvc", L"Gerenciamento de Acessórios Xbox", L"Jogos",
      L"Suporte a controles e acessórios Xbox.",
      SERVICE_DEMAND_START, false, false, false, true },
    { L"XboxNetApiSvc", L"Rede Xbox Live", L"Jogos",
      L"Rede e recursos online de jogos Xbox.",
      SERVICE_DEMAND_START, false, false, false, true },

    // Diagnóstico e telemetria (opcionais)
    { L"DiagTrack", L"Experiências do Usuário Conectado e Telemetria", L"Diagnóstico e telemetria",
      L"Coleta dados de diagnóstico e telemetria. O padrão do Windows é automático, mas o item é opcional por privacidade.",
      SERVICE_DEMAND_START, false, false, false, true },
    { L"dmwappushservice", L"Roteamento de Mensagens Push WAP", L"Diagnóstico e telemetria",
      L"Suporte a gerenciamento de dispositivos móveis. O padrão é manual.",
      SERVICE_DEMAND_START, false, false, false, true },
};

const size_t g_serviceCatalogCount = _countof(g_serviceCatalog);

// ---------------------------------------------------------------------------
// Perfis de otimização de serviços
//
// Cada perfil altera SOMENTE os serviços listados abaixo; os demais mantêm o
// padrão Microsoft. Os perfis nunca alteram serviços críticos do sistema nem
// serviços de segurança.
// ---------------------------------------------------------------------------

struct ProfileServiceRule {
    const wchar_t* name;
    DWORD start;    // SERVICE_DISABLED ou SERVICE_DEMAND_START
    bool delayed;
};

// Jogos: reduz atividade em segundo plano para liberar CPU, memória e rede.
const ProfileServiceRule g_gamingServiceRules[] = {
    // Telemetria e relatórios
    { L"DiagTrack", SERVICE_DISABLED, false },
    { L"dmwappushservice", SERVICE_DISABLED, false },
    { L"WerSvc", SERVICE_DISABLED, false },
    // Atualizações em segundo plano (evita interrupções e uso de rede)
    { L"DoSvc", SERVICE_DISABLED, false },
    { L"UsoSvc", SERVICE_DEMAND_START, false },
    // Memória e indexação
    { L"SysMain", SERVICE_DEMAND_START, false },
    { L"WSearch", SERVICE_DEMAND_START, false },
    // Itens não essenciais durante a sessão de jogo
    { L"WpnService", SERVICE_DEMAND_START, false },
    { L"Spooler", SERVICE_DEMAND_START, false },
    { L"StiSvc", SERVICE_DEMAND_START, false },
    { L"TextInputManagementService", SERVICE_DEMAND_START, false },
    { L"ShellHWDetection", SERVICE_DEMAND_START, false },
};

// IA / cargas intensas (treinamento, inferência, GPU): máximo de recursos
// para o processamento, mantendo rede e segurança.
const ProfileServiceRule g_aiServiceRules[] = {
    // Telemetria e relatórios
    { L"DiagTrack", SERVICE_DISABLED, false },
    { L"dmwappushservice", SERVICE_DISABLED, false },
    { L"WerSvc", SERVICE_DISABLED, false },
    // Entrega de atualizações e orquestração em segundo plano
    { L"DoSvc", SERVICE_DISABLED, false },
    { L"UsoSvc", SERVICE_DEMAND_START, false },
    // Jogos/Xbox não são necessários para IA
    { L"XblAuthManager", SERVICE_DISABLED, false },
    { L"XblGameSave", SERVICE_DISABLED, false },
    { L"XboxGipSvc", SERVICE_DISABLED, false },
    { L"XboxNetApiSvc", SERVICE_DISABLED, false },
    // Memória e indexação
    { L"SysMain", SERVICE_DEMAND_START, false },
    { L"WSearch", SERVICE_DEMAND_START, false },
    // Itens não essenciais
    { L"WpnService", SERVICE_DEMAND_START, false },
    { L"Spooler", SERVICE_DEMAND_START, false },
    { L"StiSvc", SERVICE_DEMAND_START, false },
    { L"TextInputManagementService", SERVICE_DEMAND_START, false },
    { L"ShellHWDetection", SERVICE_DEMAND_START, false },
};

// Escritório: mantém produtividade (impressão, pesquisa, atualizações) e
// desativa apenas entretenimento, jogos e telemetria.
const ProfileServiceRule g_officeServiceRules[] = {
    // Telemetria e relatórios
    { L"DiagTrack", SERVICE_DISABLED, false },
    { L"dmwappushservice", SERVICE_DISABLED, false },
    { L"WerSvc", SERVICE_DISABLED, false },
    // Jogos/Xbox não são necessários para trabalho de escritório
    { L"XblAuthManager", SERVICE_DISABLED, false },
    { L"XblGameSave", SERVICE_DISABLED, false },
    { L"XboxGipSvc", SERVICE_DISABLED, false },
    { L"XboxNetApiSvc", SERVICE_DISABLED, false },
    // Notificações e entrega em segundo plano (mantém sob demanda)
    { L"WpnService", SERVICE_DEMAND_START, false },
    { L"DoSvc", SERVICE_DEMAND_START, false },
};

const wchar_t* ProfileName(Profile profile) {
    switch (profile) {
        case Profile::MicrosoftDefault: return L"Padrão Microsoft";
        case Profile::Gaming: return L"Jogos";
        case Profile::AI: return L"Inteligência Artificial";
        case Profile::Office: return L"Escritório";
    }
    return L"Padrão Microsoft";
}

const wchar_t* ProfileDescription(Profile profile) {
    switch (profile) {
        case Profile::MicrosoftDefault:
            return L"Restaura serviços, políticas, tarefas e recursos para o padrão Microsoft.";
        case Profile::Gaming:
            return L"Desativa telemetria e serviços desnecessários para liberar CPU, memória e rede durante os jogos.";
        case Profile::AI:
            return L"Reduz atividade em segundo plano e desativa jogos/telemetria para dar o máximo de recursos a cargas de IA.";
        case Profile::Office:
            return L"Desativa jogos e telemetria, mantendo impressão, pesquisa e atualizações para produtividade.";
    }
    return L"";
}

bool GetProfileServiceTarget(const std::wstring& name, Profile profile, DWORD* start,
                             bool* delayed) {
    const ProfileServiceRule* rules = nullptr;
    size_t count = 0;
    switch (profile) {
        case Profile::Gaming:
            rules = g_gamingServiceRules;
            count = _countof(g_gamingServiceRules);
            break;
        case Profile::AI:
            rules = g_aiServiceRules;
            count = _countof(g_aiServiceRules);
            break;
        case Profile::Office:
            rules = g_officeServiceRules;
            count = _countof(g_officeServiceRules);
            break;
        default:
            return false;
    }
    for (size_t i = 0; i < count; ++i) {
        if (name == rules[i].name) {
            if (start) *start = rules[i].start;
            if (delayed) *delayed = rules[i].delayed;
            return true;
        }
    }
    return false;
}

// ---------------------------------------------------------------------------
// Recursos opcionais do Windows
// ---------------------------------------------------------------------------

const FeatureDef g_featureCatalog[] = {
    { L"MediaPlayback", L"Recursos de Mídia", L"Mídia",
      L"Codecs de áudio e vídeo do Windows. Necessário para reproduzir vários formatos em players e navegadores.",
      true, false },
    { L"WindowsMediaPlayer", L"Windows Media Player", L"Mídia",
      L"Reprodutor de mídia clássico do Windows. Em versões recentes pode aparecer como recurso opcional.",
      true, false },
    { L"NetFx4-AdvSrvs", L".NET Framework 4.x — Serviços Avançados", L"Compatibilidade",
      L"Serviços avançados do .NET Framework 4.x, incluindo WCF. Normalmente já vem habilitado.",
      true, false },
    { L"WCF-Services45", L"WCF Services 4.5", L"Compatibilidade",
      L"Ativação de serviços Windows Communication Foundation para .NET Framework 4.5.",
      true, false },
    { L"WCF-TCP-PortSharing45", L"Compartilhamento de Portas TCP (WCF)", L"Compatibilidade",
      L"Compartilhamento de portas TCP para serviços WCF.",
      true, false },
    { L"Printing-Foundation-Features", L"Serviços de Impressão e Documentos", L"Impressão",
      L"Base do sistema de impressão do Windows. Sem este recurso, impressoras podem não funcionar.",
      true, false },
    { L"Printing-Foundation-InternetPrinting-Client", L"Cliente de Impressão pela Internet", L"Impressão",
      L"Suporte a impressoras Web (IPP/Internet Printing).",
      true, false },
    { L"Printing-PrintToPDFServices-Features", L"Microsoft Print to PDF", L"Impressão",
      L"Impressora virtual que gera arquivos PDF.",
      true, false },
    { L"SearchEngine-Client-Package", L"Windows Search", L"Pesquisa",
      L"Componentes de pesquisa e indexação do Windows.",
      true, false },
    { L"MSRDC-Infrastructure", L"Compactação Diferencial Remota", L"Rede",
      L"Suporte à API de Compactação Diferencial Remota usada por transferências e atualizações.",
      true, false },
    { L"Microsoft-Windows-Camera-CCX-Deployment", L"Camera CCX Framework", L"Dispositivos",
      L"Estrutura de câmera usada por aplicativos e drivers de câmera modernos.",
      true, false },
    { L"WorkFolders-Client", L"Work Folders", L"Rede",
      L"Cliente para pastas de trabalho corporativas sincronizadas.",
      true, false },
    { L"Wireless-Networking", L"Rede Sem Fio", L"Rede",
      L"Suporte a adaptadores e perfis Wi-Fi. Presente por padrão em instalações com suporte sem fio.",
      true, false },
    { L"Internet-Explorer-Optional-amd64", L"Internet Explorer 11", L"Compatibilidade",
      L"Recurso legado do Windows 10 para aplicativos que dependem do motor do Internet Explorer. Não é recomendado para navegação.",
      true, false },

    // Recursos opcionais populares (não fazem parte do padrão; marcáveis manualmente)
    { L"NetFx3", L".NET Framework 3.5", L"Compatibilidade",
      L"Necessário para muitos aplicativos antigos. Não vem habilitado por padrão e pode exigir Windows Update ou mídia de instalação.",
      false, true },
    { L"DirectPlay", L"DirectPlay", L"Compatibilidade",
      L"API de jogos antiga. Instale apenas se um jogo ou aplicativo legado exigir.",
      false, true },
    { L"LegacyComponents", L"Componentes Herdados", L"Compatibilidade",
      L"Inclui DirectPlay e outros componentes legados. Pode ser necessário para jogos antigos.",
      false, true },
    { L"MicrosoftWindowsPowerShellV2Root", L"Windows PowerShell 2.0", L"Compatibilidade",
      L"Motor antigo do PowerShell. É inseguro e não é recomendado; instale apenas para compatibilidade específica.",
      false, true },
    { L"MicrosoftWindowsPowerShellV2", L"Windows PowerShell 2.0 — Módulo", L"Compatibilidade",
      L"Módulo do PowerShell 2.0. Prefira versões atuais do PowerShell.",
      false, true },
    { L"Microsoft-Hyper-V-All", L"Hyper-V", L"Virtualização",
      L"Plataforma de virtualização da Microsoft. Requer suporte de hardware e reinicialização.",
      false, true },
    { L"VirtualMachinePlatform", L"Plataforma de Máquina Virtual", L"Virtualização",
      L"Camada de virtualização usada pelo WSL 2, Windows Sandbox e máquinas virtuais.",
      false, true },
    { L"HypervisorPlatform", L"Plataforma do Hipervisor do Windows", L"Virtualização",
      L"API do hipervisor para virtualização de terceiros.",
      false, true },
    { L"Microsoft-Windows-Subsystem-Linux", L"Subsistema do Windows para Linux (WSL)", L"Desenvolvimento",
      L"Permite executar distribuições Linux no Windows.",
      false, true },
    { L"Containers-DisposableClientVM", L"Windows Sandbox", L"Virtualização",
      L"Área de trabalho descartável para executar aplicativos isolados. Requer virtualização.",
      false, true },
    { L"Containers", L"Containers", L"Virtualização",
      L"Suporte a contêineres do Windows.",
      false, true },
    { L"IIS-WebServerRole", L"Serviços de Informações da Internet (IIS)", L"Desenvolvimento",
      L"Servidor Web do Windows. Instale apenas se for desenvolver ou hospedar sites.",
      false, true },
    { L"OpenSSH.Client", L"Cliente OpenSSH", L"Desenvolvimento",
      L"Cliente SSH incluído no Windows para conexões remotas.",
      false, true },
    { L"SMB1Protocol", L"Protocolo SMB 1.0", L"Rede",
      L"Protocolo antigo e inseguro. Use apenas se um dispositivo legado exigir; não é recomendado.",
      false, true },
    { L"TelnetClient", L"Cliente Telnet", L"Rede",
      L"Cliente Telnet legado. Prefira SSH; o tráfego não é criptografado.",
      false, true },
    { L"TFTP", L"Cliente TFTP", L"Rede",
      L"Cliente de transferência trivial de arquivos. Usado por equipamentos de rede.",
      false, true },
    { L"SimpleTCP", L"Recursos Simples de TCP/IP", L"Rede",
      L"Utilitários TCP/IP legados (echo, chargen etc.).",
      false, true },
    { L"Printing-XPSServices-Features", L"Serviços XPS / Microsoft XPS Document Writer", L"Impressão",
      L"Suporte a documentos XPS e impressora virtual XPS.",
      false, true },
    { L"FaxServicesClientPackage", L"Fax e Digitalização do Windows", L"Impressão",
      L"Cliente de fax e digitalização do Windows.",
      false, true },
    { L"Windows-Photo-Viewer", L"Visualizador de Fotos do Windows", L"Mídia",
      L"Visualizador de fotos clássico do Windows 7/8.",
      false, true },
    { L"Microsoft-Windows-WordPad-FoD", L"WordPad", L"Compatibilidade",
      L"Editor de texto legado. Foi removido em versões recentes do Windows 11.",
      false, true },
    { L"SmbDirect", L"SMB Direct", L"Rede",
      L"Transferência SMB via RDMA. Requer hardware compatível; não é necessário na maioria dos PCs.",
      false, true },
    { L"Windows-Identity-Foundation", L"Windows Identity Foundation 3.5", L"Desenvolvimento",
      L"Biblioteca legada para aplicativos .NET que usam federação de identidade.",
      false, true },
    { L"Windows-Defender-ApplicationGuard", L"Microsoft Defender Application Guard", L"Segurança",
      L"Isolamento de navegador para ambientes corporativos. Requer Hyper-V.",
      false, true },
};

const size_t g_featureCatalogCount = _countof(g_featureCatalog);

// ---------------------------------------------------------------------------
// Políticas de registro
// ---------------------------------------------------------------------------

const PolicyDef g_policyCatalog[] = {
    // Segurança
    { HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\System",
      L"EnableLUA", 1, true, L"Controle de Conta de Usuário (UAC)", L"Segurança",
      L"Restaura a exigência de consentimento do UAC. Desativar o UAC reduz muito a segurança.",
      true },
    { HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\System",
      L"PromptOnSecureDesktop", 1, true, L"UAC em área de trabalho segura", L"Segurança",
      L"Exibe o prompt do UAC em uma área de trabalho isolada, dificultando ataques de clique.",
      true },
    { HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\System",
      L"ConsentPromptBehaviorAdmin", 5, true, L"Comportamento do prompt do UAC", L"Segurança",
      L"Define o padrão da Microsoft: solicitar consentimento para administradores (5).",
      true },
    { HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Services\\SharedAccess\\Parameters\\FirewallPolicy\\DomainProfile",
      L"EnableFirewall", 1, true, L"Firewall — perfil Domínio", L"Segurança",
      L"Reativa o firewall do Windows para redes de domínio.",
      true },
    { HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Services\\SharedAccess\\Parameters\\FirewallPolicy\\StandardProfile",
      L"EnableFirewall", 1, true, L"Firewall — perfil Particular", L"Segurança",
      L"Reativa o firewall do Windows para redes particulares.",
      true },
    { HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Services\\SharedAccess\\Parameters\\FirewallPolicy\\PublicProfile",
      L"EnableFirewall", 1, true, L"Firewall — perfil Público", L"Segurança",
      L"Reativa o firewall do Windows para redes públicas.",
      true },
    { HKEY_LOCAL_MACHINE, L"SOFTWARE\\Policies\\Microsoft\\Windows Defender",
      L"DisableAntiSpyware", 0, false, L"Defender — remover bloqueio do antivírus", L"Segurança",
      L"Remove a política que desativa o Microsoft Defender. A ausência da política é o padrão da Microsoft.",
      true },
    { HKEY_LOCAL_MACHINE, L"SOFTWARE\\Policies\\Microsoft\\Windows Defender",
      L"DisableAntiVirus", 0, false, L"Defender — remover bloqueio do antivírus (DisableAntiVirus)", L"Segurança",
      L"Remove outra política usada por imagens Lite para desativar o Defender.",
      true },
    { HKEY_LOCAL_MACHINE, L"SOFTWARE\\Policies\\Microsoft\\Windows Defender\\Real-Time Protection",
      L"DisableRealtimeMonitoring", 0, false, L"Defender — proteção em tempo real", L"Segurança",
      L"Remove a política que desliga a proteção em tempo real.",
      true },
    { HKEY_LOCAL_MACHINE, L"SOFTWARE\\Policies\\Microsoft\\Windows Defender\\Real-Time Protection",
      L"DisableBehaviorMonitoring", 0, false, L"Defender — monitoramento de comportamento", L"Segurança",
      L"Remove a política que desliga o monitoramento de comportamento.",
      true },
    { HKEY_LOCAL_MACHINE, L"SOFTWARE\\Policies\\Microsoft\\Windows Defender\\Real-Time Protection",
      L"DisableIOAVProtection", 0, false, L"Defender — verificação de downloads", L"Segurança",
      L"Remove a política que desliga a verificação de arquivos baixados.",
      true },
    { HKEY_LOCAL_MACHINE, L"SOFTWARE\\Policies\\Microsoft\\Windows Defender\\Real-Time Protection",
      L"DisableScriptScanning", 0, false, L"Defender — verificação de scripts", L"Segurança",
      L"Remove a política que desliga a verificação de scripts.",
      true },
    { HKEY_LOCAL_MACHINE, L"SOFTWARE\\Policies\\Microsoft\\Windows Defender\\Real-Time Protection",
      L"DisableBlockAtFirstSeen", 0, false, L"Defender — bloqueio no primeiro contato", L"Segurança",
      L"Remove a política que desliga o bloqueio preventivo de arquivos suspeitos.",
      true },

    // Windows Update
    { HKEY_LOCAL_MACHINE, L"SOFTWARE\\Policies\\Microsoft\\Windows\\WindowsUpdate\\AU",
      L"NoAutoUpdate", 0, false, L"Windows Update — remover bloqueio de atualizações", L"Atualizações",
      L"Remove a política que impede o Windows Update de baixar e instalar atualizações.",
      true },
    { HKEY_LOCAL_MACHINE, L"SOFTWARE\\Policies\\Microsoft\\Windows\\WindowsUpdate",
      L"DisableWindowsUpdateAccess", 0, false, L"Windows Update — acesso às configurações", L"Atualizações",
      L"Remove o bloqueio de acesso às configurações do Windows Update.",
      true },
    { HKEY_LOCAL_MACHINE, L"SOFTWARE\\Policies\\Microsoft\\Windows\\WindowsUpdate",
      L"DoNotConnectToWindowsUpdateInternetLocations", 0, false, L"Windows Update — permitir servidores da Microsoft", L"Atualizações",
      L"Remove a política que impede o Windows Update de acessar os servidores da Microsoft.",
      true },
    { HKEY_LOCAL_MACHINE, L"SOFTWARE\\Policies\\Microsoft\\Windows\\WindowsUpdate",
      L"SetDisableUXWUAccess", 0, false, L"Windows Update — acesso à interface", L"Atualizações",
      L"Remove a política que esconde a interface do Windows Update.",
      true },
    { HKEY_LOCAL_MACHINE, L"SOFTWARE\\Policies\\Microsoft\\Windows\\WindowsUpdate",
      L"DisableDualScan", 0, false, L"Windows Update — ignorar bloqueio de verificação dupla", L"Atualizações",
      L"Remove a política de verificação dupla usada por algumas imagens Lite.",
      true },
    { HKEY_LOCAL_MACHINE, L"SOFTWARE\\Policies\\Microsoft\\Windows\\WindowsUpdate",
      L"WUServer", 0, false, L"Windows Update — servidor WSUS forçado", L"Atualizações",
      L"Remove o servidor WSUS forçado por política, restaurando os servidores da Microsoft.",
      true },
    { HKEY_LOCAL_MACHINE, L"SOFTWARE\\Policies\\Microsoft\\Windows\\WindowsUpdate",
      L"WUStatusServer", 0, false, L"Windows Update — servidor de status WSUS forçado", L"Atualizações",
      L"Remove o servidor de status WSUS forçado por política.",
      true },

    // Microsoft Store e conteúdo
    { HKEY_LOCAL_MACHINE, L"SOFTWARE\\Policies\\Microsoft\\WindowsStore",
      L"RemoveWindowsStore", 0, false, L"Microsoft Store — remover bloqueio", L"Loja e aplicativos",
      L"Remove a política que remove ou bloqueia a Microsoft Store.",
      true },
    { HKEY_LOCAL_MACHINE, L"SOFTWARE\\Policies\\Microsoft\\WindowsStore",
      L"DisableStoreApps", 0, false, L"Microsoft Store — permitir aplicativos", L"Loja e aplicativos",
      L"Remove a política que impede a instalação de aplicativos da Store.",
      true },
    { HKEY_LOCAL_MACHINE, L"SOFTWARE\\Policies\\Microsoft\\Windows\\CloudContent",
      L"DisableWindowsConsumerFeatures", 0, false, L"Sugestões e conteúdo de consumo", L"Loja e aplicativos",
      L"Remove o bloqueio de sugestões e aplicativos promovidos pela Microsoft.",
      true },
    { HKEY_LOCAL_MACHINE, L"SOFTWARE\\Policies\\Microsoft\\Windows\\CloudContent",
      L"DisableSoftLanding", 0, false, L"Experiência de primeiros passos", L"Loja e aplicativos",
      L"Remove o bloqueio da experiência de introdução do Windows.",
      false },

    // Pesquisa
    { HKEY_LOCAL_MACHINE, L"SOFTWARE\\Policies\\Microsoft\\Windows\\Windows Search",
      L"DisableWebSearch", 0, false, L"Pesquisa — permitir resultados da Web", L"Pesquisa",
      L"Remove a política que desliga resultados da Web na pesquisa do Windows.",
      true },
    { HKEY_LOCAL_MACHINE, L"SOFTWARE\\Policies\\Microsoft\\Windows\\Windows Search",
      L"ConnectedSearchUseWeb", 0, false, L"Pesquisa — pesquisa conectada", L"Pesquisa",
      L"Remove a política que bloqueia a pesquisa conectada.",
      true },
    { HKEY_LOCAL_MACHINE, L"SOFTWARE\\Policies\\Microsoft\\Windows\\Windows Search",
      L"AllowSearchToUseLocation", 0, false, L"Pesquisa — usar localização", L"Pesquisa",
      L"Remove a política que impede a pesquisa de usar a localização.",
      false },
    { HKEY_LOCAL_MACHINE, L"SOFTWARE\\Policies\\Microsoft\\Windows\\Windows Search",
      L"AllowCortana", 0, false, L"Cortana — remover bloqueio", L"Pesquisa",
      L"Remove a política que desativa a Cortana. Em versões recentes a Cortana já foi descontinuada.",
      false },

    // Manutenção e diagnóstico
    { HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\SystemRestore",
      L"DisableSR", 0, false, L"Restauração do Sistema — remover bloqueio", L"Manutenção",
      L"Remove a política que desativa a Restauração do Sistema. A proteção ainda precisa ser ativada nas propriedades do sistema.",
      true },
    { HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\Windows Error Reporting",
      L"Disabled", 0, false, L"Relatórios de Erros do Windows", L"Diagnóstico",
      L"Remove a política que desativa os relatórios de erros. O padrão da Microsoft é permitir relatórios.",
      false },
    { HKEY_LOCAL_MACHINE, L"SOFTWARE\\Policies\\Microsoft\\Windows\\DataCollection",
      L"AllowTelemetry", 1, true, L"Telemetria — nível padrão (1)", L"Diagnóstico",
      L"Restaura o nível Básico (1) de telemetria. Não é selecionado por padrão para respeitar escolhas de privacidade.",
      false },
};

const size_t g_policyCatalogCount = _countof(g_policyCatalog);

// ---------------------------------------------------------------------------
// Tarefas agendadas
// ---------------------------------------------------------------------------

const TaskDef g_taskCatalog[] = {
    { L"\\Microsoft\\Windows\\WindowsUpdate", L"Scheduled Start",
      L"Windows Update — início agendado", L"Atualizações",
      L"Tarefa que inicia verificações periódicas do Windows Update.", true },
    { L"\\Microsoft\\Windows\\UpdateOrchestrator", L"Schedule Scan",
      L"Orquestrador de Atualizações — verificação", L"Atualizações",
      L"Agenda verificações de atualizações.", true },
    { L"\\Microsoft\\Windows\\UpdateOrchestrator", L"Schedule Scan Static Task",
      L"Orquestrador de Atualizações — verificação estática", L"Atualizações",
      L"Auxilia no agendamento de verificações de atualizações.", true },
    { L"\\Microsoft\\Windows\\UpdateOrchestrator", L"USO_UxBroker",
      L"Orquestrador de Atualizações — interface", L"Atualizações",
      L"Notifica a interface sobre atualizações pendentes.", true },
    { L"\\Microsoft\\Windows\\Windows Defender", L"Windows Defender Scheduled Scan",
      L"Defender — verificação agendada", L"Segurança",
      L"Executa verificações periódicas do Microsoft Defender.", true },
    { L"\\Microsoft\\Windows\\Windows Defender", L"Windows Defender Cache Maintenance",
      L"Defender — manutenção do cache", L"Segurança",
      L"Mantém o cache de definições do Defender.", true },
    { L"\\Microsoft\\Windows\\Windows Defender", L"Windows Defender Cleanup",
      L"Defender — limpeza", L"Segurança",
      L"Limpa arquivos temporários do Defender.", true },
    { L"\\Microsoft\\Windows\\Windows Defender", L"Windows Defender Verification",
      L"Defender — verificação de integridade", L"Segurança",
      L"Verifica a integridade dos componentes do Defender.", true },
    { L"\\Microsoft\\Windows\\SystemRestore", L"SR",
      L"Restauração do Sistema — ponto automático", L"Manutenção",
      L"Cria pontos de restauração automáticos quando habilitado.", true },
    { L"\\Microsoft\\Windows\\Application Experience", L"ProgramDataUpdater",
      L"Experiência de Aplicativos — atualização de dados", L"Manutenção",
      L"Atualiza dados de compatibilidade de aplicativos.", true },
    { L"\\Microsoft\\Windows\\Windows Error Reporting", L"QueueReporting",
      L"Relatórios de Erros — fila", L"Diagnóstico",
      L"Envia relatórios de falhas na fila. Opcional.", false },
    { L"\\Microsoft\\Windows\\Maintenance", L"WinSAT",
      L"Manutenção — avaliação de desempenho", L"Manutenção",
      L"Avalia o desempenho do sistema para ajustes automáticos.", true },
    { L"\\Microsoft\\Windows\\Defrag", L"ScheduledDefrag",
      L"Desfragmentação e otimização agendada", L"Manutenção",
      L"Otimiza unidades e desfragmenta HDDs periodicamente.", true },
    { L"\\Microsoft\\Windows\\Registry", L"RegIdleBackup",
      L"Backup do Registro", L"Manutenção",
      L"Cria backups periódicos do registro.", true },
    { L"\\Microsoft\\Windows\\DiskDiagnostic", L"Microsoft-Windows-DiskDiagnosticDataCollector",
      L"Diagnóstico de Disco — coleta de dados", L"Diagnóstico",
      L"Coleta dados de integridade de discos. Opcional.", false },
    { L"\\Microsoft\\Windows\\Chkdsk", L"ProactiveScan",
      L"Verificação proativa de disco", L"Manutenção",
      L"Verifica o sistema de arquivos em busca de problemas.", true },
    { L"\\Microsoft\\Windows\\Time Synchronization", L"SynchronizeTime",
      L"Sincronização de Horário", L"Sistema",
      L"Sincroniza o relógio com servidores de horário.", true },
    { L"\\Microsoft\\Windows\\InstallService", L"ScanForUpdates",
      L"Microsoft Store — verificar atualizações", L"Loja e aplicativos",
      L"Verifica atualizações de aplicativos da Store.", false },
    { L"\\Microsoft\\Windows\\StateRepository", L"MaintenanceTasks",
      L"Manutenção do Repositório de Estado", L"Manutenção",
      L"Mantém o banco de dados de estado de aplicativos.", true },
    { L"\\Microsoft\\Windows\\Device Information", L"Device",
      L"Informações do Dispositivo", L"Diagnóstico",
      L"Coleta informações de dispositivos para compatibilidade. Opcional.", false },
    { L"\\Microsoft\\Windows\\Autochk", L"Proxy",
      L"Verificação de disco na inicialização", L"Manutenção",
      L"Agenda verificações de disco na inicialização.", true },
};

const size_t g_taskCatalogCount = _countof(g_taskCatalog);

const FeatureDef* FindFeatureDef(const std::wstring& name) {
    for (size_t i = 0; i < g_featureCatalogCount; ++i) {
        if (_wcsicmp(g_featureCatalog[i].name, name.c_str()) == 0) {
            return &g_featureCatalog[i];
        }
    }
    return nullptr;
}

const ServiceDef* FindServiceDef(const std::wstring& name) {
    for (size_t i = 0; i < g_serviceCatalogCount; ++i) {
        if (_wcsicmp(g_serviceCatalog[i].name, name.c_str()) == 0) {
            return &g_serviceCatalog[i];
        }
    }
    return nullptr;
}
