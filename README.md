# Auklet for C

<a href="https://www.apache.org/licenses/LICENSE-2.0" alt="Apache page link -- Apache 2.0 License"><img src="https://img.shields.io/pypi/l/auklet.svg" /></a>
<a href="https://codeclimate.com/repos/599de6da0e0de702630009ca/maintainability"><img src="https://api.codeclimate.com/v1/badges/66870661edeeb2e46253/maintainability" /></a>
<a href="https://codeclimate.com/repos/599de6da0e0de702630009ca/test_coverage"><img src="https://api.codeclimate.com/v1/badges/66870661edeeb2e46253/test_coverage" /></a>

Auklet is a profiler for IoT and embedded Linux apps. Like conventional 
benchtop C/C++ profilers, it is implemented as a library that you can link 
your program against. Unlike benchtop profilers, it is meant to be run in 
production and to continuously generate performance metrics.  

## Device Requirements

Auklet's IoT C/C++ agent is built to run on any POSIX operating system. It
has been validated on:

- Debian 8.6
- Fedora 24
- Linaro 4.4.23
- OpenWRT 3.8.3
- Rasbian Jessie 4.9.30 
- Rasbian Stretch 4.14.71
- Ubuntu 16.04
- Yocto 2.2-r2

Auklet has also been validated for the following CPU architectures:

- x86-64
- ARM7
- ARM64
- MIPS

Lastly, don't forget to ensure that your device is connected to the Internet.

	
## Downloading and Integrating the Agent

When using a Makefile, include the following flags to integrate the Auklet 
agent

    CFLAGS += -finstrument-functions -g
    LDLIBS += libauklet.a -lpthread
    LDFLAGS += -no-pie
    
    libauklet.a:
    [curl command here]
    
The curl command at the end of the above flags will vary depending on your 
application's target architecture. Replace the portion in [braces] above with
 the target architecture's curl command below. 
 
 If you are not using a Makefile, you can also use the below commands in a 
 terminal to directly download the agent to your work machine. Remember to 
 use the architecture matching your destination device's architecture, not 
 your work environment's architecture.

x86-64

    curl https://s3.amazonaws.com/auklet/agent/c/latest/libauklet-amd64-latest.tgz | tar xz

ARM7

    curl https://s3.amazonaws.com/auklet/agent/c/latest/libauklet-arm-latest.tgz | tar xz

ARM64

    curl https://s3.amazonaws.com/auklet/agent/c/latest/libauklet-arm64-latest.tgz | tar xz

MIPS

    curl https://s3.amazonaws.com/auklet/agent/c/latest/libauklet-arm64-latest.tgz | tar xz

MIPS64

    curl https://s3.amazonaws.com/auklet/agent/c/latest/libauklet-MIPS64-latest.tgz | tar xz

MIPS64le

    curl https://s3.amazonaws.com/auklet/agent/c/latest/libauklet-mips64le-latest.tgz | tar xz

MIPSle

    curl https://s3.amazonaws.com/auklet/agent/c/latest/libauklet-miple-latest.tgz | tar xz`

<br /><br />
If you are compiling your application with GCC, be sure to include the 
following options in the compile invocations.

When creating the object file include

    ... -c -g -finstrument-functions ...

and then when linking the object file with **libauklet.a** include

    ... libauklet.a -lpthread -no-pie   
    
## Releasing and Running with Auklet

In order for Auklet to report your application's performance, you'll need to 
perform a release whenever your code is updated and deployed. Check the 
README for the 
[Auklet C/C++ Releaser][releaser] for instructions.

You application will also need to run with the Auklet client to report its 
data. The C/C++ Auklet Client, and instructions for running Auklet with your 
application, can be found in the 
[C/C++ Auklet Client's repository][client].

[releaser]: https://github.com/aukletio/Auklet-Releaser-C
[client]: https://github.com/aukletio/Auklet-Client-C