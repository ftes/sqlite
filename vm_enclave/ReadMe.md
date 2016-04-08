# General
- VDBE internal source files were moved here
- proxy implementations of VDBE API functions (vdbe.h) are needed to communicate with enclave via ECALLs

# Design decisions
- `sqlite3` object (DB connection handle) remains outside enclave
  - TODO: security analysis, could e.g. manipulating `nVdbeActive` or `nVdbeWrite` lead to security-critical writing collisions of two VDBEs?
