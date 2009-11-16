# Copyright 1999-2009 Gentoo Foundation
# Distributed under the terms of the GNU General Public License v2
inherit git

EAPI=2
EGIT_REPO_URI="git@github.com:lynix/vcp.git"
DESCRIPTION="cp with additional features like progress bars and error handling"
HOMEPAGE="http://github.com/lynix/vcp"
LICENSE="GPL-3"

KEYWORDS="~amd64 ~x86"
SLOT="0"
IUSE=""

RDEPEND=""
DEPEND="${RDEPEND}"

src_compile() {   
	sed -e "s/CFLAGS =/CFLAGS = ${CFLAGS}/" -i Makefile
	emake LDFLAGS="${LDFLAGS}" || die "emake failed"
}

src_install() {
	emake install DESTDIR="${D}/usr" || die "emake install failed"
	dodoc CHANGELOG README TODO || die
}
