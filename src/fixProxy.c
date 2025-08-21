// ProxySettingsPerUser.c — Compatible con MinGW clásico

#define UNICODE
#define _UNICODE

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600 // Vista/7/8/10/11
#endif

#include <windows.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>

// Fallbacks para SDKs viejos:
#ifndef KEY_WOW64_64KEY
#define KEY_WOW64_64KEY 0x0100
#endif
#ifndef KEY_WOW64_32KEY
#define KEY_WOW64_32KEY 0x0200
#endif

#define ARRAY_LEN(a) (sizeof(a) / sizeof((a)[0]))

#define REG_PATH       L"SOFTWARE\\Policies\\Microsoft\\Windows\\CurrentVersion\\Internet Settings"
#define REG_VALUE_NAME L"ProxySettingsPerUser"

static bool g_silent = false;

static void ShowMsg(UINT type, const wchar_t* title, const wchar_t* fmt, ...)
{
    wchar_t buf[1024];
    va_list args;
    va_start(args, fmt);

    // MinGW clásico NO tiene vswprintf(buf, size, ...).
    // En su lugar, usamos _vsnwprintf(buf, size, ...).
    int n = _vsnwprintf(buf, ARRAY_LEN(buf) - 1, fmt, args);
    va_end(args);

    if (n < 0) {
        // Por seguridad, dejamos una cadena corta si falló el formateo.
        buf[0] = L'\0';
    } else {
        buf[ARRAY_LEN(buf) - 1] = L'\0';
    }

    if (g_silent) {
        FILE* out = (type == MB_ICONERROR) ? stderr : stdout;
        fwprintf(out, L"%s: %s\n", title, buf);
        fflush(out);
    } else {
        MessageBoxW(NULL, buf, title, MB_OK | type);
    }
}

static bool IsRunningAsAdmin(void)
{
    BOOL isAdmin = FALSE;
    PSID adminGroup = NULL;
    SID_IDENTIFIER_AUTHORITY NtAuthority = { SECURITY_NT_AUTHORITY }; // <-- con llaves

    if (!AllocateAndInitializeSid(&NtAuthority, 2,
                                  SECURITY_BUILTIN_DOMAIN_RID,
                                  DOMAIN_ALIAS_RID_ADMINS,
                                  0,0,0,0,0,0,
                                  &adminGroup)) {
        return false;
    }

    CheckTokenMembership(NULL, adminGroup, &isAdmin);
    FreeSid(adminGroup);
    return isAdmin == TRUE;
}

// Devuelve 1, 0, o -1 si no existe/no legible
static int ReadProxySettingsPerUser(void)
{
    HKEY hKey = NULL;
    DWORD dwType = 0;
    DWORD dwData = 0;
    DWORD cbData = sizeof(dwData);

    LONG st = RegOpenKeyExW(HKEY_LOCAL_MACHINE, REG_PATH, 0,
                            KEY_QUERY_VALUE | KEY_WOW64_64KEY, &hKey);
    if (st != ERROR_SUCCESS) {
        st = RegOpenKeyExW(HKEY_LOCAL_MACHINE, REG_PATH, 0,
                           KEY_QUERY_VALUE, &hKey);
        if (st != ERROR_SUCCESS) return -1;
    }

    st = RegQueryValueExW(hKey, REG_VALUE_NAME, NULL, &dwType, (LPBYTE)&dwData, &cbData);
    RegCloseKey(hKey);

    if (st != ERROR_SUCCESS || dwType != REG_DWORD || cbData != sizeof(DWORD)) {
        return -1;
    }
    return (dwData != 0) ? 1 : 0;
}

static bool WriteProxySettingsPerUser(DWORD value)
{
    HKEY hKey = NULL;
    DWORD disposition = 0;

    LONG st = RegCreateKeyExW(HKEY_LOCAL_MACHINE, REG_PATH, 0, NULL, 0,
                              KEY_SET_VALUE | KEY_WOW64_64KEY, NULL, &hKey, &disposition);
    if (st != ERROR_SUCCESS) {
        st = RegCreateKeyExW(HKEY_LOCAL_MACHINE, REG_PATH, 0, NULL, 0,
                             KEY_SET_VALUE, NULL, &hKey, &disposition);
        if (st != ERROR_SUCCESS) return false;
    }

    st = RegSetValueExW(hKey, REG_VALUE_NAME, 0, REG_DWORD, (const BYTE*)&value, sizeof(DWORD));
    RegCloseKey(hKey);
    return (st == ERROR_SUCCESS);
}

static bool HasArg(int argc, char** argv, const char* flag)
{
    for (int i = 1; i < argc; ++i) {
        if (_stricmp(argv[i], flag) == 0) return true;
    }
    return false;
}

// Usamos main (ANSI) para compatibilidad con MinGW clásico
int main(int argc, char** argv)
{
    g_silent = HasArg(argc, argv, "/silent") || HasArg(argc, argv, "--silent");

    if (!IsRunningAsAdmin()) {
        ShowMsg(MB_ICONERROR, L"Permisos insuficientes",
                L"Este programa requiere privilegios de administrador para modificar HKLM.\n"
                L"Por favor, ejecútalo como administrador.");
        return 1;
    }

    int current = ReadProxySettingsPerUser();
    if (current == 1) {
        ShowMsg(MB_ICONINFORMATION, L"Información",
                L"El valor 'ProxySettingsPerUser' ya está en 1.\nNo se realizaron cambios.");
        return 0;
    } else if (current == 0) {
        if (WriteProxySettingsPerUser(1)) {
            ShowMsg(MB_ICONINFORMATION, L"Listo",
                    L"Registro modificado con éxito.\nProxySettingsPerUser se estableció en 1.");
            return 0;
        } else {
            ShowMsg(MB_ICONERROR, L"Error",
                    L"No se pudo escribir en el registro.\n"
                    L"Verifica políticas de GPO, antivirus o permisos.");
            return 2;
        }
    } else {
        if (WriteProxySettingsPerUser(1)) {
            ShowMsg(MB_ICONINFORMATION, L"Listo",
                    L"El valor no existía o no era legible y fue creado con 1.");
            return 0;
        } else {
            ShowMsg(MB_ICONERROR, L"Error",
                    L"No se pudo crear el valor en el registro.\n"
                    L"Verifica políticas de GPO, antivirus o permisos.");
            return 3;
        }
    }
}
