FROM ubuntu:22.04

# Evitar prompts interactivos durante la instalación
ENV DEBIAN_FRONTEND=noninteractive

# Instalar dependencias esenciales y OpenMPI
RUN apt-get update && apt-get install -y \
    build-essential \
    openmpi-bin \
    openmpi-common \
    libopenmpi-dev \
    make \
    && rm -rf /var/lib/apt/lists/*


# Crear un directorio de trabajo
WORKDIR /app

COPY . /app/

CMD ["/bin/bash"]
