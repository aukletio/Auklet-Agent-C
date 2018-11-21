# How to Contribute

## Standards

Auklet is an edge first application performance monitor; therefore, starting
with version 1.0.0 the following compliance levels are to be maintained:

- Automotive Safety Integrity Level B (ASIL B)

## Submissions

If you have found a bug, please go to https://help.auklet.io and click the blue
button in the lower-right corner to report it to our support team.

We are not accepting outside contributions at this time. If you have a feature
request or idea, please open a new issue.

If you've found a security related bug, please do not create an issue or PR.
Instead, email our team directly at [security@auklet.io](mailto:security@auklet.io).

# Working on the Auklet C Agent
To test, build, and install the agent, you can clone this repo and run:

	make -C src/

To use the agent via Docker
1. Install [Docker](www.docker.com/products/docker-desktop).
1. Build your environment with `docker-compose build`.
1. To build the agent and run unit tests, run `docker-compose run auklet`.
