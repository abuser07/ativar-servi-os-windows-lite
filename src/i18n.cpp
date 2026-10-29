#include "i18n.h"

#include <cwchar>

namespace {

const wchar_t* const g_strings[kLanguageCount][STR_COUNT] = {
    // ---------------------------------------------------------------------
    // Português (0)
    // ---------------------------------------------------------------------
    {
        L"Restaurador Windows Lite",                                       // APP_TITLE
        L"Restaurador Windows Lite — Somente leitura",                     // APP_TITLE_READONLY
        L"Restaurador Windows Lite",                                       // HEADER_TITLE
        L"Restaura serviços, políticas e recursos desativados por imagens Lite", // HEADER_SUBTITLE_ADMIN
        L"Modo somente leitura — clique em ELEVAR AGORA para liberar as alterações", // HEADER_SUBTITLE_READONLY
        L"Perfil de otimização:",                                          // PROFILE_LABEL
        L"Padrão Microsoft",                                               // PROFILE_DEFAULT
        L"Jogos",                                                          // PROFILE_GAMING
        L"IA",                                                             // PROFILE_AI
        L"Escritório",                                                     // PROFILE_OFFICE
        L"Restaura serviços, políticas, tarefas e recursos para o padrão Microsoft.", // PROFILE_DESC_DEFAULT
        L"Desativa telemetria e serviços desnecessários para liberar CPU, memória e rede durante os jogos.", // PROFILE_DESC_GAMING
        L"Reduz atividade em segundo plano e desativa jogos/telemetria para dar o máximo de recursos a cargas de IA.", // PROFILE_DESC_AI
        L"Desativa jogos e telemetria, mantendo impressão, pesquisa e atualizações para produtividade.", // PROFILE_DESC_OFFICE
        L"Recursos",                                                       // TAB_FEATURES
        L"Serviços",                                                       // TAB_SERVICES
        L"Políticas",                                                      // TAB_POLICIES
        L"Tarefas",                                                        // TAB_TASKS
        L"Diagnóstico",                                                    // TAB_DIAG
        L"Registro",                                                       // TAB_LOG
        L"Item",                                                           // COL_ITEM
        L"Categoria",                                                      // COL_CATEGORY
        L"Estado atual",                                                   // COL_CURRENT
        L"Padrão Microsoft",                                               // COL_TARGET_DEFAULT
        L"Destino do perfil",                                              // COL_TARGET_PROFILE
        L"Ação",                                                           // COL_ACTION
        L"Já conforme",                                                    // ACTION_ALREADY
        L"Instalar",                                                       // ACTION_INSTALL
        L"Desativar",                                                      // ACTION_DISABLE
        L"Definir manual",                                                 // ACTION_MANUAL
        L"Ativar",                                                         // ACTION_ENABLE
        L"Restaurar",                                                      // ACTION_RESTORE
        L"Habilitar",                                                      // ACTION_ENABLE_TASK
        L"Essencial (protegido)",                                          // ACTION_ESSENTIAL
        L"Recurso",                                                        // KIND_FEATURE
        L"Serviço",                                                        // KIND_SERVICE
        L"Política",                                                       // KIND_POLICY
        L"Tarefa",                                                         // KIND_TASK
        L"Recomendados",                                                   // BTN_RECOMMENDED
        L"Todos pendentes",                                                // BTN_ALL_PENDING
        L"Limpar",                                                         // BTN_CLEAR
        L"Configurações",                                                  // BTN_SETTINGS
        L"Atualizar análise",                                              // BTN_RESCAN
        L"Sobre",                                                          // BTN_ABOUT
        L"Aplicar selecionados",                                           // BTN_APPLY_SELECTED
        L"Aplicar perfil (recomendado)",                                   // BTN_APPLY_PROFILE
        L"Restaurar padrão Microsoft",                                     // BTN_RESTORE_DEFAULT
        L"Simular (não altera o sistema)",                                 // SIMULATE
        L"Cancelar",                                                       // CANCEL
        L"Pesquisar por nome, categoria ou identificador...",              // SEARCH_PLACEHOLDER
        L"Todas as categorias",                                            // ALL_CATEGORIES
        L"Detalhes do item",                                               // DETAIL_TITLE
        L"Selecione um item na lista para ver o estado atual, o padrão de referência da Microsoft e a ação que será aplicada.", // DETAIL_EMPTY
        L"Estado atual",                                                   // LABEL_CURRENT
        L"Padrão Microsoft",                                               // LABEL_TARGET_DEFAULT
        L"Destino do perfil",                                              // LABEL_TARGET_PROFILE
        L"Essencial — serviço protegido contra desativação.",              // DETAIL_ESSENTIAL
        L"Indisponível — pode ter sido removido pela imagem Lite.",        // DETAIL_UNAVAILABLE
        L"Já está no padrão de referência.",                               // DETAIL_OK
        L"Pendente — marque a caixa para aplicar a alteração.",            // DETAIL_PENDING
        L"Pronto. Clique em Atualizar análise para verificar o sistema.",  // STATUS_READY
        L"Analisando o sistema...",                                        // STATUS_ANALYZE
        L"Atualizar",                                                      // DIAG_BTN_REFRESH
        L"Salvar relatório",                                               // DIAG_BTN_SAVE
        L"Abrir logs",                                                     // DIAG_BTN_LOGS
        L"Abrir backups",                                                  // DIAG_BTN_BACKUPS
        L"Desfazer última execução",                                       // DIAG_BTN_UNDO
        L"Salvar log",                                                     // LOG_BTN_SAVE
        L"Abrir pasta",                                                    // LOG_BTN_FOLDER
        L"Limpar visualização",                                            // LOG_BTN_CLEAR
        L"Idioma / Language",                                              // LANG_LABEL
        L"Atualizações",                                                   // CAT_UPDATE
        L"Loja e aplicativos",                                             // CAT_STORE
        L"Interface e áudio",                                              // CAT_UI_AUDIO
        L"Segurança",                                                      // CAT_SECURITY
        L"Rede",                                                           // CAT_NETWORK
        L"Impressão",                                                      // CAT_PRINT
        L"Dispositivos",                                                   // CAT_DEVICES
        L"Sistema",                                                        // CAT_SYSTEM
        L"Pesquisa",                                                       // CAT_SEARCH
        L"Área de trabalho remota",                                        // CAT_RDP
        L"Gerenciamento remoto",                                           // CAT_REMOTE
        L"Jogos",                                                          // CAT_GAMES
        L"Diagnóstico e telemetria",                                       // CAT_DIAG_TEL
        L"Mídia",                                                          // CAT_MEDIA
        L"Compatibilidade",                                                // CAT_COMPAT
        L"Virtualização",                                                  // CAT_VIRT
        L"Outros recursos",                                                // CAT_OTHER
        L"Diagnóstico",                                                    // CAT_DIAGNOSTIC
    },

    // ---------------------------------------------------------------------
    // English (1)
    // ---------------------------------------------------------------------
    {
        L"Windows Lite Restore",
        L"Windows Lite Restore — Read-only",
        L"Windows Lite Restore",
        L"Restores services, policies and features disabled by Lite images",
        L"Read-only mode — click ELEVATE NOW to enable changes",
        L"Optimization profile:",
        L"Microsoft Default",
        L"Gaming",
        L"AI",
        L"Office",
        L"Restores services, policies, tasks and features to the Microsoft default.",
        L"Disables telemetry and unnecessary services to free CPU, memory and network during gaming.",
        L"Reduces background activity and disables gaming/telemetry to give maximum resources to AI workloads.",
        L"Disables gaming and telemetry, keeping printing, search and updates for productivity.",
        L"Features",
        L"Services",
        L"Policies",
        L"Tasks",
        L"Diagnostics",
        L"Log",
        L"Item",
        L"Category",
        L"Current state",
        L"Microsoft default",
        L"Profile target",
        L"Action",
        L"Already compliant",
        L"Install",
        L"Disable",
        L"Set manual",
        L"Enable",
        L"Restore",
        L"Enable",
        L"Essential (protected)",
        L"Feature",
        L"Service",
        L"Policy",
        L"Task",
        L"Recommended",
        L"All pending",
        L"Clear",
        L"Settings",
        L"Rescan",
        L"About",
        L"Apply selected",
        L"Apply profile (recommended)",
        L"Restore Microsoft defaults",
        L"Simulate (no system changes)",
        L"Cancel",
        L"Search by name, category or identifier...",
        L"All categories",
        L"Item details",
        L"Select an item in the list to see its current state, the Microsoft reference default and the action that will be applied.",
        L"Current state",
        L"Microsoft default",
        L"Profile target",
        L"Essential — service protected against disabling.",
        L"Unavailable — may have been removed by the Lite image.",
        L"Already at the reference default.",
        L"Pending — tick the box to apply the change.",
        L"Ready. Click Rescan to check the system.",
        L"Analyzing the system...",
        L"Refresh",
        L"Save report",
        L"Open logs",
        L"Open backups",
        L"Undo last run",
        L"Save log",
        L"Open folder",
        L"Clear view",
        L"Language / Idioma",
        L"Updates",
        L"Store and apps",
        L"UI and audio",
        L"Security",
        L"Network",
        L"Printing",
        L"Devices",
        L"System",
        L"Search",
        L"Remote Desktop",
        L"Remote management",
        L"Games",
        L"Diagnostics and telemetry",
        L"Media",
        L"Compatibility",
        L"Virtualization",
        L"Other features",
        L"Diagnostics",
    },

    // ---------------------------------------------------------------------
    // Español (2)
    // ---------------------------------------------------------------------
    {
        L"Restaurador Windows Lite",
        L"Restaurador Windows Lite — Solo lectura",
        L"Restaurador Windows Lite",
        L"Restaura servicios, políticas y funciones desactivadas por imágenes Lite",
        L"Modo de solo lectura — haz clic en ELEVAR AHORA para permitir cambios",
        L"Perfil de optimización:",
        L"Predeterminado de Microsoft",
        L"Juegos",
        L"IA",
        L"Oficina",
        L"Restaura servicios, políticas, tareas y funciones al valor predeterminado de Microsoft.",
        L"Desactiva telemetría y servicios innecesarios para liberar CPU, memoria y red durante los juegos.",
        L"Reduce la actividad en segundo plano y desactiva juegos/telemetría para dar el máximo de recursos a cargas de IA.",
        L"Desactiva juegos y telemetría, manteniendo impresión, búsqueda y actualizaciones para la productividad.",
        L"Características",
        L"Servicios",
        L"Políticas",
        L"Tareas",
        L"Diagnóstico",
        L"Registro",
        L"Elemento",
        L"Categoría",
        L"Estado actual",
        L"Predeterminado de Microsoft",
        L"Destino del perfil",
        L"Acción",
        L"Ya conforme",
        L"Instalar",
        L"Desactivar",
        L"Definir manual",
        L"Activar",
        L"Restaurar",
        L"Habilitar",
        L"Esencial (protegido)",
        L"Característica",
        L"Servicio",
        L"Política",
        L"Tarea",
        L"Recomendados",
        L"Todos los pendientes",
        L"Limpiar",
        L"Configuración",
        L"Actualizar análisis",
        L"Acerca de",
        L"Aplicar seleccionados",
        L"Aplicar perfil (recomendado)",
        L"Restaurar predeterminados de Microsoft",
        L"Simular (sin cambios en el sistema)",
        L"Cancelar",
        L"Buscar por nombre, categoría o identificador...",
        L"Todas las categorías",
        L"Detalles del elemento",
        L"Selecciona un elemento de la lista para ver su estado actual, el valor de referencia de Microsoft y la acción que se aplicará.",
        L"Estado actual",
        L"Predeterminado de Microsoft",
        L"Destino del perfil",
        L"Esencial — servicio protegido contra la desactivación.",
        L"No disponible — puede haber sido eliminado por la imagen Lite.",
        L"Ya está en el valor de referencia.",
        L"Pendiente — marca la casilla para aplicar el cambio.",
        L"Listo. Haz clic en Actualizar análisis para comprobar el sistema.",
        L"Analizando el sistema...",
        L"Actualizar",
        L"Guardar informe",
        L"Abrir registros",
        L"Abrir copias de seguridad",
        L"Deshacer última ejecución",
        L"Guardar registro",
        L"Abrir carpeta",
        L"Limpiar vista",
        L"Idioma / Language",
        L"Actualizaciones",
        L"Tienda y aplicaciones",
        L"Interfaz y audio",
        L"Seguridad",
        L"Red",
        L"Impresión",
        L"Dispositivos",
        L"Sistema",
        L"Búsqueda",
        L"Escritorio remoto",
        L"Administración remota",
        L"Juegos",
        L"Diagnóstico y telemetría",
        L"Multimedia",
        L"Compatibilidad",
        L"Virtualización",
        L"Otras características",
        L"Diagnóstico",
    },

    // ---------------------------------------------------------------------
    // Français (3)
    // ---------------------------------------------------------------------
    {
        L"Restaurateur Windows Lite",
        L"Restaurateur Windows Lite — Lecture seule",
        L"Restaurateur Windows Lite",
        L"Restaure les services, stratégies et fonctionnalités désactivés par les images Lite",
        L"Mode lecture seule — cliquez sur ÉLEVER pour autoriser les changements",
        L"Profil d'optimisation :",
        L"Défaut Microsoft",
        L"Jeux",
        L"IA",
        L"Bureau",
        L"Restaure services, stratégies, tâches et fonctionnalités aux valeurs par défaut de Microsoft.",
        L"Désactive la télémétrie et les services inutiles pour libérer CPU, mémoire et réseau pendant les jeux.",
        L"Réduit l'activité en arrière-plan et désactive jeux/télémétrie pour donner le maximum de ressources aux charges d'IA.",
        L"Désactive les jeux et la télémétrie, en conservant impression, recherche et mises à jour pour la productivité.",
        L"Fonctionnalités",
        L"Services",
        L"Stratégies",
        L"Tâches",
        L"Diagnostic",
        L"Journal",
        L"Élément",
        L"Catégorie",
        L"État actuel",
        L"Défaut Microsoft",
        L"Cible du profil",
        L"Action",
        L"Déjà conforme",
        L"Installer",
        L"Désactiver",
        L"Définir manuel",
        L"Activer",
        L"Restaurer",
        L"Activer",
        L"Essentiel (protégé)",
        L"Fonctionnalité",
        L"Service",
        L"Stratégie",
        L"Tâche",
        L"Recommandés",
        L"Tous les éléments en attente",
        L"Effacer",
        L"Paramètres",
        L"Réanalyser",
        L"À propos",
        L"Appliquer la sélection",
        L"Appliquer le profil (recommandé)",
        L"Restaurer les valeurs par défaut Microsoft",
        L"Simuler (aucun changement système)",
        L"Annuler",
        L"Rechercher par nom, catégorie ou identifiant...",
        L"Toutes les catégories",
        L"Détails de l'élément",
        L"Sélectionnez un élément de la liste pour voir son état actuel, la valeur de référence Microsoft et l'action qui sera appliquée.",
        L"État actuel",
        L"Défaut Microsoft",
        L"Cible du profil",
        L"Essentiel — service protégé contre la désactivation.",
        L"Indisponible — peut avoir été supprimé par l'image Lite.",
        L"Déjà à la valeur de référence.",
        L"En attente — cochez la case pour appliquer le changement.",
        L"Prêt. Cliquez sur Réanalyser pour vérifier le système.",
        L"Analyse du système...",
        L"Actualiser",
        L"Enregistrer le rapport",
        L"Ouvrir les journaux",
        L"Ouvrir les sauvegardes",
        L"Annuler la dernière exécution",
        L"Enregistrer le journal",
        L"Ouvrir le dossier",
        L"Effacer la vue",
        L"Langue / Language",
        L"Mises à jour",
        L"Store et applications",
        L"Interface et audio",
        L"Sécurité",
        L"Réseau",
        L"Impression",
        L"Périphériques",
        L"Système",
        L"Recherche",
        L"Bureau à distance",
        L"Administration à distance",
        L"Jeux",
        L"Diagnostic et télémétrie",
        L"Multimédia",
        L"Compatibilité",
        L"Virtualisation",
        L"Autres fonctionnalités",
        L"Diagnostic",
    },

    // ---------------------------------------------------------------------
    // 简体中文 (4)
    // ---------------------------------------------------------------------
    {
        L"Windows Lite 恢复工具",
        L"Windows Lite 恢复工具 — 只读模式",
        L"Windows Lite 恢复工具",
        L"恢复被精简版镜像禁用的服务、策略和功能",
        L"只读模式 — 点击“立即提权”以允许更改",
        L"优化配置：",
        L"Microsoft 默认",
        L"游戏",
        L"AI",
        L"办公",
        L"将服务、策略、任务和功能恢复为 Microsoft 默认值。",
        L"禁用遥测和不必要的服务，以便在游戏时释放 CPU、内存和网络资源。",
        L"减少后台活动并禁用游戏/遥测，为 AI 工作负载提供最大资源。",
        L"禁用游戏和遥测，保留打印、搜索和更新以提高办公效率。",
        L"功能",
        L"服务",
        L"策略",
        L"任务",
        L"诊断",
        L"日志",
        L"项目",
        L"类别",
        L"当前状态",
        L"Microsoft 默认",
        L"配置目标",
        L"操作",
        L"已符合",
        L"安装",
        L"禁用",
        L"设为手动",
        L"启用",
        L"恢复",
        L"启用",
        L"关键（受保护）",
        L"功能",
        L"服务",
        L"策略",
        L"任务",
        L"推荐",
        L"全部待处理",
        L"清除",
        L"设置",
        L"重新分析",
        L"关于",
        L"应用所选项目",
        L"应用配置（推荐）",
        L"恢复 Microsoft 默认值",
        L"模拟（不更改系统）",
        L"取消",
        L"按名称、类别或标识符搜索...",
        L"所有类别",
        L"项目详情",
        L"在列表中选择一个项目，以查看其当前状态、Microsoft 参考默认值以及将要应用的操作。",
        L"当前状态",
        L"Microsoft 默认",
        L"配置目标",
        L"关键 — 该服务受保护，无法禁用。",
        L"不可用 — 可能已被精简版镜像移除。",
        L"已处于参考默认值。",
        L"待处理 — 勾选以应用更改。",
        L"就绪。点击“重新分析”以检查系统。",
        L"正在分析系统...",
        L"刷新",
        L"保存报告",
        L"打开日志",
        L"打开备份",
        L"撤销上次运行",
        L"保存日志",
        L"打开文件夹",
        L"清空视图",
        L"语言 / Language",
        L"更新",
        L"应用商店和应用",
        L"界面与音频",
        L"安全",
        L"网络",
        L"打印",
        L"设备",
        L"系统",
        L"搜索",
        L"远程桌面",
        L"远程管理",
        L"游戏",
        L"诊断与遥测",
        L"媒体",
        L"兼容性",
        L"虚拟化",
        L"其他功能",
        L"诊断",
    },

    // ---------------------------------------------------------------------
    // हिन्दी (5)
    // ---------------------------------------------------------------------
    {
        L"Windows Lite रिस्टोर",
        L"Windows Lite रिस्टोर — केवल-पढ़ने",
        L"Windows Lite रिस्टोर",
        L"Lite इमेज द्वारा अक्षम की गई सेवाएँ, नीतियाँ और सुविधाएँ पुनर्स्थापित करें",
        L"केवल-पढ़ने मोड — बदलाव के लिए ELEVATE NOW पर क्लिक करें",
        L"अनुकूलन प्रोफ़ाइल:",
        L"Microsoft डिफ़ॉल्ट",
        L"गेमिंग",
        L"AI",
        L"कार्यालय",
        L"सेवाओं, नीतियों, कार्यों और सुविधाओं को Microsoft डिफ़ॉल्ट पर पुनर्स्थापित करता है।",
        L"गेमिंग के दौरान CPU, मेमोरी और नेटवर्क मुक्त करने के लिए टेलीमेट्री और अनावश्यक सेवाएँ अक्षम करता है।",
        L"पृष्ठभूमि गतिविधि घटाता है और AI कार्यभार को अधिकतम संसाधन देने के लिए गेमिंग/टेलीमेट्री अक्षम करता है।",
        L"गेमिंग और टेलीमेट्री अक्षम करता है, उत्पादकता के लिए प्रिंटिंग, खोज और अपडेट बनाए रखता है।",
        L"सुविधाएँ",
        L"सेवाएँ",
        L"नीतियाँ",
        L"कार्य",
        L"निदान",
        L"लॉग",
        L"आइटम",
        L"श्रेणी",
        L"वर्तमान स्थिति",
        L"Microsoft डिफ़ॉल्ट",
        L"प्रोफ़ाइल लक्ष्य",
        L"क्रिया",
        L"पहले से अनुरूप",
        L"इंस्टॉल करें",
        L"अक्षम करें",
        L"मैनुअल करें",
        L"सक्षम करें",
        L"पुनर्स्थापित करें",
        L"सक्षम करें",
        L"आवश्यक (संरक्षित)",
        L"सुविधा",
        L"सेवा",
        L"नीति",
        L"कार्य",
        L"अनुशंसित",
        L"सभी लंबित",
        L"साफ़ करें",
        L"सेटिंग्स",
        L"पुनः विश्लेषण",
        L"परिचय",
        L"चयनित लागू करें",
        L"प्रोफ़ाइल लागू करें (अनुशंसित)",
        L"Microsoft डिफ़ॉल्ट पुनर्स्थापित करें",
        L"सिमुलेट करें (सिस्टम में कोई बदलाव नहीं)",
        L"रद्द करें",
        L"नाम, श्रेणी या पहचानकर्ता से खोजें...",
        L"सभी श्रेणियाँ",
        L"आइटम विवरण",
        L"वर्तमान स्थिति, Microsoft संदर्भ डिफ़ॉल्ट और लागू होने वाली क्रिया देखने के लिए सूची में कोई आइटम चुनें।",
        L"वर्तमान स्थिति",
        L"Microsoft डिफ़ॉल्ट",
        L"प्रोफ़ाइल लक्ष्य",
        L"आवश्यक — यह सेवा अक्षम करने से संरक्षित है।",
        L"अनुपलब्ध — Lite इमेज द्वारा हटाई गई हो सकती है।",
        L"पहले से संदर्भ डिफ़ॉल्ट पर है।",
        L"लंबित — बदलाव लागू करने के लिए बॉक्स चुनें।",
        L"तैयार। सिस्टम जाँचने के लिए पुनः विश्लेषण पर क्लिक करें।",
        L"सिस्टम का विश्लेषण हो रहा है...",
        L"ताज़ा करें",
        L"रिपोर्ट सहेजें",
        L"लॉग खोलें",
        L"बैकअप खोलें",
        L"अंतिम रन पूर्ववत करें",
        L"लॉग सहेजें",
        L"फ़ोल्डर खोलें",
        L"दृश्य साफ़ करें",
        L"भाषा / Language",
        L"अपडेट",
        L"स्टोर और ऐप्स",
        L"UI और ऑडियो",
        L"सुरक्षा",
        L"नेटवर्क",
        L"प्रिंटिंग",
        L"डिवाइस",
        L"सिस्टम",
        L"खोज",
        L"दूरस्थ डेस्कटॉप",
        L"दूरस्थ प्रबंधन",
        L"गेम",
        L"निदान और टेलीमेट्री",
        L"मीडिया",
        L"संगतता",
        L"वर्चुअलाइज़ेशन",
        L"अन्य सुविधाएँ",
        L"निदान",
    },
};

const wchar_t* const g_languageNames[kLanguageCount] = {
    L"Português", L"English", L"Español", L"Français", L"中文 (简体)", L"हिन्दी",
};

Language g_language = Language::Portuguese;

// Mapeia a categoria original (português) para o id traduzível.
struct CategoryMap {
    const wchar_t* original;
    StrId id;
};

const CategoryMap g_categories[] = {
    {L"Atualizações", STR_CAT_UPDATE},
    {L"Loja e aplicativos", STR_CAT_STORE},
    {L"Interface e áudio", STR_CAT_UI_AUDIO},
    {L"Segurança", STR_CAT_SECURITY},
    {L"Rede", STR_CAT_NETWORK},
    {L"Impressão", STR_CAT_PRINT},
    {L"Dispositivos", STR_CAT_DEVICES},
    {L"Sistema", STR_CAT_SYSTEM},
    {L"Pesquisa", STR_CAT_SEARCH},
    {L"Área de trabalho remota", STR_CAT_RDP},
    {L"Gerenciamento remoto", STR_CAT_REMOTE},
    {L"Jogos", STR_CAT_GAMES},
    {L"Diagnóstico e telemetria", STR_CAT_DIAG_TEL},
    {L"Mídia", STR_CAT_MEDIA},
    {L"Compatibilidade", STR_CAT_COMPAT},
    {L"Virtualização", STR_CAT_VIRT},
    {L"Outros recursos", STR_CAT_OTHER},
    {L"Diagnóstico", STR_CAT_DIAGNOSTIC},
};

}  // namespace

