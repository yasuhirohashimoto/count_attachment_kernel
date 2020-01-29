# count_attachment_kernel

The codes were confirmed to be compiled in the following environment:
* gcc (Ubuntu 7.4.0-1)
* libboost-1.65.1

such as
```
% g++ count_attachment_kernel.cpp -O3 -std=c++17
```

The input file is supposed to be a tab-separated file such as
```
# some comments
L0C0  L0C1 L0C2  L0C3
L1C0  L1C1 L1C2  L1C3  L1C4  L1C5
L2C0  L2C1 L2C2  L2C3  L2C4
...
```
where each line corresponds to edges added at each time step.
If `1` is passed as the first argument such as
```
% cat <input-file> | ./a.out 2 > <output-file>
```
the columns before C2, that is L\*C0 and L\*C1 in this case, are ignored.
