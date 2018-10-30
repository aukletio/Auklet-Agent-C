# How to Contribute

## Standards

Auklet is an edge first application performance monitor; therefore, starting 
with version 1.0.0 the following compliance levels are to be maintained:

- Automotive Safety Integrity Level B (ASIL B)

## Submissions

To submit code changes, please open a pull request that lists and explains all 
changes.

If you have found a bug please check the submitted issues. If you do not see 
your issue listed please open a new issue and we will respond as quickly as 
possible. 

If you've found a security related bug, please email our team 
directly at [hello@auklet.io](mailto:hello@auklet.io). 

# Working on the Auklet C Agent
To test, build, and install the agent, you can clone this repo and run

	make -C src/
	
To use the agent via Docker
1. Install [Docker](www.docker.com/products/docker-desktop).
1. Build your environment with `docker-compose build`.
1. To build the agent and run unit tests, run `docker-compose run auklet`.
