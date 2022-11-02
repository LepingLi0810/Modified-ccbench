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

./ccbench -a10 -b16 -d3 -t12

  test:          CAS_SUCCESS  / #threads: 16 / #duration: 10s / placement:  Hyperthreading + Inter-socket  / fence:  none

  Thread 15 (CPU 33) operation_executed 2709697 

  Thread 13 (CPU 13) operation_executed 2706265 

  Thread 14 (CPU 23) operation_executed 1730254 

  Thread 04 (CPU 1) operation_executed 1746250 

  Thread 06 (CPU 21) operation_executed 1763587 

  Thread 00 (CPU 0) operation_executed 1887935 

  Thread 08 (CPU 2) operation_executed 1768900 

  Thread 10 (CPU 22) operation_executed 1777390 

  Thread 03 (CPU 30) operation_executed 2747633 

  Thread 05 (CPU 11) operation_executed 2722321 

  Thread 02 (CPU 20) operation_executed 1656956 

  Thread 07 (CPU 31) operation_executed 2694247 

  Thread 01 (CPU 10) operation_executed 2751295 

  Thread 09 (CPU 12) operation_executed 2754483 

  Thread 11 (CPU 32) operation_executed 2739232 

  Thread 12 (CPU 3) operation_executed 1731149 

  Execution: Total = 35887594, Avg = 2242974.62 Max = 2754483 Min = 1656956

  Fairness index: 0.954929

  Average atomic execution time(ns) = 278.647825


</pre>
