# -------------------------------------------------------------------
# Minimum required CMake version
# Versao minima requerida do CMake
# -------------------------------------------------------------------
cmake_minimum_required(VERSION 3.25)  # Minimum CMake version / Versao minima do CMake

# -------------------------------------------------------------------
# Project name and version
# Nome do projeto e versao
# -------------------------------------------------------------------
project(ProgramacaoGenetica
    VERSION 1.0
    LANGUAGES CXX
)

# -------------------------------------------------------------------
# C++ standard configuration
# Configuracao do padrao C++
# -------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 23)           # Use C++23 / Usa C++23
set(CMAKE_CXX_STANDARD_REQUIRED ON)  # Require C++23 standard / Exige C++23
set(CMAKE_CXX_EXTENSIONS OFF)        # Disable compiler-specific extensions / Desativa extensoes do compilador

# -------------------------------------------------------------------
# Add executable
# Adiciona executavel
# -------------------------------------------------------------------
add_executable(programacao_genetica src/main.cpp)

# -------------------------------------------------------------------
# Informational messages
# Mensagens informativas
# -------------------------------------------------------------------
message(STATUS "Project: ${PROJECT_NAME}")
message(STATUS "C++ version: ${CMAKE_CXX_STANDARD}")
message(STATUS "Source: main.cpp")

# -------------------------------------------------------------------
# Threading Building Blocks (TBB)
# Biblioteca de paralelismo TBB
# -------------------------------------------------------------------
find_package(TBB REQUIRED)

target_link_libraries(programacao_genetica PRIVATE TBB::tbb)