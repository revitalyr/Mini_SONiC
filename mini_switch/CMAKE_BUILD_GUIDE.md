# CMake Build Guide for SONiC Mini Switch

## 🏗️ **Почему CMake вместо Makefile?**

### **Преимущества CMake:**

#### **1. Кросс-платформенность**
```bash
# Windows (Visual Studio)
cmake -G "Visual Studio 16 2019" -A x64 ..

# Windows (MinGW)
cmake -G "MinGW Makefiles" ..

# Linux (GCC)
cmake -G "Unix Makefiles" ..

# macOS (Xcode)
cmake -G "Xcode" ..
```

#### **2. Автоматическое управление зависимостями**
```bash
# Автоматический поиск библиотек
find_package(Redis REQUIRED)
find_package(Threads REQUIRED)

# Условная компиляция
if(REDIS_FOUND)
    target_compile_definitions(sonic_core PUBLIC HAVE_REDIS)
endif()
```

#### **3. Современная система сборки**
- **Out-of-source build**: Исходники остаются чистыми
- **Множественные конфигурации**: Debug/Release одновременно
- **Интеграция с IDE**: Visual Studio, CLion, VSCode
- **Пакетирование**: Автоматическое создание DEB/RPM/NSIS

---

## 🚀 **Быстрый старт**

### **Базовая сборка**
```bash
# Создаем директорию сборки
mkdir build && cd build

# Конфигурация
cmake ..

# Сборка
cmake --build .

# Запуск
./bin/mini_sonic_switch
```

### **Сборка с опциями**
```bash
# Debug версия с тестами
cmake -DCMAKE_BUILD_TYPE=Debug -DSONIC_BUILD_TESTS=ON ..

# Release версия с документацией
cmake -DCMAKE_BUILD_TYPE=Release -DSONIC_BUILD_DOCS=ON ..

# Demo версия для веб-интерфейса
cmake -DSONIC_BUILD_DEMO=ON -DSONIC_ENABLE_DEBUG=ON ..
```

---

## ⚙️ **Опции сборки**

### **Основные опции:**
| Опция | Значение по умолчанию | Описание |
|--------|---------------------|----------|
| `SONIC_BUILD_DEMO` | OFF | Собрать демо-версию |
| `SONIC_BUILD_TESTS` | ON | Собрать тесты |
| `SONIC_BUILD_DOCS` | OFF | Генерировать документацию |
| `SONIC_ENABLE_DEBUG` | OFF | Включить отладочные функции |
| `SONIC_INSTALL_SYSTEM` | OFF | Устанавливать в систему |

### **Опции отладки:**
| Опция | Описание |
|--------|----------|
| `SONIC_ENABLE_COVERAGE` | Покрытие кода тестами |
| `SONIC_ENABLE_ASAN` | AddressSanitizer для поиска ошибок памяти |
| `SONIC_ENABLE_TSAN` | ThreadSanitizer для поиска ошибок потоков |
| `SONIC_STATIC_ANALYSIS` | Статический анализ кода |

---

## 🛠️ **Примеры использования**

### **1. Разработка с отладкой**
```bash
mkdir build && cd build

# Конфигурация для разработки
cmake -DCMAKE_BUILD_TYPE=Debug \
      -DSONIC_ENABLE_DEBUG=ON \
      -DSONIC_ENABLE_ASAN=ON \
      -DSONIC_BUILD_TESTS=ON \
      -DSONIC_STATIC_ANALYSIS=ON \
      ..

# Сборка
cmake --build . --parallel

# Запуск тестов
ctest --verbose

# Статический анализ
make cppcheck
```

### **2. Production сборка**
```bash
mkdir build && cd build

# Конфигурация для production
cmake -DCMAKE_BUILD_TYPE=Release \
      -DSONIC_BUILD_DOCS=ON \
      -DSONIC_INSTALL_SYSTEM=ON \
      ..

# Сборка
cmake --build . --config Release

# Установка
cmake --install .
```

### **3. Кросс-платформенная сборка**
```bash
# Windows (Visual Studio)
cmake -G "Visual Studio 16 2019" -A x64 \
      -DCMAKE_BUILD_TYPE=Release \
      -DSONIC_BUILD_DEMO=ON \
      ..

# Сборка
cmake --build . --config Release

# macOS (Xcode)
cmake -G Xcode \
      -DCMAKE_BUILD_TYPE=Release \
      -DSONIC_BUILD_TESTS=ON \
      ..

# Сборка
cmake --build . --config Release
```

---

## 📦 **Пакетирование**

### **Автоматическое создание пакетов**
```bash
# Конфигурация для пакетирования
cmake -DCMAKE_BUILD_TYPE=Release \
      -DCPACK_GENERATOR="DEB;RPM" \
      ..

# Создание пакетов
cpack

# Специфичные пакеты
cpack -G DEB    # Debian package
cpack -G RPM    # RPM package
cpack -G NSIS   # Windows installer
cpack -G TGZ    # Tarball
```

