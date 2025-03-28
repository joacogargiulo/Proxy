#include <stdio.h>
#include <stdlib.h>

int main() {
    // Comando para editar el registro
    char command[] = "reg add \"HKEY_LOCAL_MACHINE\\SOFTWARE\\Policies\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\" /v \"ProxySettingsPerUser\" /t REG_DWORD /d 1 /f";
    
    // Ejecutar el comando
    int result = system(command);
    
    return 0;
}
 