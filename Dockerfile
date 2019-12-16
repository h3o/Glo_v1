FROM ubuntu:18.04

ARG DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    apt-utils \
    bison \
    ca-certificates \
    ccache \
    check \
    cmake \
    curl \
    flex \
    git \
    gperf \
    lcov \
    libncurses-dev \
    libusb-1.0-0-dev \
    make \
    ninja-build \
    python3 \
    python3-pip \
    python2.7 \
    python-pip \
    unzip \
    wget \
    xz-utils \
    zip \
   && apt-get autoremove -y \
   && rm -rf /var/lib/apt/lists/* \
   && update-alternatives --install /usr/bin/python python /usr/bin/python3 10

RUN python -m pip install --upgrade pip virtualenv

# To build the image for a branch or a tag of IDF, pass --build-arg IDF_CLONE_BRANCH_OR_TAG=name.
# To build the image with a specific commit ID of IDF, pass --build-arg IDF_CHECKOUT_REF=commit-id.
# It is possibe to combine both, e.g.:
#   IDF_CLONE_BRANCH_OR_TAG=release/vX.Y
#   IDF_CHECKOUT_REF=<some commit on release/vX.Y branch>.

ARG IDF_CLONE_URL=https://github.com/espressif/esp-idf.git
ARG IDF_CLONE_BRANCH_OR_TAG=master
# IDF revision used for Glo 1.5
ARG IDF_CHECKOUT_REF=a2556229aa6f55b16b171e3325ee9ab1943e8552

ENV IDF_PATH=/opt/esp/idf
ENV IDF_TOOLS_PATH=/opt/esp

RUN echo IDF_CHECKOUT_REF=$IDF_CHECKOUT_REF IDF_CLONE_BRANCH_OR_TAG=$IDF_CLONE_BRANCH_OR_TAG && \
    git clone --recursive \
      ${IDF_CLONE_BRANCH_OR_TAG:+-b $IDF_CLONE_BRANCH_OR_TAG} \
      $IDF_CLONE_URL $IDF_PATH && \
    if [ -n "$IDF_CHECKOUT_REF" ]; then \
      cd $IDF_PATH && \
      git checkout $IDF_CHECKOUT_REF && \
      git submodule update --init --recursive; \
    fi

# no install script in IDF revision used by glo 1.5
# RUN $IDF_PATH/install.sh && \
#   rm -rf $IDF_TOOLS_PATH/dist

RUN mkdir -p $HOME/.ccache && \
  touch $HOME/.ccache/ccache.conf

# No Entrypoint in a25562
# COPY entrypoint.sh /opt/esp/entrypoint.sh

# ENTRYPOINT [ "/opt/esp/entrypoint.sh" ]
# ADD . /usr/glo

# We need python 2 and pyserial TODO: 2.7 is installed above
RUN apt update && apt install python2.7 && python2.7 -m pip install pyserial

ENV XTENSA_PATH="/opt/xtensa-esp32-elf"
RUN wget https://dl.espressif.com/dl/xtensa-esp32-elf-linux64-1.22.0-80-g6c4433a-5.2.0.tar.gz && \
    tar -xzf xtensa-esp32-elf-linux64-1.22.0-80-g6c4433a-5.2.0.tar.gz -C /opt/ && \
    rm xtensa-esp32-elf-linux64-1.22.0-80-g6c4433a-5.2.0.tar.gz

ENV PATH="$PATH:$XTENSA_PATH/bin"

# Removing upstream changes from a255562 ref
RUN cd $IDF_PATH && \
    rm -rf examples/build_system && \
    rm -rf components/unity && \
    rm -rf components/protobuf-c && \
    rm -rf components/mqtt/ && \
    rm -rf components/mbedtls/mbedtls/ && \
    rm -rf components/lwip/lwip/ && \
    rm -rf components/expat/expat && \
    rm -rf components/esp_wifi && \
    rm -rf components/cbor && \
    rm -rf components/bt/host && \
    rm -rf components/bt/controller && \
    rm -rf components/asio && \
    rm -rf components/bootloader/subproject/components

ENV GLO_PATH="/usr/glo"

CMD [ "/bin/bash" ]
