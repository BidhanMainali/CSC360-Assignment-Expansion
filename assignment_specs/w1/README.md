# CSC 360: Operating Systems (Summer 2026)

## Assignment 1: `w1`

> **Spec Out**: May 13, 2026 <br>
> **Submission Due**: May 27, 2026, 11:59 PM


## Table of Contents

- [CSC 360: Operating Systems (Summer 2026)](#csc-360-operating-systems-summer-2026)
  - [Assignment 1: `w1`](#assignment-1-w1)
  - [Table of Contents](#table-of-contents)
  - [Question 1](#question-1)
  - [Question 2](#question-2)
  - [Question 3](#question-3)
  - [Submission Guidelines](#submission-guidelines)

## Question 1

Most modern processors provide two modes of operation: user mode and kernel mode. Please answer the following questions concisely in a bullet point format.

(a) What are the main differences between these two modes? `[0.5]`

(b) From the viewpoint of operating systems, why are they needed? `[0.5]`

(c) What are the main differences between mode switch and context switch? `[0.5]`

(d) What are the pros and cons of micro-kernel structures in operating systems? `[0.5]`


## Question 2

In the following example, assume all system and library calls are always completed without error.

```C
#define OUTPUT printf("%d\n", i)

int main() {
    int i=0;
    OUTPUT;
    if (fork()) {
        i+=2;
        OUTPUT;
    } else {
        i+=1;
        OUTPUT;
        return(0);
    }
}
```

(a) Please write down all possible outputs when running this program. `[1.0]`

(b) Add one system call in the pseudo code to ensure that the output values are always in increasing order. `[1.0]`

## Question 3

Processes have three major states: running, blocked (also known as waiting), and ready. For each of the following possible state transitions, explain whether it is feasible: if feasible, give an example; if not, give reason. `[6.0]`

(a) running-to-blocked

(b) blocked-to-running

(c) blocked-to-ready

(d) ready-to-blocked

(e) ready-to-running

(f) running-to-ready

## Submission Guidelines

- You must submit this assignment using the department GitLab server at `https://gitlab.csc.uvic.ca/`.

- Please click the blue `New project` button at `https://gitlab.csc.uvic.ca/courses/2026051/CSC360_COSI/assignments/<netlink_id>` to create a new ***repository/project*** named `w1`, e.g., `https://gitlab.csc.uvic.ca/courses/2026051/CSC360_COSI/assignments/<netlink_id>/w1`.

- Please write your answers of the above questions in **plain text** format in a single file named **`w1-<netlink_id>-V#.txt`** and commit it to your `w1` repository.

- Your submission for the `w1` assignment is the last commit before the due date. If you commit and push your written assignment after the due date, the last commit before the due date will be considered your final submission.
