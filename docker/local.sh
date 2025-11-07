#!/bin/bash

# Build iowarp-runtime Docker image

# Get the project root directory (parent of docker folder)
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJECT_ROOT="$( cd "${SCRIPT_DIR}/.." && pwd )"

# Build the Docker image
docker build --no-cache -t iowarp/context-transfer-engine-build:latest -f "${SCRIPT_DIR}/local.Dockerfile" "${PROJECT_ROOT}"
