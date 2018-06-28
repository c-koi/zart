#!/bin/bash
set -ev

if [ "${TRAVIS_BRANCH}" = devel ]; then
    config=debug
else
    config=release
fi

qmake --version

echo "Building standalone plugin"
qmake CONFIG+=${config}
make
