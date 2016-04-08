# Compiling SQLite on Windows
1. install [TCL 8.5](http://www.activestate.com/activetcl/downloads)
2. set TCL environment variables, if TCL was not installed to `C:\tcl'
  a. TCLINCDIR="<tcl_install_dir>\include"
  b. TCLLIBDIR="<tcl_install_dir>\lib"
3. if checked out via Git, make sure current `manifest` and `manifest.uuid` files are present (fossil SCM versioning information needed by the build)
4. `nmake /f Makefile.msc`