### **Настройка пакетов**
```cmake
# В CMakeLists.txt
set(CPACK_PACKAGE_NAME "SONiC_Mini_Switch")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY ${PROJECT_DESCRIPTION})

# Debian
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6, libpthread0")
set(CPACK_DEBIAN_PACKAGE_SECTION "net")

# RPM
set(CPACK_RPM_PACKAGE_LICENSE "Apache-2.0")
set(CPACK_RPM_PACKAGE_GROUP "Applications/System")
```

---

## 🧪 **Тестирование**

### **Запуск тестов**
```bash
# Все тесты
ctest

# Подробный вывод
ctest --verbose

# Конкретный тест
ctest -R TestFdbOrch

# Тесты с покрытием
cmake -DSONIC_ENABLE_COVERAGE=ON ..
make run_tests
lcov --capture --directory . --output-file coverage.info
genhtml coverage.info --output-directory coverage_html
```

### **Интеграция с CI/CD**
```yaml
# .github/workflows/ci.yml
name: CI
on: [push, pull_request]

jobs:
  build:
    runs-on: ubuntu-latest
    steps:
    - uses: actions/checkout@v2
    
    - name: Configure CMake
      run: |
        cmake -DCMAKE_BUILD_TYPE=Release \
              -DSONIC_BUILD_TESTS=ON \
              -DSONIC_ENABLE_COVERAGE=ON \
              .
    
    - name: Build
      run: cmake --build .
    
    - name: Test
      run: ctest --output-on-failure
    
    - name: Coverage
      run: |
        lcov --capture --directory . --output-file coverage.info
        bash <(curl -s https://codecov.io/bash) -f coverage.info
```

---

## 🔍 **Интеграция с IDE**

### **Visual Studio Code**
```json
// .vscode/settings.json
{
    "cmake.configureOnOpen": true,
    "cmake.buildDirectory": "${workspaceFolder}/build",
    "cmake.generator": "Unix Makefiles",
    "CMAKE_BUILD_TYPE": "Debug"
}
```

### **CLion**
```bash
# CLion автоматически обнаружит CMakeLists.txt
# Настройки: Settings → Build, Execution, Deployment → CMake
```

### **Visual Studio**
```bash
# Генерация проекта
cmake -G "Visual Studio 16 2019" -A x64 ..

# Открытие .sln файла
start SONiC_Mini_Switch.sln
```

---

## 📊 **Сравнение CMake vs Makefile**

| Критерий | Makefile | CMake |
|----------|----------|-------|
| **Простота** | ✅ Прост для небольших проектов | ⚠️ Сложнее для новичков |
| **Масштабируемость** | ❌ Плохо масштабируется | ✅ Отлично масштабируется |
| **Кросс-платформенность** | ❌ Только Unix-like | ✅ Windows/Linux/macOS |
| **Зависимости** | ❌ Ручное управление | ✅ Автоматический поиск |
| **IDE интеграция** | ❌ Ограниченная | ✅ Полная поддержка |
| **Пакетирование** | ❌ Нет поддержки | ✅ Встроенная поддержка |
| **Тестирование** | ❌ Ручная настройка | ✅ Встроенная поддержка |
| **Документация** | ❌ Ручная настройка | ✅ Интеграция с Doxygen |

---

## 🎯 **Когда использовать CMake:**

### **✅ Идеально для:**
- **Крупных проектов** (>50 файлов)
- **Кросс-платформенной разработки**
- **Командной разработки**
- **CI/CD пайплайнов**
- **Распространения пакетов**
- **Интеграции с IDE**

### **❌ Избыточно для:**
- **Малых проектов** (<10 файлов)
- **Простых утилит**
- **Быстрых прототипов**
- **Образовательных примеров**

---

## 🚀 **Рекомендации для SONiC Mini Switch:**

### **Для разработки:**
```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Debug \
      -DSONIC_ENABLE_DEBUG=ON \
      -DSONIC_ENABLE_ASAN=ON \
      -DSONIC_BUILD_TESTS=ON \
      ..
cmake --build . --parallel
```

### **Для production:**
```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release \
      -DSONIC_BUILD_DOCS=ON \
      -DSONIC_INSTALL_SYSTEM=ON \
      ..
cmake --build . --config Release
cmake --install .
```

### **Для демо:**
```bash
mkdir build && cd build
cmake -DSONIC_BUILD_DEMO=ON \
      -DSONIC_ENABLE_DEBUG=ON \
      ..
cmake --build .
./bin/demo_switch
```

---

## 📝 **Заключение**

**CMake - правильный выбор для SONiC Mini Switch потому что:**

1. **Масштабируемость**: Проект растет, CMake справится
2. **Профессиональный стандарт**: Используется в крупных проектах
3. **Кросс-платформенность**: Работает везде
4. **Интеграция**: Отлично работает с современными инструментами
5. **Сообщество**: Большая поддержка и документация

**Переход с Makefile на CMake - инвестиция в будущее проекта!** 🚀
