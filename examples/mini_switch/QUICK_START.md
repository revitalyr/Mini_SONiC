# Quick Start Guide

## 🚀 Быстрый запуск

### 1. Сборка проекта
```bash
cd mini_switch
bash build.sh
```

### 2. Настройка демо-топологии
```bash
bash scripts/netns_demo.sh
```

### 3. Запуск свитча
```bash
sudo ./bin/mini_switch veth1 veth2
```

### 4. Тестирование
```bash
# Терминал 1
sudo ip netns exec ns1 ping 10.0.0.2

# Терминал 2  
sudo ip netns exec ns2 ping 10.0.0.1
```

## 📊 Что вы видите

При запуске свитч покажет:
- Конфигурацию портов
- MAC learning сообщения
- ARP таблицу
- Forwarding статистику

## 🔧 Очистка
```bash
sudo ip netns del ns1
sudo ip netns del ns2
sudo ip link del veth1
```

---
**Готово для демонстрации на собеседовании!**
