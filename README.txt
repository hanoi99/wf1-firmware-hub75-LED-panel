WF1 Vendoring script fix

The previous script incorrectly refused to complete while temporary lib_deps
were still present, even though they must remain configured until the cache
has been copied. This corrected script copies the resolved libraries first
and leaves removal of the temporary lib_deps block to the user.

Workflow:
1. Temporarily add the four pinned library dependencies to platformio.ini.
2. Run: pio run -e huidu_hd_wf1
3. Run this corrected script: bash scripts/vendor-dependencies.sh
4. Remove the temporary lib_deps block from platformio.ini.
5. Run: pio run -e huidu_hd_wf1
6. Run: pio run -e huidu_hd_wf1 -t buildfs
