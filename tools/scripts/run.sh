#!/bin/bash

# Скрипт для сборки и запуска Mini SONiC
# Использование: ./scripts/run.sh {app|test|clean}

set -e # Остановить выполнение при ошибке

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_ROOT/build"

function check_dependencies() {
    echo "== Проверка зависимостей =="
    
    for cmd in cmake g++ make; do
        if ! command -v $cmd &> /dev/null; then
            echo "[ОШИБКА] Инструмент $cmd не найден. Установите его (sudo apt install build-essential cmake)."
            exit 1
        fi
    done

    if [[ "$OSTYPE" == "linux-gnu"* ]] && [ ! -f /usr/include/numa.h ]; then
        echo "[ОШИБКА] Библиотека libnuma-dev не найдена. Установите её (sudo apt install libnuma-dev)."
        exit 1
    fi
    echo "Зависимости подтверждены."
}

function build_project() {
    check_dependencies
    echo "== Сборка проекта в $BUILD_DIR =="
    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"

    # Определяем количество ядер для сборки (Linux/macOS)
    if [[ "$OSTYPE" == "darwin"* ]]; then
        NPROC=$(sysctl -n hw.ncpu)
    else
        NPROC=$(nproc)
    fi

    if ! cmake ..; then
        echo "[ПРЕДУПРЕЖДЕНИЕ] Ошибка конфигурации (возможно конфликт генераторов). Очистка build..."
        rm -rf ./*
        cmake ..
    fi

    cmake --build . -j $NPROC
    cd - > /dev/null
}

function run_app() {
    build_project
    echo "== Запуск приложения Mini SONiC =="
    # Мы можем добавить фильтрацию логов через grep, если нужно
    "$BUILD_DIR/mini_sonic"
}

function run_tests() {
    build_project
    echo "== Запуск тестов =="
    if [ -f "$BUILD_DIR/mini_sonic_tests" ]; then
        "$BUILD_DIR/mini_sonic_tests"
    else
        echo "[ОШИБКА] Бинарный файл тестов не найден. Убедитесь, что тесты описаны в CMakeLists.txt"
        exit 1
    fi
}

case "$1" in
    app)
        run_app
        ;;
    test)
        run_tests
        ;;
    clean)
        echo "== Очистка сборки =="
        rm -rf "$BUILD_DIR"
        ;;
    *)
        echo "Использование: $0 {app|test|clean}"
        exit 1
        ;;
esac