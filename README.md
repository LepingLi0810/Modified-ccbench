ccbench
=======
Add time parameter and thread placement

new options:

  -a, --duration <int>
        runtime of the test(in seconds) (default=5)
  
  -b, --threads <int>
        number of threads to create (default=4)


Interpreting the results:
-------------------------

The comments prefixed with "#>>" explain the results.

[leping@snares-02] (9)$ ./ccbench -a15 -b6 -c6 -t0 -r10000000
  
test:    STORE_ON_MODIFIED  / #cores: 6 / #repetitions: 10000000 / stride: 2048 (128 kiB)  / fence:  none
  
5 thread = 8 cpu
  
4 thread = 7 cpu
  
3 thread = 6 cpu
  
2 thread = 2 cpu
  
1 thread = 1 cpu
  
0 thread = 0 cpu
  
* warning: avg pfd correction is 30.5 with std deviation: 2233763527852936704.0%. Recalculating.
  
* warning: avg pfd correction is 30.5 with std deviation: 2233763542496078080.0%. Recalculating.
  
* warning: avg pfd correction is 30.7 with std deviation: 171.5%. Recalculating.
  
* warning: avg pfd correction is 30.7 with std deviation: 171.5%. Recalculating.
  
* warning: avg pfd correction is 30.7 with std deviation: 171.5%. Recalculating.
  
* warning: avg pfd correction is 30.7 with std deviation: 171.5%. Recalculating.
  
* warning: setting pfd correction manually
  
* warning: no default value for pfd correction is provided (fix in src/pfd.c)
  
* set pfd correction: 18 (std deviation: 16236615652737165312.0%)

  
* warning: setting pfd correction manually
  
* warning: no default value for pfd correction is provided (fix in src/pfd.c)
  
* set pfd correction: 18 (std deviation: 16236560247138439168.0%)

  
* warning: setting pfd correction manually
  
* warning: no default value for pfd correction is provided (fix in src/pfd.c)
  
* set pfd correction: 27 (std deviation: 141.4%)

  
* warning: setting pfd correction manually
  
* warning: no default value for pfd correction is provided (fix in src/pfd.c)
  
* set pfd correction: 27 (std deviation: 141.4%)

  
* warning: setting pfd correction manually
  
* warning: no default value for pfd correction is provided (fix in src/pfd.c)
  
* set pfd correction: 27 (std deviation: 141.4%)

  
* warning: setting pfd correction manually
  
* warning: no default value for pfd correction is provided (fix in src/pfd.c)
  
* set pfd correction: 27 (std deviation: 141.4%)
  

id 00 operation_executed 37315 schedstat 14997469498 2602014 115
  
id 03 operation_executed 37315 schedstat 14996736407 487683 66
  
id 04 operation_executed 37315 schedstat 14996707076 408299 71
  
id 02 operation_executed 37315 schedstat 14988748721 8408886 80
  
id 01 operation_executed 37315 schedstat 14996178412 318418 52
  
id 05 operation_executed 37315 schedstat 14993440997 3697165 64
  
Total Exeuctions = 223890
  
Average atomic execution time(ns) = 66997.186118
  
Per thread execution average = 37315.000000
  

</pre>
