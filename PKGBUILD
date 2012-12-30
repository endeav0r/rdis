##This is a basic PKGBUILD for rdis to ease building RDIS in my Arch boxes
##Maintainer : TDKPS
pkgname=rdis
pkgrel=1
pkgver=20121230
arch=(x86_64)
url="https://github.com/endeav0r/rdis"
license=('GPLv3')
depends=(gtk3 luajit cairo jansson git)
optdepends=(lua51-socket)
_gitroot="https://github.com/endeav0r/rdis.git"
_gitname="rdis"


build() {
  cd "$srcdir"

  msg "Connecting to GIT server..."
  if [[ -d ${_gitname} ]]; then
    (cd ${_gitname} && git pull origin)
  else
    git clone ${_gitroot} ${_gitname}
  fi
  msg "GIT checkout done or server timeout"
  msg "Starting make..."

  rm -rf ${_gitname}-build
  git clone ${_gitname} ${_gitname}-build

  cd ${srcdir}/${_gitname}-build/

make

}
package() {
  cd "$srcdir/$_gitname-build"
  make DESTDIR="$pkgdir/" install
}
