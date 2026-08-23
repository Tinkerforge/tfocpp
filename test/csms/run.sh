#!/bin/sh
# Runs the CSMS integration tests. Requires uv (https://docs.astral.sh/uv/).
# Configuration via .env, see .env.example. Pass through pytest arguments,
# e.g. ./run.sh -k security -v
cd "$(dirname "$0")" || exit 1
exec uv run --quiet --with pytest --with requests python -m pytest "$@"
