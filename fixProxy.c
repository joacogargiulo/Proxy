#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

int main() {
    // Comando para obtener el valor actual del registro
    char checkCommand[] = "reg query \"HKEY_LOCAL_MACHINE\\SOFTWARE\\Policies\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\" /v ProxySettingsPerUser";
    
    // Ejecutar el comando y leer la salida
    FILE *fp = popen(checkCommand, "r");
    if (fp == NULL) {
        MessageBoxW(NULL, L"Error al leer el registro", L"Error", MB_OK | MB_ICONERROR);
        return 1;
    }
    
    char output[256];
    int currentValue = -1;
    while (fgets(output, sizeof(output), fp) != NULL) {
        if (strstr(output, "0x1")) {
            currentValue = 1;
            break;
        }
    }
    pclose(fp);
    
    // Si el valor ya es 1, informar y salir
    if (currentValue == 1) {
        MessageBoxW(NULL, L"El valor ya estaba en 1, no fue necesario modificarlo.", L"Información", MB_OK | MB_ICONINFORMATION);
        return 0;
    }
    
    // Comando para editar el registro
    char command[] = "reg add \"HKEY_LOCAL_MACHINE\\SOFTWARE\\Policies\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\" /v \"ProxySettingsPerUser\" /t REG_DWORD /d 1 /f";
    
    // Ejecutar el comando
    int result = system(command);
    
    if (result == 0) {
        MessageBoxW(NULL, L"Registro modificado con éxito.", L"Información", MB_OK | MB_ICONINFORMATION);
    } else {
        MessageBoxW(NULL, L"Error al modificar el registro. Asegúrate de ejecutar el programa como administrador.", L"Error", MB_OK | MB_ICONERROR);
    }
    
    return result;
}
