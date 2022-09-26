FROM ubuntu:20.04
ENV REFRESHED_AT 2022-09-26
RUN apt-get update && apt update

#Install various needed tools for build system
RUN apt-get install -y wget && \
	apt-get install -y git && \
	apt-get install -y make && \
	apt install -y curl && \
	apt install -y zip && \
	rm -rf /var/lib/apt/lists/*

#Install Conda
ENV PATH="/root/miniconda3/bin:${PATH}"
ARG PATH="/root/miniconda3/bin:${PATH}"
RUN wget \
    https://repo.anaconda.com/miniconda/Miniconda3-latest-Linux-x86_64.sh \
    && mkdir /root/.conda \
    && bash Miniconda3-latest-Linux-x86_64.sh -b \
    && rm -f Miniconda3-latest-Linux-x86_64.sh \
    && conda --version \
    && conda init bash \
    && conda config --set auto_activate_base false

#Create Conda environment to use with Morpheus NRF project
COPY scripts/environment.yml /usr/morpheus_config/
RUN conda env create -n nrf_env --file /usr/morpheus_config/environment.yml

#Install NRF command line tools
RUN mkdir .tmp && \
cd .tmp && \
wget -qO - "https://www.nordicsemi.com/-/media/Software-and-other-downloads/Desktop-software/nRF-command-line-tools/sw/Versions-10-x-x/10-17-0/nrf-command-line-tools-10.17.0_Linux-amd64.tar.gz" | tar xz && \
#apt update && \
#apt install -y /.tmp/JLink_Linux_V766a_x86_64.deb && \
cp -r ./nrf-command-line-tools /opt && \
ln -s /opt/nrf-command-line-tools/bin/nrfjprog /usr/local/bin/nrfjprog && \
ln -s /opt/nrf-command-line-tools/bin/mergehex /usr/local/bin/mergehex && \
cd .. && rm -rf .tmp

#Set working directory and create
WORKDIR /usr/morpheus_fw_nrf52
ENTRYPOINT ["conda", "run", "--no-capture-output", "-n", "nrf_env", "make", "V=1"] 
#default make target that can be overriden
CMD ["app"] 