# Maintainer: Aniket <https://github.com/aniketcore>
pkgname=openrequest-git
_pkgname=openrequest
pkgver=0.1.0.r3.gc45f0d7
pkgrel=1
pkgdesc="Fast and native HTTP and API client"
arch=('x86_64' 'aarch64')
url="https://github.com/aniketcore/openrequest"
license=('custom')
depends=(
    'curl'
    'freetype2'
    'gcc-libs'
    'glibc'
    'sdl3'
    'vulkan-icd-loader'
)
makedepends=(
    'cmake'
    'git'
    'ninja'
    'vulkan-headers'
)
provides=("$_pkgname=$pkgver")
conflicts=("$_pkgname")
source=("$_pkgname::git+https://github.com/aniketcore/openrequest.git")
b2sums=('SKIP')

pkgver() {
    cd "$_pkgname"
    git describe --long --tags --abbrev=7 2>/dev/null | sed 's/^v//;s/\([^-]*-g\)/r\1/;s/-/./g' ||
    printf "r%s.%s" "$(git rev-list --count HEAD)" "$(git rev-parse --short=7 HEAD)"
}

prepare() {
    cd "$_pkgname"
    git submodule update --init --recursive
}

build() {
    cmake -B build -S "$_pkgname" \
        -G Ninja \
        -DCMAKE_BUILD_TYPE=None \
        -DCMAKE_INSTALL_PREFIX=/usr \
        -Wno-dev
    cmake --build build
}

package() {
    DESTDIR="$pkgdir" cmake --install build

    if [ -f "$_pkgname/LICENSE" ]; then
        install -Dm644 "$_pkgname/LICENSE" "$pkgdir/usr/share/licenses/$pkgname/LICENSE"
    elif [ -f "$_pkgname/LICENSE.md" ]; then
        install -Dm644 "$_pkgname/LICENSE.md" "$pkgdir/usr/share/licenses/$pkgname/LICENSE.md"
    fi
}
