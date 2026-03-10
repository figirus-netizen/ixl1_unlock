Вмешательство в NVM сетевой карты может привести к ее полной неработоспособности. 
Все действия выполняются на свой страх и риск. Автор не несет ответственности за кирпичи.
Я несколько раз убивал карту, получилось её восстановить перепрошивкой через утилиту nvmupdate64e, так же она начала определяться как 4 портовая карта и получилось её восстановить через UEFI shell.

Исследование проводил на:
uname -a
FreeBSD brdr3 14.3-RELEASE FreeBSD 14.3-RELEASE GENERIC amd64

dmesg | grep ixl | head -5
ixl0: <Intel(R) Ethernet Controller XL710 for 40GbE QSFP+ - 2.3.3-k> mem 0x60800000-0x60ffffff,0x61008000-0x6100ffff at device 0.0 on pci1
ixl0: fw 9.156.79020 api 1.15 nvm 9.56 etid 80010101 oem 1.271.0
ixl0: PF-ID[0]: VFs 64, MSI-X 129, VF MSI-X 5, QPs 768, I2C
ixl0: Using 1024 TX descriptors and 1024 RX descriptors
ixl0: Using 24 RX queues 24 TX queues

pciconf -lv | grep -A 10 ixl
ixl0@pci0:1:0:0:        class=0x020000 rev=0x01 hdr=0x00 vendor=0x8086 device=0x1583 subvendor=0x8086 subdevice=0x0002
    vendor     = 'Intel Corporation'
    device     = 'Ethernet Controller XL710 for 40GbE QSFP+'
    class      = network
    subclass   = ethernet
ixl1@pci0:1:0:1:        class=0x020000 rev=0x01 hdr=0x00 vendor=0x8086 device=0x1583 subvendor=0x8086 subdevice=0x0000
    vendor     = 'Intel Corporation'
    device     = 'Ethernet Controller XL710 for 40GbE QSFP+'
    class      = network
    subclass   = ethernet



Инструкция по ручной разблокировке порта ixl1 на Intel XL710

Проблема
Утилита ixl_unlock от bu7cher после выполнения (-u) разблокировала только первый порт (ixl0). Второй порт (ixl1) остался заблокирован.

Диагностика
Путём анализа дампов NVM установлено:

Адрес 0x69cc (LAN 0, порт ixl0) содержит значение 0x0310 — бит 11 сброшен (разблокирован)
Адрес 0x69da (LAN 1, порт ixl1) содержит значение 0x0b10 — бит 11 установлен (заблокирован)

Решение
Принудительно записать значение 0x0310 по адресу 0x69da, сбросив тем самым 11-й бит (отключение проверки квалификации модуля для второго порта).

Проверить состояние порта ixl0 (должно быть 0x0310)
./ixl_unlock -l ixl0 0x69cc 1

Проверить состояние порта ixl1 (скорее всего 0x0b10)
./ixl_unlock -l ixl1 0x69da 1

Если значение отличается от 0x0310 требуется запись.

Компиляция
cc -o fix_lan1 fix_lan1.c

Запуск
./fix_lan1

Перезагрузка машины 

P/S
Я не проверил но скорее всего можно указать адрес 0x69cc для модификации ixl0

Проверка работы
После установки значения 0x0310 на модули подаётся ток и в ifconfig видно что есть ток на TX и мощность на RX.

Если не сработало то будет 
        lane 1: RX power: 0.00 mW (-inf dBm) TX bias: 0.00 mA
        lane 2: RX power: 0.00 mW (-inf dBm) TX bias: 0.00 mA
        lane 3: RX power: 0.00 mW (-inf dBm) TX bias: 0.00 mA
        lane 4: RX power: 0.00 mW (-inf dBm) TX bias: 0.00 mA

 ifconfig -v

ixl0: flags=1008843<UP,BROADCAST,RUNNING,SIMPLEX,MULTICAST,LOWER_UP> metric 0 mtu 1500
        options=4e507bb<RXCSUM,TXCSUM,VLAN_MTU,VLAN_HWTAGGING,JUMBO_MTU,VLAN_HWCSUM,TSO4,TSO6,LRO,VLAN_HWFILTER,VLAN_HWTSO,RXCSUM_IPV6,TXCSUM_IPV6,HWSTATS,MEXTPG>
        ether 3c:fd:fe:9e:c8:ec
        inet 10.0.0.10 netmask 0xffffff00 broadcast 10.0.0.255
        media: Ethernet autoselect (40Gbase-LR4 <full-duplex>)
        status: active
        nd6 options=29<PERFORMNUD,IFDISABLED,AUTO_LINKLOCAL>
        drivername: ixl0
        plugged: QSFP+ 40GBASE-LR4 (LC)
        vendor: OEM PN: QSFP-40G-LR4 SN: 14724I142002 DATE: 2013-01-01
        module temperature: 29.88 C voltage: 3.29 Volts
        lane 1: RX power: 0.89 mW (-0.49 dBm) TX bias: 27.31 mA
        lane 2: RX power: 0.63 mW (-1.98 dBm) TX bias: 27.46 mA
        lane 3: RX power: 0.57 mW (-2.44 dBm) TX bias: 27.60 mA
        lane 4: RX power: 0.78 mW (-1.10 dBm) TX bias: 27.46 mA
