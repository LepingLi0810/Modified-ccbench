ccbench
=======
Use rdtsc() and rdtscp() instructions to monitor performance.
Changes start at line 408 in ccbench.c


Interpreting the results:
-------------------------

The comments prefixed with "#>>" explain the results.

./ccbench -r100000 -t0 -a5
<pre>
#>> settings:
test:    STORE_ON_MODIFIED  / #cores: 2 / #repetitions: 100000 / stride: 2048 (128 kiB)  / fence:  none
core1:   0 / core2:   1 
* warning: avg pfd correction is 16.2 with std deviation: 7.5%. Recalculating.
* warning: setting pfd correction manually
* warning: no default value for pfd correction is provided (fix in src/pfd.c)
* set pfd correction: 16 (std deviation: 6.9%)

* warning: avg pfd correction is 16.2 with std deviation: 7.0%. Recalculating.
* warning: setting pfd correction manually
* warning: no default value for pfd correction is provided (fix in src/pfd.c)
* set pfd correction: 17 (std deviation: 24.6%)

number of operations Core 0 performed within 5 seconds: 63450

number of operations Core 1 performed within 5 seconds: 63450

#>> The meaning of the results
[00]  ** Results from Core 0 and 1 : store on modified

#>> The final val in the cache line that was used / the sum of all loads on this core
#>> These values can be used for ensuring the correctness of some test (e.g., FAI)
[00]  value of cl is 0          / sum is 0
[01]  value of cl is 0          / sum is 0
</pre>
