p4c-multip4: Multi-piplined FPGA P4 backend compiler
===================================================

## Table Analyzer

Table Analyzer analyzes the dependency among tables and actions.

- Drawing graphs is supported. Table Analyzer draws a data dependence graph of
  each control block.
  - Add `graphs->writeGraphToFile("file/name")` in `preorder(PackageBlock)`.
- Printing stats is supported. Table Analyzer calculates:
  - the number of Tables
  - the number of table-independent pairs (no data dependence between two)
  - the number of match-independent pairs (no key-action dependence but 
    action-action dependence)

## Getting started

1. Make sure that you have `p4c` compiler which works properly.
   - Current p4c version: 77ecabc (there is a bug on `is_const_table`, so you 
     need to fix it.)

2. Install Boost Graph library. `sudo apt-get install libboost-graph-dev`
3. Clone this repository. `git clone [...]`
4. Make a symbolic link to `p4c/extensions` directory.
  - `ln -s [path_to_your_p4c-multip4] [p4]/p4c/extensions/p4c-multip4`
5. Build it! `cd [p4]/p4c/build & make -j10`
6. Go to `test` directory, and test some p4 programs.
  - Currently p4c-multip4 does not include directory `p4include` automatically. 
  - `./p4c-multip4 [test.p4] -I[p4]/p4c/p4include`

## Contact Info

- Seungbin Song ([sbsong@postech.ac.kr](mailto:sbsong@postech.ac.kr))
