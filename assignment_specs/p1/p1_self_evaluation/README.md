P1 Self Evaluation Tool
=================

+ Download the marking tool binary file based on your operating system and architecture from [bin](./bin).

+ **Note** that we provide the self evaluation tool in multiple operating systems and architectures here, however, the final marking will be conducted on `linux.csc.uvic.ca`.

+ Put the evaluation tool binary file in the same directory as your `Makefile`. Your produced `ssi` binary should be in the same directory as the `Makefile`.

+ Execute the evaluation tool binary file from the command line. It will automatically perform the steps as defined in the rubric.

+ The evaluation tool will create a series of temporary directory and files during the tests for `cd` and `ls` tests, which will be removed afterwards.

+ The evaluation tool will generate a `self-marking.log` file in the same directory. A sample `self-marking.log` ([V00.log](./V00.log)) is available for reference.

+ Note that the tests included in the self evaluation tool only represent a subset of the tests that will be used to mark your assignment. It is **NOT** an exhaustive list of tests.

+ **Note** that the output of `ls` shown in the evaluation tool output is in the following form

```
2024/09/30 20:05:26 clarkzjw@linux200: /home/clarkzjw/CSC360/V00 > ls temp
A
B
```

Instead of

```
2024/09/30 20:05:26 clarkzjw@linux200: /home/clarkzjw/CSC360/V00 > ls temp
A  B
```

You do not need to worry about this.
