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
PREFIX='libauklet'
BENCH_PREFIX="$PREFIX-bench-overhead"
S3_BUCKET='auklet'
S3_PREFIX='agent/c'
while IFS=, read arch cc ar ld oc nm pkg
do
  echo "=== $arch ==="
  if [[ "$pkg" != "" ]]; then
    echo "Installing $pkg cross compilation toolchain..."
    sudo apt -y install $pkg > /dev/null 2>&1
  fi
  CC=$cc AR=$ar LD=$ld OC=$oc NM=$nm TARNAME="$PREFIX-$arch-$VERSION.tgz" make -C src clean libauklet.tgz install
  CC=$cc make -C bench clean overhead
  mv bench/overhead "$BENCH_PREFIX-$arch-$VERSION"
  mv src/*.tgz .
  make -C src uninstall
  echo
done < arch-grid.csv

echo 'Installing AWS CLI...'
sudo apt -y install awscli > /dev/null 2>&1

if [[ "$ENVDIR" == "production" ]]; then
  echo 'Erasing production C agent binaries in public S3...'
  aws s3 rm s3://$S3_BUCKET/$S3_PREFIX/latest/ --recursive
fi

echo 'Uploading C agent binaries to S3...'
# Iterate over each file and upload it to S3.
for f in ${PREFIX}-*; do
  # Upload to the internal bucket.
  S3_LOCATION="s3://auklet-profiler/$ENVDIR/$S3_PREFIX/$VERSION/$f"
  aws s3 cp $f $S3_LOCATION
  # Do not upload the benchmark tool to the public bucket.
  if [[ $f == ${BENCH_PREFIX}* ]]; then
    continue;
  fi
  # Upload to the public bucket for production builds.
  if [[ "$ENVDIR" == "production" ]]; then
    # Copy to the public versioned directory.
    VERSIONED_NAME="${f/$VERSION/$VERSION_SIMPLE}"
    aws s3 cp $S3_LOCATION s3://$S3_BUCKET/$S3_PREFIX/$VERSION_SIMPLE/$VERSIONED_NAME
    # Copy to the public "latest" directory.
    LATEST_NAME="${f/$VERSION/latest}"
    aws s3 cp $S3_LOCATION s3://$S3_BUCKET/$S3_PREFIX/latest/$LATEST_NAME
  fi
done
