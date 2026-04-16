# 🎯 Mini Switch Demo Guide

Это демо показывает **реальную работу L2/L3 свитча** с MAC learning, ARP, VLAN и визуализацией пакетов в реальном времени.

## 📋 Что показывает демо

- **L2 Forwarding** - пересылка пакетов между портами
- **MAC Learning** - автоматическое изучение MAC-адресов  
- **ARP Processing** - обработка ARP запросов/ответов
- **VLAN Support** - поддержка 802.1Q тегов
- **ICMP Ping** - ответы на ping запросы
- **Multi-port Threading** - работа на нескольких интерфейсах
- **Live Visualization** - цветной вывод пакетов и таблиц

## � Быстрый запуск

```bash
# Собрать проект
make

# Запустить демо (требуются sudo права)
make demo
```

## 🖥 Топология демо

```
ns1(veth1) ──┐
             │  mini_switch
ns2(veth2) ──┘
```

- **ns1**: Network namespace с IP 10.0.0.1/24
- **ns2**: Network namespace с IP 10.0.0.2/24  
- **mini_switch**: Программный свитч между veth_sw1 и veth_sw2

## 🎨 Цветовая визуализация

Во время работы свитча вы увидите цветной вывод пакетов:

- 🟢 **[L2]** - известный MAC, прямая пересылка
- 🟡 **[FLOOD]** - неизвестный MAC, рассылка на все порты
- 🔵 **[ARP]** - ARP пакет (запрос/ответ)
- 🟣 **[VLAN 100]** - VLAN тегированный пакет

## 📊 CLI команды

Во время работы демо доступны команды:

```bash
mini-switch> show mac    # Показать MAC таблицу
mini-switch> show arp    # Показать ARP таблицу  
mini-switch> help        # Все команды
mini-switch> exit        # Выход
```

## 🧪 Тестирование

### Базовый тест
```bash
# Из другого терминала
ip netns exec ns1 ping -c 3 10.0.0.2
ip netns exec ns2 ping -c 3 10.0.0.1
```

### Непрерывный пинг (для визуализации)
```bash
ip netns exec ns1 ping 10.0.0.2
```

### VLAN тест (опционально)
```bash
# Создать VLAN интерфейсы
ip link add link veth1 name veth1.100 type vlan id 100
ip link add link veth2 name veth2.100 type vlan id 100

ip -n ns1 addr add 10.0.100.1/24 dev veth1.100
ip -n ns2 addr add 10.0.100.2/24 dev veth2.100

ip link set veth1.100 up
ip link set veth2.100 up

# Пинг через VLAN
ip -n ns1 ping -c 2 10.0.100.2
```

## 📈 Пример вывода

```
=== Mini Switch Demo Setup ===
Setting up network namespaces...
Network namespaces setup complete.
ns1: 10.0.0.1/24 (veth1 <-> veth_sw1)
ns2: 10.0.0.2/24 (veth2 <-> veth_sw2)

Starting mini-switch...
You should see colored packet output and MAC/ARP tables.

Mini Switch - L2/L3 Software Switch
=====================================
Port 0: veth_sw1 (index 5)
Port 1: veth_sw2 (index 6)
Switch running... Press Ctrl+C to stop
Type 'help' for available commands

[ARP] Port 0 -> Port 1 | 02:42:AC:11:00:02 -> 02:42:AC:11:00:03
[L2]  Port 1 -> Port 0 | 02:42:AC:11:00:03 -> 02:42:AC:11:00:02
[FLOOD] Port 0 -> ALL | FF:FF:FF:FF:FF:FF -> 02:42:AC:11:00:02

=== MAC Table ===
Port 0: 02:42:AC:11:00:02
Port 1: 02:42:AC:11:00:03
================

=== ARP Table ===
10.0.0.2 -> 02:42:AC:11:00:03
10.0.0.1 -> 02:42:AC:11:00:02
================
```

## 🎬 Для собеседования

### Split-screen демо
Откройте 3 терминала:
1. **Левый**: `ip netns exec ns1 bash`
2. **Правый**: `ip netns exec ns2 bash`  
3. **Нижний**: `sudo ./mini_switch veth_sw1 veth_sw2`

### Запись демо
```bash
# Установить asciinema
pip install asciinema

# Записать демо
asciinema rec mini_switch_demo.cast make demo

# Конвертировать в GIF
asciinema upload mini_switch_demo.cast
```

## 🔧 Ручной запуск

Если нужно запустить пошагово:

```bash
# 1. Создать namespace
ip netns add ns1
ip netns add ns2

# 2. Создать veth пары
ip link add veth1 type veth peer name veth_sw1
ip link add veth2 type veth peer name veth_sw2

# 3. Настроить интерфейсы
ip link set veth1 netns ns1
ip link set veth2 netns ns2
ip -n ns1 addr add 10.0.0.1/24 dev veth1
ip -n ns2 addr add 10.0.0.2/24 dev veth2
ip -n ns1 link set veth1 up
ip -n ns2 link set veth2 up
ip link set veth_sw1 up
ip link set veth_sw2 up

# 4. Запустить свитч
sudo ./mini_switch veth_sw1 veth_sw2

# 5. Тестировать
ip netns exec ns1 ping 10.0.0.2
```

## 🧹 Очистка

```bash
# Автоматическая (Ctrl+C во время демо)
# Или вручную:
pkill -f mini_switch
ip netns del ns1 ns2
ip link del veth_sw1 veth_sw2
```

## 💡 Ключевые особенности для демонстрации

1. **Real-time packet processing** - пакеты обрабатываются мгновенно
2. **MAC learning** - свитч запоминает где какие MAC-адреса
3. **ARP resolution** - автоматическое определение IP->MAC
4. **Thread safety** - многопоточная обработка без гонок
5. **Production-ready architecture** - модульная структура кода

---

**Готово для собеседования!** 🎉
