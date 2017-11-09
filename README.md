p4c-multip4: Multi-piplined FPGA P4 backend compiler
===================================================

## Table Analyzer

Table Analyzer analyzes the dependency among tables and actions. Currently
Table Analyzer presents the tables and corresponding keys (matches) and 
actions.

## Getting started

1. Make sure that you have `p4c` compiler which works properly.

2. Clone this repository.

3. Make a symbolic link to `p4c/extensions` directory.

```
ln -s [path_to_your_p4c-multip4] [~/p4/p4c/extensions/p4c-multip4]
```

4. Build it!

## To-do

1. Analyze action inside!

2. How to use `LOG()` instead of `std::cout`? 
