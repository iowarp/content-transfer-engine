# README 

This project includes CUDA-enabled documentation to get hermes and dependencies up and running. 

1. **Compile IOWarp(Chimaera) with CUDA-enabled support**\
    `cd ~/iowarp-runtime`\
    `git pull`\
    `git checkout dev`\
    `mkdir build`\
    `cd build`\
    `spack load chimaera@dev+nocompile`\
    `module load hermes_shm`\
    `cmake ../ -DCMAKE_INSTALL_PREFIX=$(scspkg pkg root chimaera) -DCHIMAERA_ENABLE_CUDA=ON`\
    `make -j32 install`\
    `cd ..`\
    `pip install -r requirements.txt`

2. **Compile Content Transfer Engine with CUDA-enabled support**\
    `cd ~/content-transfer-engine`\
    `git pull`\
    `git checkout dev`\
    `mkdir build`\
    `cd build`\
    `spack load chimaera@dev+nocompile`\
    `module load hermes_shm chimaera`\
    `cmake ../ -DCMAKE_INSTALL_PREFIX=$(scspkg pkg root hermes) -DHERMES_ENABLE_NVIDIA_GDS_ADAPTER=ON`\
    `make -j32 install`

3. **Run CUDA unit tests using Jarvis**\
   Jarvis package is used for defining how to run GDS unit test. It is located in **/test/jarvis_hermes/jarvis_hermes/hermes_nvidia_gds_tests**\
   In order to run the tests, we need to call Jarvis pipeline. The pipeline for Nvidia GDS is located in **test/pipelines/nvidia_gds**


    > [!NOTE]
    > There are two pipelines for NVIDIA_GDS
    -  test_hermes_nvidia_gds.yaml  (*Unit test with hermes*)
    -  test_nvidia_gds_basic.yaml   (*Unit test without hermes*)

    To run either of abovementioned pipelines use following command:
    - `jarvis ppl load yaml test/pipelines/nvidia_gds/<test_name>`
    - `jarvis ppl run`

