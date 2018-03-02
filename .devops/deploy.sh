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
GO_LDFLAGS="-X main.Version=$VERSION -X main.BuildDate=$TIMESTAMP"

echo 'Compiling releaser...'
echo 'OS/Arch: linux/amd64'
GOOS=linux GOARCH=amd64 go build -ldflags "$GO_LDFLAGS" -o release-$VERSION-linux-amd64 ./release
echo 'OS/Arch: windows/amd64'
GOOS=windows GOARCH=amd64 go build -ldflags "$GO_LDFLAGS" -o release-$VERSION-windows-amd64.exe ./release
echo

echo 'Compiling wrapper/library combinations...'
echo
export GOOS=linux
while IFS=, read arch cc ar pkg
do
  echo "OS/Arch: $GOOS/$arch"
  if [[ "$pkg" != "" ]]; then
    echo "Installing $pkg cross compilation toolchain..."
    sudo apt-get -y install $pkg > /dev/null 2>&1
    echo "$pkg cross compilation toolchain installed; proceeding with compilation..."
  fi
  if [[ "$arch" == "arm" ]]; then
    # We don't support ARM 5 or 6.
    export GOARM=7
  fi
  echo 'Compiling wrapper...'
  GOARCH=$arch go build -ldflags "$GO_LDFLAGS" -o wrap-$VERSION-$GOOS-$arch ./wrap
  echo 'Compiling library...'
  CC=$cc AR=$ar TARNAME="libauklet-$VERSION-$GOOS-$arch.tgz" ./bt libpkg
  echo "DONE: $GOOS/$arch"
  echo
done < compile-combos.csv

echo 'Installing AWS CLI...'
sudo apt-get -y install awscli > /dev/null 2>&1

if [[ "$ENVDIR" == "production" ]]; then
  echo 'Erasing production profiler components in public S3...'
  aws s3 rm s3://auklet/release/latest/ --recursive
  aws s3 rm s3://auklet/wrap/latest/ --recursive
  aws s3 rm s3://auklet/libauklet/latest/ --recursive
fi

echo 'Uploading profiler components to S3...'
# Iterate over each file and upload it to S3.
for f in {release-,wrap-,libauklet-}*; do
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
