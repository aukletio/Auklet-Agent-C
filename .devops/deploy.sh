#!/bin/bash
set -e
if [[ "$1" == "" ]]; then
  echo "ERROR: env not provided."
  exit 1
fi
ENVDIR=$1
VERSION="$(cat VERSION)"
VERSION_SIMPLE=$(cat VERSION | xargs | cut -f1 -d"+")
export TIMESTAMP="$(date --rfc-3339=seconds | sed 's/ /T/')"

echo 'Compiling/packaging C agent binaries...'
echo
while IFS=, read arch cc ar pkg
do
  echo "=== $arch ==="
  if [[ "$pkg" != "" ]]; then
    echo "Installing $pkg cross compilation toolchain..."
    sudo apt -y install $pkg > /dev/null 2>&1
  fi
  CC=$cc AR=$ar TARNAME="libauklet-$arch-$VERSION.tgz" ./bt libpkg
  echo
done < arch-grid.csv

echo 'Installing AWS CLI...'
sudo apt -y install awscli > /dev/null 2>&1

if [[ "$ENVDIR" == "production" ]]; then
  echo 'Erasing production C agent binaries in public S3...'
  aws s3 rm s3://auklet/libauklet/latest/ --recursive
fi

echo 'Uploading C agent binaries to S3...'
# Iterate over each file and upload it to S3.
for f in {libauklet-}*; do
  # Upload to the internal bucket.
  S3_LOCATION="s3://auklet-profiler/$ENVDIR/$VERSION/$f"
  aws s3 cp $f $S3_LOCATION
  # Upload to the public bucket for production builds.
  if [[ "$ENVDIR" == "production" ]]; then
    # Get the component name.
    COMPONENT=$(echo $f | cut -f1 -d"-")
    # Copy to the public versioned directory.
    VERSIONED_NAME="${f/$VERSION/$VERSION_SIMPLE}"
    aws s3 cp $S3_LOCATION s3://auklet/$COMPONENT/$VERSION_SIMPLE/$VERSIONED_NAME
    # Copy to the public "latest" directory.
    LATEST_NAME="${f/$VERSION/latest}"
    aws s3 cp $S3_LOCATION s3://auklet/$COMPONENT/latest/$LATEST_NAME
  fi
done
