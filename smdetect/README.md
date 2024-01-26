## README

This is just a backup, the original is available at [https://www.48bits.com/projects/smdetect.cpp](https://www.48bits.com/projects/smdetect.cpp).

### Purpose

The purpose of this PoC is to detect whether the system is running under a hypervisor that utilizes *memory deduplication*, such as Linux KSM (Kernel Samepage Merging).

During the time this project was created (2010), many hypervisors were using memory deduplication as a default practice to optimize memory usage, especially when running multiple instances of the same guest operating system.

However, using memory deduplication has significant security implications. It could be abused by attackers for several purposes beyond VMM detection. For instance, it could be leveraged as a side-channel attack, information leak, or covert-channel. It may also introduce [new exploitation vectors](https://www.cs.vu.nl/~kaveh/pubs/pdf/ffs-usenixsec16.pdf) to other vulnerabilities like Rowhammer.

