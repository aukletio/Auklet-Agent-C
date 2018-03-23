# Auklet C/C++ Agent

Auklet's IoT C/C++ agent is built to run on any POSIX operating system. It
has been validated on:

- Ubuntu 16.04

# Build

To test, build, and install the agent, run

	make

# Docker Setup

1. Install Docker for Mac Beta.
1. Build your environment with `docker-compose build`.
1. To build the agent and run unit tests, run `docker-compose run auklet`.
