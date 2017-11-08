p4c-multip4: Multi-piplined FPGA P4 backend compiler
===================================================

# Match-action Dependency Analyzer (MDA)

Match-action Dependency Analyzer (MDA) analyzes the dependency among
match-action tables and actions.

## Getting started

1. Make sure that you have `p4c` compiler which works properly.

2. Clone this repository.

3. Make a symbolic link to `p4c/extensions` directory.

```
ln -s [path_to_your_p4c-multip4] [~/p4/p4c/extensions/p4c-multip4]
```

4. Build it!

## To-do

1. Make table-actions list with `Visitor`, `P4Table::getActionList()`.

2. Make table-reads list with `Visitor`, `P4Table::getKey()`.

3. What is in `ActionListElement.expression`?
