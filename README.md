p4c-multip4: Multi-piplined FPGA P4 backend compiler
===================================================

## Table Analyzer

Table Analyzer analyzes the dependency among tables and actions. Currently
Table Analyzer presents the tables and corresponding keys (matches) and 
actions.

## Getting started

1. Make sure that you have `p4c` compiler which works properly.
   - Current p4c version: 77ecabc (there is a bug on `is_const_table`, so you 
     need to fix it.)

2. Clone this repository. `git clone [...]`
3. Make a symbolic link to `p4c/extensions` directory.
  - `ln -s [path_to_your_p4c-multip4] [p4]/p4c/extensions/p4c-multip4`
4. Build it! `cd [p4]/p4c/build & make -j10`
5. Go to `test` directory, and test some p4 programs.
  - Currently p4c-multip4 does not include directory `p4include` automatically. 
  - You need to change `<v1model.p4>` and `<core.p4>` to 
    `"p4include/v1model.p4"` and `"p4include/core.p4"`, respectively.

## Contact Info

- Seungbin Song ([sbsong@postech.ac.kr](mailto:sbsong@postech.ac.kr))
