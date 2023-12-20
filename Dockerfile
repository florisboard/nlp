FROM ubuntu:22.04

RUN apt update && apt install git vim htop wget unzip android-sdk openjdk-17-jdk openjdk-17-jre cmake make clang python3 build-essential zlib1g-dev libncurses5-dev libgdbm-dev libnss3-dev libssl-dev libreadline-dev libffi-dev libsqlite3-dev libbz2-dev -y
WORKDIR /home/ubuntu

COPY ./setup-toolchain.sh /home/ubuntu/nlp/setup-toolchain.sh
RUN cd /home/ubuntu/nlp && ./setup-toolchain.sh
ENV LD_LIBRARY_PATH=/home/ubuntu/nlp/buildtools/clang+llvm-17.0.5-x86_64-linux-gnu-ubuntu-22.04/lib/x86_64-unknown-linux-gnu

RUN wget -qO /usr/local/bin/ninja.gz https://github.com/ninja-build/ninja/releases/latest/download/ninja-linux.zip 
RUN gunzip /usr/local/bin/ninja.gz && chmod a+x /usr/local/bin/ninja

COPY . /home/ubuntu/nlp
WORKDIR /home/ubuntu/nlp

RUN ./cmake.sh --preset=release .
RUN ./cmake.sh --build --preset=release

# COPY . 
# ENV ANDROID_HOME="/usr/lib/android-sdk"
# 
# RUN apt update && apt install git vim htop wget unzip android-sdk openjdk-17-jdk openjdk-17-jre cmake make clang python3 build-essential zlib1g-dev libncurses5-dev libgdbm-dev libnss3-dev libssl-dev libreadline-dev libffi-dev libsqlite3-dev libbz2-dev -y
# 
# RUN cd /root/ && wget --header "Referer: developer.android.com" --user-agent="Mozilla/5.0 (Windows NT 6.1; Win64; x64; rv:47.0) Gecko/20100101 Firefox/47.0" "https://dl.google.com/android/repository/android-ndk-r25c-linux.zip" && unzip android-ndk-r25c-linux.zip
# 
# RUN wget -qO /usr/local/bin/ninja.gz https://github.com/ninja-build/ninja/releases/latest/download/ninja-linux.zip 
# RUN gunzip /usr/local/bin/ninja.gz && chmod a+x /usr/local/bin/ninja
# 
# RUN cd /root/ && wget https://dl.google.com/android/repository/commandlinetools-linux-9477386_latest.zip && unzip commandlinetools-linux-9477386_latest.zip && cd /root/cmdline-tools/bin && yes | ./sdkmanager --licenses --sdk_root=$ANDROID_HOME
# 
# RUN cd /root/ && git clone https://github.com/llvm/llvm-project.git && cd llvm-project && git checkout llvmorg-17.0.4
# 
# # RUN cd /root/llvm-project && cmake -S llvm -B build -G Ninja -DLLVM_ENABLE_PROJECTS="clang;clang-tools-extra" -DCMAKE_BUILD_TYPE=Release -DLLVM_PARALLEL_COMPILE_JOBS=2 -DLLVM_PARALLEL_LINK_JOBS=2 && cmake --build build
# 
# # RUN cd /root/llvm-project && cmake -S llvm -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/clang-17 -DLLVM_ENABLE_PROJECTS="clang;clang-tools-extra" -DLLVM_ENABLE_RUNTIMES="libcxx;libcxxabi" -DLLVM_PARALLEL_COMPILE_JOBS=2 -DLLVM_PARALLEL_LINK_JOBS=2 && cmake --build build
# # RUN cd /root/llvm-project && cmake -S llvm -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/clang-17 -DLLVM_ENABLE_PROJECTS="clang;clang-tools-extra" -DLLVM_ENABLE_RUNTIMES="libc;libcxx;libcxxabi" -DLLVM_PARALLEL_COMPILE_JOBS=2 -DLLVM_PARALLEL_LINK_JOBS=2 && cmake --build build
# RUN apt install -y libc++-dev libc++abi-dev
# RUN cd /root/llvm-project && cmake -S llvm -B build -G Ninja -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/clang-17 -DLLVM_ENABLE_PROJECTS="clang;clang-tools-extra" -DLLVM_PARALLEL_COMPILE_JOBS=4 -DLLVM_PARALLEL_LINK_JOBS=4 && cmake --build build
# 
# RUN cd /root/llvm-project && cmake -S llvm -B build -G Ninja -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/clang-17 -DLLVM_ENABLE_PROJECTS="clang;clang-tools-extra" -DLLVM_ENABLE_RUNTIMES="libc;libcxx;libcxxabi" -DLLVM_PARALLEL_COMPILE_JOBS=4 -DLLVM_PARALLEL_LINK_JOBS=4 && cmake --build build
# RUN cd /root/llvm-project && cmake --install build
