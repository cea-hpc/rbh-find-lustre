# This file is part of rbh-find-lustre
# Copyright (C) 2022 Commissariat a l'energie atomique et aux energies
#                    alternatives
#
# SPDX-License-Identifer: LGPL-3.0-or-later

check = dependency('check', version: '>=0.9.9', required: false)

if not check.found()
    message('check is missing: skipping unit testing')
    subdir_done()
endif

# force tests to use the newly built binaries over paths in
# LD_LIBRARY_PATH. The problem is that recent linkers will
# use RUNPATH when meson requests -rpath, and runpath has
# lower priority than LD_LIBRARY_PATH (contrary to rpath)
# See https://github.com/mesonbuild/meson/issues/1383
env = environment()
env.prepend('LD_LIBRARY_PATH', meson.build_root() + '/src')

tests = ['check_nodeset']

foreach t: tests
    e = executable(t, t + '.c',
                   dependencies: [check, rbh_lfind],
                   include_directories: include_dirs)
    test(t, e, env: env)
endforeach
