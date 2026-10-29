# Windows Lite Restore

Herramienta nativa para Windows 10/11 (x64) que restaura los servicios, las
políticas de registro, las tareas programadas y las características opcionales
que las imágenes "Lite" de Windows suelen desactivar o eliminar — siempre con
revisión del usuario y usando los valores de referencia de Microsoft.

Además de la restauración, la aplicación ofrece **perfiles de optimización**
(Juegos, IA, Oficina) que desactivan — o ponen en manual — solo los servicios
innecesarios para cada situación. Todo cambio, ya sea activar **o** desactivar,
pasa por revisión, simulación, informe y copia de seguridad reversible.

---

## Características

- Interfaz nativa sin dependencias externas (solo Win32).
- Pestañas: **Características**, **Servicios**, **Políticas**, **Tareas**,
  **Diagnóstico** y **Registro**.
- Selector de perfil como botones visibles (**Predeterminado de Microsoft**,
  **Juegos**, **IA**, **Oficina**).
- Selección individual por casilla, con búsqueda, filtro por categoría y acciones
  rápidas ("Recomendados", "Todos los pendientes", "Limpiar").
- **Simulación (dry-run)** para ver exactamente qué cambiará antes de aplicar.
- **Punto de restauración** y **copia de seguridad reversible** (servicios,
  políticas, tareas) antes de aplicar.
- **Deshacer la última ejecución** (menú Diagnóstico).
- Informes en texto y registro (log) detallado.
- Auto-elevación mediante UAC (`runas`) conservando la selección.

### Perfiles de optimización

| Perfil | Qué hace |
|--------|----------|
| **Predeterminado de Microsoft** | Restaura servicios, políticas, tareas y características al valor de una instalación completa. |
| **Juegos** | Desactiva telemetría/informes y la entrega en segundo plano; pone en manual SysMain, Windows Search, impresión, notificaciones, etc. |
| **Inteligencia Artificial** | Igual que Juegos, y además desactiva los servicios de Xbox; libera CPU/RAM/disco para cargas de IA. |
| **Oficina** | Desactiva solo Xbox y telemetría; mantiene impresión, búsqueda y actualizaciones. |

Los perfiles de optimización **nunca cambian los servicios esenciales del sistema**
(arranque, inicio de sesión, seguridad y red básica). Esos servicios aparecen
como **"Esencial (protegido)"** y no se pueden desactivar, de modo que una
optimización jamás impedirá que Windows arranque o inicie sesión. Los servicios
no incluidos en un perfil aparecen como "Sin cambios".

### Qué se restaura

| Categoría | Ejemplos |
|-----------|----------|
| Servicios | Windows Update, BITS, Store/AppX, audio, temas, seguridad, red |
| Políticas de registro | UAC, telemetría desactivada por política, bloqueos de servicios |
| Tareas programadas | Mantenimiento, Windows Update, diagnóstico |
| Características opcionales | .NET Framework 3.5, características de medios/idioma, herramientas |

El catálogo completo está en `src/catalog.cpp` (con descripción, categoría, tipo
de inicio de referencia y si el elemento es recomendado/crítico/opcional).

---

## Requisitos

- Windows 10 o Windows 11, **64 bits**.
- Sin runtime, framework ni bibliotecas adicionales: el ejecutable se enlaza
  estáticamente (MinGW-w64) y se ejecuta sin instalación.

> Para **instalar características opcionales eliminadas**, puede ser necesaria
> una media/ISO de instalación de Windows de la misma edición/arquitectura
> (carpeta de origen configurable en *Configuración*).

---

## Uso

1. Ejecuta `WindowsLiteRestore.exe`.
2. Si Windows pide elevación (UAC), acéptala — muchos cambios requieren
   privilegios de administrador. Sin elevación, la app funciona en **modo de
   solo lectura**.
3. El análisis se ejecuta automáticamente. Elige el **perfil** deseado en los
   botones bajo las pestañas y revisa los elementos marcados.
4. Opcionalmente marca **Simular** para validar el plan sin cambiar nada.
5. Haz clic en **Aplicar seleccionados** (elementos marcados) o **Aplicar perfil** /
   **Restaurar predeterminados de Microsoft** (recomendados del perfil).
6. Confirma el plan y elige si deseas crear un punto de restauración y una copia.
7. Para revertir servicios/políticas/tareas, usa *Diagnóstico → Deshacer última
   ejecución*.

### Opciones de línea de comandos

