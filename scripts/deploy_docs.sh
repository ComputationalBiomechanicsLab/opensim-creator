#!/usr/bin/env bash

# note: this requires that the client has the necessary
# SSH keys configured on their machine

rsync -avz --delete build/docs/ docs.opensimcreator.com:/var/www/docs.opensimcreator.com/manual/en/latest
