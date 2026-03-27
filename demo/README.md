# Mini SONiC Real Network Demo

Это демонстрация использует **реальные** компоненты скомпилированного проекта Mini SONiC для показа работоспособности системы, а не имитацию.

## Что делает демо

Демо создает реальную сеть коммутаторов используя:
- **Реальные** классы Mini SONiC (`SimulatedSai`, `Pipeline`, `PipelineThread`, `SPSCQueue`)
- **Настоящие** пакеты в формате Mini SONiC (`DataPlane::Packet`)
- **Реальные** процедуры обработки L2/L3 сервисов
- **Настоящие** потоки обработки пакетов

## Архитектура демо

```
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│   RealSW1       │    │   RealSW2       │    │   RealSW3       │
│                 │    │                 │    │                 │
│ ┌─────────────┐ │    │ ┌─────────────┐ │    │ ┌─────────────┐ │
│ │SimulatedSai │ │    │ │SimulatedSai │ │    │ │SimulatedSai │ │
│ └─────────────┘ │    │ └─────────────┘ │    │ └─────────────┘ │
│ ┌─────────────┐ │    │ ┌─────────────┐ │    │ ┌─────────────┐ │
│ │  Pipeline   │ │    │ │  Pipeline   │ │    │ │  Pipeline   │ │
│ └─────────────┘ │    │ └─────────────┘ │    │ └─────────────┘ │
│ ┌─────────────┐ │    │ ┌─────────────┐ │    │ ┌─────────────┐ │
│ │SPSCQueue    │◄┼────┼──►│SPSCQueue    │◄┼────┼──►│SPSCQueue    │ │
│ └─────────────┘ │    │ └─────────────┘ │    │ └─────────────┘ │
│ ┌─────────────┐ │    │ ┌─────────────┐ │    │ ┌─────────────┐ │
│ │PipelineThrd │ │    │ │PipelineThrd │ │    │ │PipelineThrd │ │
│ └─────────────┘ │    │ └─────────────┘ │    │ └─────────────┘ │
└─────────────────┘    └─────────────────┘    └─────────────────┘
```

## Сборка и запуск

### Предварительные требования

1. Скомпилировать основной проект Mini SONiC:
   ```bash
   cd ..
   cmake --build build --config Release
   ```

2. Убедиться что `build/mini_sonic_lib.lib` существует

### Быстрый запуск

Windows:
```cmd
cd demo
build_and_run_demo.bat
```

### Ручная сборка

```bash
cd demo
mkdir build && cd build
cmake -G "Ninja" -DCMAKE_BUILD_TYPE=Release -DMINI_SONIC_BUILD_DIR=../../build ..
cmake --build . --config Release
```

### Запуск демо

```bash
# Базовая конфигурация
./mini_sonic_real_demo --switches 3 --rate 100 --duration 30

# Стресс-тест
./mini_sonic_real_demo --switches 5 --rate 200 --duration 60

# Расширенная конфигурация
./mini_sonic_real_demo --switches 8 --rate 100 --duration 120
```

## Параметры командной строки

- `--switches <n>`: Количество коммутаторов (по умолчанию: 3)
- `--rate <n>`: Пакетов в секунду (по умолчанию: 100)
- `--duration <n>`: Длительность в секундах (по умолчанию: 30)
- `--no-viz`: Отключить визуализацию
- `--help`: Показать справку

## Что демонстрирует демо

### 1. Реальная обработка пакетов
- Генерируются настоящие пакеты `DataPlane::Packet`
- Пакеты обрабатываются через реальный `Pipeline`
- Используется настоящая очередь `SPSCQueue`
- Обработка в реальных потоках `PipelineThread`

### 2. L2/L3 функциональность
- **MAC Learning**: Коммутаторы изучают MAC-адреса
- **L2 Forwarding**: Пересылка на основе MAC-таблиц
- **IP Routing**: L3 маршрутизация (если доступно)
- **SAI Abstraction**: Работа через SAI интерфейс

### 3. Производительность
- **Throughput**: Измерение реальной пропускной способности
- **Latency**: Измерение задержки обработки
- **Queue Depth**: Мониторинг глубины очередей
- **Packet Loss**: Отслеживание потерь пакетов

### 4. Масштабируемость
- **Multi-threading**: Реальная многопоточная обработка
- **Lock-free Queues**: Безблокировочные очереди
- **Memory Management**: Управление памятью пакетов
- **Resource Usage**: Мониторинг использования ресурсов

## Пример вывода

```
=== Mini SONiC REAL Network Demo ===
Switches: 3
Packet Rate: 100 pps
Duration: 30 seconds
Log File: demo_output.log
=====================================

[12345] SW1: Initialized - Real Mini SONiC components loaded
[12346] SW2: Initialized - Real Mini SONiC components loaded  
[12347] SW3: Initialized - Real Mini SONiC components loaded

[12348] SW1: Started - Pipeline thread active
[12349] SW2: Started - Pipeline thread active
[12350] SW3: Started - Pipeline thread active

[12351] PKT0: GENERATOR -> SW1 (Generated packet)
[12352] PKT0: INGRESS -> RealSW1 (Packet received)
[12353] RealSW1: MAC Learning - Learned MAC 00:11:22:33:44:01 on port 5
[12354] PKT0: RealSW1 -> EGRESS (Processed in 45μs)

=== REAL-TIME STATISTICS ===
Packets Generated: 1500
Packets Processed: 1485  
Packets Dropped: 15
MAC Learning Events: 89
Route Lookups: 1245
Throughput: 98.50 pps
Average Latency: 47.23 μs
============================
```

## Логирование

Демо создает детальный лог-файл `demo_output.log` с:
- Временными метками всех событий
- Потоком пакетов через систему
- Статистикой производительности
- Событиями MAC learning
- Ошибками и потерями пакетов

## Валидация работоспособности

Демо доказывает работоспособность проекта путем:

1. **Компиляции**: Успешная сборка с реальными компонентами
2. **Исполнения**: Запуск реальных процедур обработки
3. **Интеграции**: Взаимодействие всех компонентов системы
4. **Производительности**: Измерение реальных метрик
5. **Масштабируемости**: Работа под нагрузкой

## Отличие от имитации

| Имитация (HTML) | Реальное демо |
|----------------|---------------|
| Визуальная анимация | Настоящая обработка пакетов |
| Симуляция алгоритмов | Вызов реальных функций |
| Фейковые метрики | Реальные измерения |
| JavaScript код | Скомпилированный C++ |
| Браузерное демо | Standalone приложение |

Это демо - **доказательство** того, что Mini SONiC действительно работает, а не просто красивая визуализация.
