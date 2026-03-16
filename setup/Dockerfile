# Production Dockerfile for H8 32-bit compiler toolchain
FROM ubuntu:22.04 AS build

RUN apt-get update && \
    DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
        git cmake build-essential zlib1g-dev unzip 7zip ca-certificates wget && \
    rm -rf /var/lib/apt/lists/*

# Build binutils 2.22
RUN wget https://ftp.gnu.org/gnu/binutils/binutils-2.22.tar.gz && \
    tar xzf binutils-2.22.tar.gz && \
    mkdir binutils-build && cd binutils-build && \
    ../binutils-2.22/configure --target=h8300-elf --prefix=/opt/h8-toolchain --disable-nls --disable-werror && \
    make -j$(nproc) && \
    make install

# Install ISx
WORKDIR /build
COPY isx-linux-paths.patch .
RUN git clone https://github.com/devbis/ISx.git && \
    cd ISx && \
    git checkout f1f0951 && \
    git apply ../isx-linux-paths.patch && \
    cmake . && \
    make && \
    install -m 0755 isx /usr/local/bin/isx

# Install unshield
WORKDIR /build
RUN git clone https://github.com/twogood/unshield.git --depth=1 && \
    cd unshield && \
    cmake . && \
    make && \
    install -m 0755 src/unshield /usr/local/bin/unshield

# Unpack Renesas H8 C Toolchains
WORKDIR /work
COPY installers/ installers/
COPY unpack.sh .
RUN ./unpack.sh installers/


FROM ubuntu:22.04

# Install Wine (wine32 alone doesn't provide the 'wine' binary on some systems)
RUN dpkg --add-architecture i386 && \
    apt-get update && \
    DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
        wine \
        wine32 \
        libmpfr6 \
        libgmp10 \
        make \
        python3 && \
    rm -rf /var/lib/apt/lists/*

# Add GCC H8 Toolchain
COPY --from=build /opt/h8-toolchain /opt/h8-toolchain
ENV PATH="/opt/h8-toolchain/bin:${PATH}"

# Add Renesas H8 Toolchains
COPY --from=build /work/H8 /opt/H8/

WORKDIR /work

# Wine environment for 32-bit CLI
ENV WINEARCH=win32 \
    WINEPREFIX=/opt/.wine \
    WINEDEBUG=-all

# Initialize Wine prefix and generate wrappers for all H8 versions
RUN wine wineboot --init && wineserver -w && \
    for version_dir in /opt/H8/*; do \
        version=$(basename "$version_dir"); \
        if [ -d "$version_dir/bin" ]; then \
            for exe in "$version_dir/bin"/*.exe; do \
                exe_name=$(basename "$exe" .exe); \
                wrapper_name="h8-${version}-${exe_name}"; \
                printf "#!/bin/bash\nexec wine \"%s\" \"\$@\"\n" "$exe" > "/usr/local/bin/${wrapper_name}"; \
                chmod +x "/usr/local/bin/${wrapper_name}"; \
                echo "Created wrapper: ${wrapper_name}"; \
            done \
        fi \
    done
# Use CMD instead of ENTRYPOINT to allow running arbitrary binaries
CMD ["/bin/bash"]