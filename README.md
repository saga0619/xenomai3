Xenomai 3.3-rc1 mirror.
I219 lan driver patch applied, which is from [here](https://lore.kernel.org/xenomai/CAFCNAPBUT9W_X=Y_Hm4uOzE_3ZHK=C2M4oD4KwHA0TmAAH5xSQ@mail.gmail.com/)



Where to start from?
====================

http://xenomai.org/start-here/ is the best place to start learning
about Xenomai 3.

Also, make sure to read the per-architecture README files, i.e.:
kernel/cobalt/arch/*/README

Documentation
=============

The Xenomai 3.x documentation can be built then installed this way:

xenomai-3.x.y/configure --enable-doc-build --prefix=<install-dir>

Asciidoc, Doxygen, W3M and Dot packages are required for building the
documentation.

Online documentation
====================

The online version of the documentation is available from our website
for the current release:

http://xenomai.org/installing-xenomai-3-x/
http://xenomai.org/building-applications-with-xenomai-3-x/
http://xenomai.org/running-applications-with-xenomai-3-x/
http://xenomai.org/migrating-from-xenomai-2-x-to-3-x/
http://xenomai.org/documentation/xenomai-3/html/xeno3prm/index.html
http://xenomai.org/troubleshooting-a-dual-kernel-configuration/
http://xenomai.org/troubleshooting-a-single-kernel-configuration/

Building from sources
=====================

Detailed instructions for building from sources are available at:
http://xenomai.org/installing-xenomai-3-x/

- GIT clone:

  git://git.xenomai.org/xenomai-3.git
  http://git.xenomai.org/xenomai-3.git
  http://git.xenomai.org/xenomai-3.git

  Once the repository is cloned, make sure to bootstrap the autoconf
  system in the top-level directory by running scripts/bootstrap.  In
  order to do this, you will need the GNU autotools installed on your
  workstation.

  If you intend to update the Xenomai code base, you may want to pass
  --enable-maintainer-mode to the configure script for building, so
  that autoconf/automake output files are automatically regenerated at
  the next (re)build in case the corresponding templates have changed.

- Tarballs:

  http://xenomai.org/downloads/xenomai/

  Source tarballs are self-contained and ready for building.

Licensing terms
===============

Source files which implement the Xenomai software system generally
include a copyright notice and license header. In absence of license
header in a particular file, the terms and conditions stated by the
COPYING or LICENSE file present in the top-level directory of the
relevant package apply.

For instance, lib/cobalt/COPYING states the licensing terms and
conditions applicable to the source files present in the hierarchy
rooted at lib/cobalt.
