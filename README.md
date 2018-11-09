# Auklet for C

<a href="https://www.apache.org/licenses/LICENSE-2.0" alt="Apache page link -- Apache 2.0 License"><img src="https://img.shields.io/pypi/l/auklet.svg" /></a>
<a href="https://codeclimate.com/repos/599de6da0e0de702630009ca/test_coverage"><img src="https://api.codeclimate.com/v1/badges/66870661edeeb2e46253/test_coverage" /></a>

This is the official C agent for Auklet. It officially supports C
and C++, and runs on most POSIX-based operating systems (Debian, 
Ubuntu Core, Raspbian, QNX, etc).

## Features

[auklet_site]: https://app.auklet.io
[auklet_releaser]: https://github.com/aukletio/Auklet-Releaser-C
[auklet_client]: https://github.com/aukletio/Auklet-Client-C
[mail_auklet]: mailto:hello@auklet.io

- Automatic crash reporting
- Automatic Function performance issue reporting
- Location, system architecture, and system metrics identification for all 
issues
- Ability to define data usage restriction

## Device Requirements

Auklet's C/C++ agent is built to run on any POSIX operating system. If 
you don't see the OS or CPU architecture you are using for your application 
listed below, and are wondering if Auklet will be compatible, please hit us 
up at [hello@auklet.io][mail_auklet]. 

#### Validated OSes:

- Debian 8.6
- Fedora 24
- Linaro 4.4.23
- OpenWRT 3.8.3
- Rasbian Jessie 4.9.30 
- Rasbian Stretch 4.14.71
- Ubuntu 16.04
- Yocto 2.2-r2

#### Validated CPU architectures:

- x86-64
- ARM7
- ARM64
- MIPS

### Networking
Auklet is built to work in network constrained environments. It can operate 
while devices are not connected to the internet and then upload data once 
connectivity is reestablished. Auklet can also work in non-IP based 
environments as well. For assistance with getting Auklet running in a non-IP 
based environment contact [hello@auklet.io][mail_auklet].

## Quickstart

### Getting Started

1. When using a Makefile, include the following flags to integrate the Auklet 
agent

        CFLAGS += -finstrument-functions -g
        LDLIBS += libauklet.a -lpthread
        LDFLAGS += -no-pie
    
        libauklet.a:
        curl <insert link for your cpu> | tar xz
    
1. The curl command at the end of the above flags will vary depending on your 
   application's target architecture. Replace the portion with < > above with
   the target architecture's link below. 

    - [x86-64](https://s3.amazonawscom/auklet/agent/c/latest/libauklet-amd64-latest.tgz)
        
    - [ARM7](https://s3.amazonaws.com/auklet/agent/c/latest/libauklet-arm-latest.tgz)
    
    - [ARM64](https://s3.amazonaws.com/auklet/agent/c/latest/libauklet-arm64-latest.tgz)

    - [MIPS](https://s3.amazonaws.com/auklet/agent/c/latest/libauklet-arm64-latest.tgz)

    - [MIPS64](https://s3.amazonaws.com/auklet/agent/c/latest/libauklet-MIPS64-latest.tgz)

    - [MIPS64le](https://s3.amazonaws.com/auklet/agent/c/latest/libauklet-mips64le-latest.tgz)

    - [MIPSle](https://s3.amazonaws.com/auklet/agent/c/latest/libauklet-miple-latest.tgz)

1. When compiling, include `-g -finstrument-functions`.

1. When linking, include `libauklet.a -lpthread -no-pie`. **libauklet.a** is 
downloadable from the links in step two.
    

### Next steps
1. In order for Auklet to report your application's performance, you'll need to 
perform a release whenever your code is updated and deployed. Check the 
README for the 
[Auklet C/C++ Releaser][auklet_releaser] for instructions.

1. You application will also need to run with the Auklet client to report its 
data. The C/C++ Auklet Client, and instructions for running Auklet with your 
application, can be found in the 
[C/C++ Auklet Client's repository][auklet_client].


