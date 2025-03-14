sudo docker run --rm --volume ${PWD}/paper:/data --user 1000:1000 --env JOURNAL=joss openjournals/inara -o pdf,tex paper.md
