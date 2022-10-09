ccbench
=======
Add time parameter and thread placement

new options:

  -a, --duration <int>
        runtime of the test(in seconds) (default=5)
  
  -b, --threads <int>
        number of threads to create (default=4)
        
  -d, --placement <int>
        how to place threads (default=Hyperthreading)


Interpreting the results:
-------------------------

The comments prefixed with "#>>" explain the results.

yuvraj@node-0:~/ssd/leping/Modified-ccbench$ ./ccbench -a15 -b10 -c10 -d1 -t0 -r100000

test:    STORE_ON_MODIFIED  / #cores: 10 / #repetitions: 100000 / stride: 2048 (128 kiB)  / fence:  none

thread 6 = cpu 6

thread 7 = cpu 7

thread 8 = cpu 8

thread 9 = cpu 9

thread 5 = cpu 5

thread 4 = cpu 4

thread 3 = cpu 3

thread 2 = cpu 2

thread 1 = cpu 1

thread 0 = cpu 0


id 08 operation_executed 24666 schedstat 14999290449 101025 11

id 01 operation_executed 24666 schedstat 15000759154 442706 20

id 02 operation_executed 24666 schedstat 15000736844 397527 17

id 04 operation_executed 24666 schedstat 14998979861 386082 13

id 09 operation_executed 24666 schedstat 15000839265 123551 13

id 00 operation_executed 24666 schedstat 14997543156 3767097 79

id 03 operation_executed 24666 schedstat 15000791159 320566 14

id 07 operation_executed 24666 schedstat 14999207208 105350 15

id 05 operation_executed 24666 schedstat 14999265272 228502 8

id 06 operation_executed 24666 schedstat 15000970329 146409 20

Total Exeuctions = 246660

Average atomic execution time(ns) = 60812.454391

Per thread execution average = 24666.000000

  

</pre>
