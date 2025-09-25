spack load iowarp
module load content-transfer-engine
chi_refresh_repo .
scspkg build profile m=cmake path=.env.cmake
scspkg build profile m=dotenv path=.env
