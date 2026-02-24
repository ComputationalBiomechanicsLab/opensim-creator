# `docker/` Linux-Based Development Environments

## Cheatsheet

```bash
# List all images
docker images

# Delete `fooimage`
docker rmi fooimage

# List all containers
docker ps

# Prune all not-running containers
docker container prune

# Build a development environment image
cd docker && docker build -t oscar-ubuntu22 -f ubuntu22-ci.Dockerfile . && cd -

# Build project via the environment
# --init is necessary to prevent the bash script from being PID=1, which can
#        make xvfb-run hang
BUILD_TYPE=Release
docker run --init --rm -v "${PWD}:/project" -w /project oscar-ubuntu22 bash -c "cd third_party && cmake --workflow --preset ${BUILD_TYPE} && cd .. && xvfb-run cmake --workflow --preset ${BUILD_TYPE}"
```
