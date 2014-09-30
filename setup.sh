#!/bin/bash

# Alter these variables to suit your need
BINUTILS_VERSION="2.24"
GCC_VERSION="4.9.1"
BINUTILS_URL="http://ftp.gnu.org/gnu/binutils/binutils-$BINUTILS_VERSION.tar.bz2"
GCC_URL="ftp://ftp.lip6.fr/pub/gcc/releases/gcc-$GCC_VERSION/gcc-$GCC_VERSION.tar.bz2"


# Temporary building directory
BUILDDIR="build"

cat <<EOF
Blast ! SDK - SEGA 16 bit development tools.
This script will setup the Blast SDK for your system.
EOF

if [ "$(uname -o)" = "Cygwin" ]; then
DEFAULTPREFIX="/cygdrive/c/blastsdk"
elif [ "$UID" -eq 0 ]; then
DEFAULTPREFIX="/opt/blastsdk"
else
DEFAULTPREFIX="$HOME/opt/blastsdk"
fi

if [ -z "$BLSPREFIX" ]; then
cat <<EOF
Please select the installation prefix for compilation tools.
You can automate this step by setting the BLSPREFIX variable.

(default BLSPREFIX=$DEFAULTPREFIX)
EOF
read BLSPREFIX
if [ -z "$BLSPREFIX" ]; then
BLSPREFIX="$DEFAULTPREFIX"
fi
fi

# INSTALL_DIR for asmx and bls Makefiles
export BLSPREFIX
export INSTALL_DIR="$BLSPREFIX/bin"

pushd . &>/dev/null
trap 'popd &>/dev/null' EXIT
cd "$(dirname "$0")"

if [ "$1" = "force" ]; then
  FORCE=1
fi

if [ "$1" = "uninstall" ]; then
  echo ""
  echo "Warning: uninstall cannot remove GCC and binutils."
  echo "Please remove them by hand."
  echo ""
  echo "Ready to uninstall Blast SDK. Press enter to continue or Ctrl-C to abort."
  read dummy
  make -C tools/asmx2 uninstall
  make -C tools/bls uninstall
  make -C src uninstall
  rm -rf "$BLSPREFIX/share/blastsdk"
  exit
fi

if [ "$1" = "clean" ]; then
  if [ -e "$BUILDDIR" ]; then
    rm -rf "$BUILDDIR"
  fi
  [ -e "sgdk" ] && rm -rf sgdk
  make -C tools/asmx2 clean
  make -C tools/bls clean
  make -C src clean
  exit
fi

mkdir -p "$BLSPREFIX/bin"
mkdir -p "$BLSPREFIX/share/blastsdk"

SRCBINUTILS="binutils-$BINUTILS_VERSION"
SRCGCC="gcc-$GCC_VERSION"

checkcmd() {
  while [ "$1" ]; do
    if ! which $1 &>/dev/null; then
      return 1
    fi
  shift; done
  return 0
}

requirecmd() {
  while [ "$1" ]; do
    if ! which $1 &>/dev/null; then
      echo "Could not find command $1"
      exit 1
    fi
  shift; done
  return 0
}

installsrc() {
  if [ "$FORCE" ] && [ -e "$BLSPREFIX$2/$1" ]; then
    rm -rf "$BLSPREFIX$2/$1"
  fi
  if ! [ -e "$BLSPREFIX$2/$1" ]; then
    echo "Installing blastsdk sources : $1"
    cp -r $1 "$BLSPREFIX$2/$1"
  fi
}



# Install asmx

if [ -z "$FORCE" ] && [ -e "$BLSPREFIX/bin/asmx" ]; then
if ! ( "$BLSPREFIX/bin/asmx" 2>&1 | grep -- '-I prefix' &>/dev/null ); then
  echo "Error : unpatched asmx detected. Please remove it to install the version provided with blastsdk"
  exit 1
fi
else
echo "Installing asmx"
make -C tools/asmx2 install || exit $?
fi

[ -d "$BUILDDIR" ] || mkdir -p "$BUILDDIR"
OLDPATH="$PATH"
export PATH="$BLSPREFIX/bin:$PATH"

set -e

pushd .
cd "$BUILDDIR"

echo "Checking for binutils"
if ! checkcmd m68k-elf-as m68k-elf-ld m68k-elf-nm m68k-elf-objcopy; then

requirecmd gcc make wget pkg-config

echo "Building binutils $BINUTILS_VERSION"

[ -e "$SRCBINUTILS.tar.bz2" ] || wget -O"$SRCBINUTILS.tar.bz2" "$BINUTILS_URL"
[ -e "$SRCBINUTILS" ] || tar -xvjf "$SRCBINUTILS.tar.bz2"
[ -e binutils_build ] || mkdir binutils_build
cd binutils_build
../"$SRCBINUTILS"/configure --prefix="$BLSPREFIX" --target=m68k-elf
make
make install
cd ..

else
echo "Binutils found."
fi

echo "Checking for gcc"
if ! checkcmd m68k-elf-gcc; then

requirecmd bison yacc gcc make wget

echo "Building gcc $GCC_VERSION"

[ -e "$SRCGCC.tar.bz2" ] || wget -O"$SRCGCC.tar.bz2" "$GCC_URL"
[ -e "$SRCGCC" ] || tar -xvjf "$SRCGCC.tar.bz2"

[ -e gcc_build ] || mkdir gcc_build
cd gcc_build
../"$SRCGCC"/configure --prefix="$BLSPREFIX" --target=m68k-elf --enable-languages=c --disable-libssp --disable-libquadmath --disable-libada --disable-shared
make || true # May fail because of libstdc++
make install || true # May fail because of libstdc++
cd ..

else
echo "gcc found"
fi

popd

if [ "$FORCE" ] || ! checkcmd blsbuild bdb d68k scdupload; then
requirecmd pkg-config
if ! pkg-config --exists libpng; then
echo "libpng development package is not installed."
exit 1
fi
echo "Installing bls"
export INCDIR="$BLSPREFIX/share/blastsdk/inc"
export INCLUDEDIR="$BLSPREFIX/share/blastsdk/include"
export SRCDIR="$BLSPREFIX/share/blastsdk/src"
export BLAST_AS="$(which m68k-elf-as 2>/dev/null)"
export BLAST_CC="$(which m68k-elf-gcc 2>/dev/null)"
export BLAST_CXX="$(which m68k-elf-g++ 2>/dev/null)"
export BLAST_LD="$(which m68k-elf-ld 2>/dev/null)"
export BLAST_NM="$(which m68k-elf-nm 2>/dev/null)"
export BLAST_OBJCOPY="$(which m68k-elf-objcopy 2>/dev/null)"
export BLAST_ASMX="$(which asmx 2>/dev/null)"
make -C tools/bls install

pushd . &>/dev/null
cd "$BUILDDIR"
if ! "$BLSPREFIX/bin/d68ktest"; then
echo "d68k self-test failed"
exit 1
fi
popd

fi

if ! [ "$FORCE" ]; then
echo "Installing sgdk library"
requirecmd svn
if [ -e "sgdk" ]; then
  svn update sgdk
else
  svn checkout http://sgdk.googlecode.com/svn/trunk/ sgdk
fi
cp tools/makelib.sgdk sgdk
make -C sgdk -f makelib.sgdk
fi

echo "Installing bls sources"
installsrc include /share/blastsdk
installsrc src /share/blastsdk


echo ""
echo "Installation finished."

PATH="$OLDPATH"
if ! which blsbuild &>/dev/null; then
  echo "Please add $BLSPREFIX/bin to your PATH (edit $HOME/.profile)."
fi