| Opción | Descripción |
|--------|-------------|
| `--scan-report <archivo>` | Genera un informe de análisis y sale (sin interfaz). |
| `--profile <nombre>` | Perfil inicial: `default`/`padrao`, `gaming`/`jogos`, `ai`/`ia`, `office`/`escritorio`. |
| `--no-elevate` / `--read-only` | Inicia sin pedir elevación (solo lectura). |
| `--elevated` | Indica que el proceso ya está elevado (uso interno). |
| `--resume-selection <archivo>` | Restaura una selección guardada (uso interno). |

Ejemplo:

```powershell
WindowsLiteRestore.exe --scan-report informe.txt --profile gaming
```

---

## Compilación

El único requisito de compilación es un compilador **MinGW-w64** (recomendado
[w64devkit](https://github.com/skeeto/w64devkit), que incluye `g++` y `windres`).

```powershell
# Opción A: define la ruta del toolchain y compila
$env:W64DEVKIT = "C:\ruta\a\w64devkit"
.\build.ps1

# Opción B: descarga el toolchain automáticamente (requiere internet)
.\build.ps1 -DownloadToolchain
```

El script `build.ps1`:

1. Localiza `g++.exe`/`windres.exe` (mediante `$env:W64DEVKIT`, `tools\w64devkit` o `PATH`).
2. Compila los recursos (`resources.rc`, manifiesto, icono, versión).
3. Compila `main.cpp`, `ui.cpp`, `core.cpp` y `catalog.cpp`.
4. Enlaza estáticamente y escribe `dist\WindowsLiteRestore.exe`, el `README.md`
   y el `SHA256.txt`.

El icono se puede regenerar con `tools\make-icon.ps1` (usa System.Drawing, sin
dependencias externas).

### Estructura del proyecto

```
src/
  app.h            tipos, modelo, clase App, contratos entre módulos
  catalog.cpp      catálogo de servicios, características, políticas y tareas
  core.cpp         escaneo, aplicación, backup, deshacer, informe, elevación, DISM
  ui.cpp           interfaz gráfica (Win32 nativo)
  main.cpp         punto de entrada (wWinMain)
  resources.rc     diálogos, icono, manifiesto, versión
  resource.h       IDs de recursos/comandos
  app.manifest     asInvoker, DPI aware, controles comunes v6
dist/              ejecutable final + README + SHA256
tools/             make-icon.ps1
```

---

## Seguridad y limitaciones

- **Los valores de referencia** corresponden a una instalación completa de
  Windows 10/11. Las ediciones, builds y el hardware varían; revisa antes de aplicar.
- **Los perfiles de optimización desactivan servicios** (telemetría, Xbox, etc.)
  y ponen otros en manual. **No** cambian servicios críticos ni de seguridad, y
  todo se puede revertir con el backup/Deshacer.
- **Los servicios esenciales están protegidos** y nunca se pueden desactivar.
- **Los servicios ausentes no se "instalan"** con la app — la imagen Lite puede
  haber eliminado el componente de origen. Estos elementos aparecen como
  no disponibles.
- **Las características eliminadas** pueden requerir media/ISO compatible
  (errores comunes de DISM: `0x800F081F`, `0x800F0954`, `0x800F0906` — indica la
  carpeta de origen).
- **Las características opcionales no se revierten** con "Deshacer", para evitar
  quitar componentes que podrían desestabilizar el sistema.
- **Algunos servicios están protegidos** (p. ej. Windows Update Medic, Defender).
  Si un cambio falla, revisa la protección contra manipulaciones.
- La aplicación es independiente y **no está afiliada a Microsoft**. Úsala bajo
  tu responsabilidad, preferiblemente con un punto de restauración y una copia.

### Ubicación de los datos

| Dato | Ubicación |
|------|-----------|
| Configuración | `%LOCALAPPDATA%\WindowsLiteRestore\settings.ini` |
| Logs (solo lectura) | `%LOCALAPPDATA%\WindowsLiteRestore\logs` |
| Logs (elevado) | `%PROGRAMDATA%\WindowsLiteRestore\logs` |
| Copias de seguridad | `%PROGRAMDATA%\WindowsLiteRestore\Backups` |
| Informes | `%PROGRAMDATA%\WindowsLiteRestore\reports` |

---

## Aviso

Este software modifica la configuración del sistema e instala componentes de
Windows. Crea siempre un punto de restauración y una copia de seguridad
reversible antes de aplicar, y ejecuta una simulación para revisar el plan.
