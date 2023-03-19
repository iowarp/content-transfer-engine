#!/bin/bash

# Hermes dependency installation script
#
# Hermes depends on the following packages (in alphabetical order):
#
# Catch2
# GLOG
# GLPK
# HDF5
# IOR (for performance testing)
# Thallium
# yaml-cpp
#
# This script will build and install them via Spack from source
# because Hermes requires a very specific version and configuration options
# for each package.

set -x
set -e
set -o pipefail

sudo apt-get install -y pkg-config

# Change this especially when your $HOME doesn't have enough disk space. 
INSTALL_DIR="${HOME}/${LOCAL}"

SPACK_DIR=${INSTALL_DIR}/spack

echo "Installing dependencies at ${INSTALL_DIR}"
mkdir -p ${INSTALL_DIR}

# Spack
git clone https://github.com/spack/spack ${SPACK_DIR}
pushd ${SPACK_DIR}
git checkout v${SPACK_VERSION}
popd

set +x
. ${SPACK_DIR}/share/spack/setup-env.sh
set -x

# This will allow Spack to skip building some packages that are directly
# available from the system. For example, autoconf, cmake, m4, etc.
# Modify ci/pckages.yaml to skip building compilers or build tools via Spack.
cp ci/packages.yaml ${SPACK_DIR}/etc/spack/packages.yaml

# This will override Spack's default package repository to allow building
# a custom package when the same package is available from Spack.
spack repo add ./ci/hermes

THALLIUM_SPEC="mochi-thallium@${THALLIUM_VERSION} ^mercury~boostsys"

HERMES_VERSION=0.9.8
spack install hermes
SPACK_STAGING_DIR=~/spack_staging
mkdir -p ${SPACK_STAGING_DIR}

# Spack installation directory has hash value.
# This will simplify and consolidate the installation path.
spack view --verbose symlink ${SPACK_STAGING_DIR} ${ALL_SPECS}

# Copy what Spack installed in a temporary location to your desired location.
cp -LRnv ${SPACK_STAGING_DIR}/* ${INSTALL_DIR}
