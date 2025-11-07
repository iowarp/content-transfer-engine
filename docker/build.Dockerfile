# Dockerfile for building the Content Transfer Engine (CTE)
# Inherits from iowarp/iowarp-runtime-build:latest which contains all build dependencies

FROM iowarp/iowarp-runtime-build:latest

# Set working directory
WORKDIR /workspace

# Copy the entire CTE source tree
COPY . /workspace/

# Configure and build CTE, installing to both /usr/local and /cte
RUN sudo chown -R $(whoami):$(whoami) /workspace && \
    mkdir -p build && \
    cd build && \
    cmake .. \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=/usr/local \
        -DCTE_ENABLE_CUDA=OFF \
        -DCTE_ENABLE_ROCM=OFF \
        -DCTE_ENABLE_CMAKE_DOTENV=OFF \
        -DWRP_CTE_ENABLE_PYTHON=OFF && \
    make -j$(nproc) && \
    sudo make install && \
    cmake .. -DCMAKE_INSTALL_PREFIX=/cte && \
    sudo make install && \
    rm -rf /workspace

# Add iowarp-cte to Spack configuration
RUN echo "  iowarp-cte:" >> ~/.spack/packages.yaml && \
    echo "    externals:" >> ~/.spack/packages.yaml && \
    echo "    - spec: iowarp-cte@main" >> ~/.spack/packages.yaml && \
    echo "      prefix: /usr/local" >> ~/.spack/packages.yaml && \
    echo "    buildable: false" >> ~/.spack/packages.yaml
