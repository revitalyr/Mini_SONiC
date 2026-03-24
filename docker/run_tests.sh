#!/bin/bash

# Exit immediately if a command exits with a non-zero status (except where handled)
set -e

# Determine the project root directory relative to this script
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

echo "=== Mini SONiC Unit Tests ==="
echo "Project Root: $PROJECT_ROOT"

# Change to project root to run docker-compose commands
cd "$PROJECT_ROOT"

echo "1. Building test environment..."
# Build specifically the 'tests' service to ensure latest code is used
docker-compose -f docker/docker-compose.yml build tests

echo "2. Running tests..."
# Temporarily disable 'set -e' to capture the test exit code
set +e

# Run the tests container and remove it afterwards
docker-compose -f docker/docker-compose.yml run --rm tests
TEST_EXIT_CODE=$?

set -e

if [ $TEST_EXIT_CODE -eq 0 ]; then
    echo "=== SUCCESS: All tests passed ==="
else
    echo "=== FAILURE: Tests failed with exit code $TEST_EXIT_CODE ==="
fi

exit $TEST_EXIT_CODE