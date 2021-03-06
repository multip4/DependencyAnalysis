# p4c-multip4: Data dependency analysis for P4 programs

** **NOTICE: This project will be merged to [p4c-psdn](https://github.com/multip4/p4c-psdn).**

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
   - Supported p4c version: 77ecabc
   - There is a bug on `is_const_table`, so you need to fix it.
   - Checkout the proper version.
   - p4c compiler github: https://github.com/p4lang/p4c

2. Install Boost Graph library. `sudo apt-get install libboost-graph-dev`
3. Clone this repository. `git clone git@github.com:multip4/DependencyAnalysis.git`
4. Rename repository as `p4c-multip4`. `mv DependencyAnalysis p4c-multip4`
5. Make a symbolic link to `p4c/extensions` directory.
   - `ln -s [path_to_your_p4c-multip4] [p4]/p4c/extensions/p4c-multip4`
6. Build it! `cd [p4]/p4c/build & make -j10`
7. Go to `test` directory, and test some p4 programs.
   - Currently p4c-multip4 does not include directory `p4include` automatically. 
   - `./p4c-multip4 [test.p4] -I[p4]/p4c/p4include`

## Contact Info

- Seungbin Song ([seungbin@yonsei.ac.kr](mailto:seungbin@yonsei.ac.kr))