ixl1: flags=1008843<UP,BROADCAST,RUNNING,SIMPLEX,MULTICAST,LOWER_UP> metric 0 mtu 1500
        options=4e507bb<RXCSUM,TXCSUM,VLAN_MTU,VLAN_HWTAGGING,JUMBO_MTU,VLAN_HWCSUM,TSO4,TSO6,LRO,VLAN_HWFILTER,VLAN_HWTSO,RXCSUM_IPV6,TXCSUM_IPV6,HWSTATS,MEXTPG>
        ether 3c:fd:fe:9e:c8:ee
        inet 10.0.0.1 netmask 0xffffff00 broadcast 10.0.0.255
        fib: 1
        media: Ethernet autoselect (40Gbase-LR4 <full-duplex>)
        status: active
        nd6 options=29<PERFORMNUD,IFDISABLED,AUTO_LINKLOCAL>
        drivername: ixl1
        plugged: QSFP+ 40GBASE-LR4 (LC)
        vendor: OEM PN: QSFP-40G-LR4 SN: 14724I142001 DATE: 2013-01-01
        module temperature: 29.88 C voltage: 3.29 Volts
        lane 1: RX power: 0.73 mW (-1.35 dBm) TX bias: 27.29 mA
        lane 2: RX power: 0.92 mW (-0.38 dBm) TX bias: 27.27 mA
        lane 3: RX power: 0.74 mW (-1.30 dBm) TX bias: 27.36 mA
        lane 4: RX power: 0.78 mW (-1.08 dBm) TX bias: 27.37 mA

Так же попробовал модули на 100G, видно что сетевая прочитала EEPROM но по всей видимости не может согласовать медиа из списка сетевой карты.

        supported media:
                media autoselect
                media 40Gbase-LR4
                media 40Gbase-SR4
                media 40Gbase-CR4


ifconfig -v
ixl0: flags=8843<UP,BROADCAST,RUNNING,SIMPLEX,MULTICAST> metric 0 mtu 1500
        options=4e507bb<RXCSUM,TXCSUM,VLAN_MTU,VLAN_HWTAGGING,JUMBO_MTU,VLAN_HWCSUM,TSO4,TSO6,LRO,VLAN_HWFILTER,VLAN_HWTSO,RXCSUM_IPV6,TXCSUM_IPV6,HWSTATS,MEXTPG>
        ether 3c:fd:fe:9e:c8:ec
        inet 10.0.0.10 netmask 0xffffff00 broadcast 10.0.0.255
        media: Ethernet autoselect
        status: no carrier
        nd6 options=29<PERFORMNUD,IFDISABLED,AUTO_LINKLOCAL>
        drivername: ixl0
        plugged: QSFP28 100G AOC (Active Optical Cable) (No separable connector)
        vendor: FINISAR CORP PN: FCBN425QE2C10 SN: W69A532 DATE: 2021-08-26
        module temperature: 44.93 C voltage: 3.26 Volts
        lane 1: RX power: 0.79 mW (-1.01 dBm) TX bias: 7.56 mA
        lane 2: RX power: 0.82 mW (-0.88 dBm) TX bias: 7.43 mA
        lane 3: RX power: 0.81 mW (-0.93 dBm) TX bias: 7.56 mA
        lane 4: RX power: 0.78 mW (-1.06 dBm) TX bias: 7.49 mA
ixl1: flags=8843<UP,BROADCAST,RUNNING,SIMPLEX,MULTICAST> metric 0 mtu 1500
        options=4e507bb<RXCSUM,TXCSUM,VLAN_MTU,VLAN_HWTAGGING,JUMBO_MTU,VLAN_HWCSUM,TSO4,TSO6,LRO,VLAN_HWFILTER,VLAN_HWTSO,RXCSUM_IPV6,TXCSUM_IPV6,HWSTATS,MEXTPG>
        ether 3c:fd:fe:9e:c8:ee
        inet 10.0.0.1 netmask 0xffffff00 broadcast 10.0.0.255
        fib: 1
        media: Ethernet autoselect
        status: no carrier
        nd6 options=29<PERFORMNUD,IFDISABLED,AUTO_LINKLOCAL>
        drivername: ixl1
        plugged: QSFP28 100G AOC (Active Optical Cable) (No separable connector)
        vendor: FINISAR CORP PN: FCBN425QE2C10 SN: W69A532 DATE: 2021-08-26
        module temperature: 44.37 C voltage: 3.27 Volts
        lane 1: RX power: 0.81 mW (-0.92 dBm) TX bias: 7.56 mA
        lane 2: RX power: 0.79 mW (-1.04 dBm) TX bias: 7.56 mA
        lane 3: RX power: 0.80 mW (-0.99 dBm) TX bias: 7.56 mA
        lane 4: RX power: 0.79 mW (-1.04 dBm) TX bias: 7.56 mA


## Благодарности
- @bu7cher за оригинальную утилиту ixl_unlock
- @bluedogs за форк
- Сообществу FreeBSD за драйвер ixl и возможность ковыряться в NVM
