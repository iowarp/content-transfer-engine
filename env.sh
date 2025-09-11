spack load iowarp
module load iowarp-runtime
scspkg build profile m=cmake path=.env.cmake
scspkg build profile m=dotenv path=.env