const wchar_t* LanguageName(Language language) {
    int index = static_cast<int>(language);
    if (index < 0 || index >= kLanguageCount) index = 0;
    return g_languageNames[index];
}

Language LanguageFromName(const wchar_t* name) {
    if (!name) return Language::Portuguese;
    for (int i = 0; i < kLanguageCount; ++i) {
        if (_wcsicmp(name, g_languageNames[i]) == 0) return static_cast<Language>(i);
    }
    return Language::Portuguese;
}

void SetUiLanguage(Language language) {
    g_language = language;
}

Language GetUiLanguage() {
    return g_language;
}

const wchar_t* Tr(StrId id) {
    int lang = static_cast<int>(g_language);
    if (lang < 0 || lang >= kLanguageCount) lang = 0;
    int index = static_cast<int>(id);
    if (index < 0 || index >= STR_COUNT) return L"";
    return g_strings[lang][index];
}

const wchar_t* TrCategory(const wchar_t* original) {
    if (!original) return L"";
    for (const CategoryMap& map : g_categories) {
        if (wcscmp(original, map.original) == 0) return Tr(map.id);
    }
    return original;
}

const wchar_t* TrProfileName(int profile) {
    switch (profile) {
        case 0: return Tr(STR_PROFILE_DEFAULT);
        case 1: return Tr(STR_PROFILE_GAMING);
        case 2: return Tr(STR_PROFILE_AI);
        case 3: return Tr(STR_PROFILE_OFFICE);
    }
    return L"";
}

const wchar_t* TrProfileDescription(int profile) {
    switch (profile) {
        case 0: return Tr(STR_PROFILE_DESC_DEFAULT);
        case 1: return Tr(STR_PROFILE_DESC_GAMING);
        case 2: return Tr(STR_PROFILE_DESC_AI);
        case 3: return Tr(STR_PROFILE_DESC_OFFICE);
    }
    return L"";
}
