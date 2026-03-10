#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/sockio.h>
#include <sys/stdint.h>

#include <net/if.h>

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <err.h>

#define I40E_NVM_ACCESS \
     (((((((('E' << 4) + '1') << 4) + 'K') << 4) + 'G') << 4) | 5)

#define I40E_NVM_READ   0xB
#define I40E_NVM_WRITE  0xC

#define I40E_NVM_TRANS_SHIFT    8
#define I40E_NVM_SA             (0x1 | 0x2)  /* I40E_NVM_SNT | I40E_NVM_LCB */
#define I40E_NVM_CSUM           0x8

struct i40e_nvm_access {
    uint32_t command;
    uint32_t config;
    uint32_t offset;
    uint32_t data_size;
    uint8_t data[1];
};

int main(int argc, char **argv) {
    struct ifdrv req;
    struct i40e_nvm_access *nvm;
    uint16_t value;
    int s;
    
    printf("IXL1 Fix - устанавливаем 0x0310 в LAN 1\n");
    
    // Создаем сокет
    s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s == -1)
        err(1, "socket");
    
    // Выделяем память для NVM access
    nvm = calloc(1, sizeof(*nvm) + sizeof(uint16_t));
    if (nvm == NULL)
        err(2, "calloc");
    
    // Подготавливаем запрос для ixl1
    memset(&req, 0, sizeof(req));
    strlcpy(req.ifd_name, "ixl1", sizeof(req.ifd_name));
    req.ifd_cmd = I40E_NVM_ACCESS;
    req.ifd_len = sizeof(*nvm) + sizeof(uint16_t);
    req.ifd_data = nvm;
    
    // АДРЕС LAN 1 Misc0 для ixl1 (0x69da)
    printf("Читаем текущее значение по адресу 0x69da...\n");
    
    nvm->command = I40E_NVM_READ;
    nvm->config = I40E_NVM_SA << I40E_NVM_TRANS_SHIFT;
    nvm->offset = 0x69da * sizeof(uint16_t);
    nvm->data_size = sizeof(uint16_t);
    
    if (ioctl(s, SIOCGDRVSPEC, &req) == -1)
        err(3, "ioctl read");
    
    value = *(uint16_t *)nvm->data;
    printf("Текущее значение: 0x%04x\n", value);
    
    // Устанавливаем 0x0310 (сбрасываем бит 11)
    value = 0x0310;
    printf("Новое значение: 0x%04x\n", value);
    
    // Записываем новое значение
    printf("Записываем...\n");
    
    nvm->command = I40E_NVM_WRITE;
    nvm->config = I40E_NVM_SA << I40E_NVM_TRANS_SHIFT;
    nvm->offset = 0x69da * sizeof(uint16_t);
    nvm->data_size = sizeof(uint16_t);
    *(uint16_t *)nvm->data = value;
    
    if (ioctl(s, SIOCSDRVSPEC, &req) == -1)
        err(4, "ioctl write");
    
    printf("Записано успешно!\n");
    
    // Проверяем, что записалось
    printf("Проверяем запись...\n");
    
    nvm->command = I40E_NVM_READ;
    nvm->config = I40E_NVM_SA << I40E_NVM_TRANS_SHIFT;
    nvm->offset = 0x69da * sizeof(uint16_t);
    nvm->data_size = sizeof(uint16_t);
    
    if (ioctl(s, SIOCGDRVSPEC, &req) == -1)
        err(5, "ioctl verify");
    
    value = *(uint16_t *)nvm->data;
    printf("Значение после записи: 0x%04x\n", value);
    
    // Обновляем контрольную сумму NVM
    printf("Обновляем контрольную сумму NVM...\n");
    
    nvm->command = I40E_NVM_WRITE;
    nvm->config = (I40E_NVM_SA | I40E_NVM_CSUM) << I40E_NVM_TRANS_SHIFT;
    nvm->offset = 0;
    nvm->data_size = sizeof(uint16_t);
    *(uint16_t *)nvm->data = 0;
    
    if (ioctl(s, SIOCSDRVSPEC, &req) == -1)
        err(6, "ioctl csum");
    
    printf("Контрольная сумма обновлена!\n");
    printf("ГОТОВО. Теперь перезагрузите сервер.\n");
    
    close(s);
    free(nvm);
    return 0;
}