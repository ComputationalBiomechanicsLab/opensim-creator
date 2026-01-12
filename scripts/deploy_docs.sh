#!/usr/bin/env bash
#
# Deploys built documentation to the production hosting server
#
# NOTE: this script assumes the client has already configured
#       SSH (rsync) access to `docs.opensimcreator.com` in their
#       `.ssh/config` so that it's passwordless (e.g. with an
#       `IdentityFile` entry or similar.

rsync -avz --delete build/docs/ docs.opensimcreator.com:/var/www/docs.opensimcreator.com/manual/en/latest
