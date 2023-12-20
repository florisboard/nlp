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

# ENV ANDROID_HOME="/usr/lib/android-sdk"
# 
# RUN cd /root/ && wget --header "Referer: developer.android.com" --user-agent="Mozilla/5.0 (Windows NT 6.1; Win64; x64; rv:47.0) Gecko/20100101 Firefox/47.0" "https://dl.google.com/android/repository/android-ndk-r25c-linux.zip" && unzip android-ndk-r25c-linux.zip
# 
# RUN cd /root/ && wget https://dl.google.com/android/repository/commandlinetools-linux-9477386_latest.zip && unzip commandlinetools-linux-9477386_latest.zip && cd /root/cmdline-tools/bin && yes | ./sdkmanager --licenses --sdk_root=$ANDROID_HOME
