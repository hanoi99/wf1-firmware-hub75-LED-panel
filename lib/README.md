# Vendored libraries

PlatformIO compiles project libraries stored in one directory per library under
this folder. The four external Arduino libraries used by the current WF1 build
are copied here by `scripts/vendor-dependencies.sh` from the already-resolved
`.pio/libdeps/huidu_hd_wf1` cache.

The script copies the exact versions that were used by the successful local
build, retaining upstream source files, metadata and license files. Do not
replace these directories with a newer release without testing the WF1 build.

See `../third_party/THIRD_PARTY_NOTICES.md` for versions, upstream attribution
and license information. Each library's own license and copyright notices
remain in its vendored directory.
