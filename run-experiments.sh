#!/usr/bin/env bash

# usage: call when args not parsed, or when help needed
usage () {
    echo "usage: run-experiments.sh [-h] [-a] [-e experiment]"
    echo "  -h                help message"
    echo "  -a                run all experiments"
    echo "  -e n              run only experiment n"
    echo "  experiment 1      single my_ccbench"
    echo "  experiment 2      single lock-free linked list"
    echo "  experiment 3      two my_ccbench"
    echo "  experiment 4      one my_ccbench, one lock-free linked list"
    echo "  experiment 5      one my_ccbench, one original_ccbench"
    echo "  experiment 6      two my_ccbench but lightweight"
    return 0
}

#go_my_ccbench: go to my_ccbench directory
go_my_ccbench () {
  MY_CCBENCH_DIR=./Modified-ccbench
  echo $MY_CCBENCH_DIR
  cd $MY_CCBENCH_DIR
}

#build_my_ccbench: run Makefile to build my_ccbench
build_my_ccbench () {
  make clean
  make
}

#build_linked_list: run Makefile to build lock-free linked list
build_linked_list () {
  make clean_linked_list
  make linked_list
}

#run_single_ccbench: run one my_ccbench
run_single_ccbench () {
  go_my_ccbench
  build_my_ccbench
  if [ -d "./one_ccbench" ]; then
    echo "one_ccbench directory already exists, please remove it first"
    exit 1
  fi
  mkdir one_ccbench

  for a in 5 10 30 60 120
  do
    b=2
    while [ $b -lt 41 ] 
    do
      for d in 0 1 2 3
      do
        t=12
        while [ $t -lt 24 ]
        do
          echo "./ccbench -a$a -b$b -d$d -t$t > ./one_ccbench/duration${a}_threads${b}_placement${d}_test${t}"
          ./ccbench -a$a -b$b -d$d -t$t > ./one_ccbench/duration${a}_threads${b}_placement${d}_test${t}
          t=`expr $t + 1`
        done
      done
      b=`expr $b + 1`
    done
  done
}

#run_single_linked_list: run single lock-free linked list
run_single_linked_list () {
  go_my_ccbench
  build_linked_list
  if [ -d "./one_linked_list" ]; then
    echo "one_linked_list directory already exists, please remove it first"
    exit 1
  fi
  mkdir one_linked_list

  for a in 5 7
  do
    b=2
    while [ $b -lt 41 ] 
    do
      for d in 0 1 2 3
      do
        echo "./linked_list $a $b $d 0 > ./one_linked_list/duration${a}_threads${b}_placement${d}"
        ./linked_list $a $b $d 0 > ./one_linked_list/duration${a}_threads${b}_placement${d}
      done
      b=`expr $b + 1`
    done
  done
}


#run_two_my_ccbench: Have two my_ccbench, run them individually first, then together
run_two_my_ccbench () {
  go_my_ccbench
  build_my_ccbench
  if [ -d "./two_ccbench" ]; then
    echo "two_ccbench directory already exists, please remove it first"
    exit 1 
  fi
  mkdir two_ccbench
  for a in 5 10 30 60 120
  do
    b1=2
    while [ $b1 -lt 41 ] 
    do
      t1=12
      while [ $t1 -lt 24 ]
      do
        b2=1
        while [ $b2 -lt `expr 41 - $b1` ]
        do
          t2=12
          while [ $t2 -lt 24 ]
          do 
            for d in 0 1 2
            do
              case "$d" in
      	        0)
      	          #hyperthreading
                  g=`expr $b1 / 2`
                  if [[ `expr $b1 % 2` != 0 ]] && [[ $b2 > 1 ]]; then
                    g=`expr $g + 1`
                  fi
                  while [ `expr $g + $(($b2 + 1)) / 2` -lt 21 ]
                  do
                    ./ccbench -a$a -b$b1 -d$d -t$t1 > ./two_ccbench/duration${a}_threads${b1}_${b2}_placement${d}_test${t1}_${t2}_start${g}
                    ./ccbench -a$a -b$b2 -d$d -t$t2 -g$g >> ./two_ccbench/duration${a}_threads${b1}_${b2}_placement${d}_test${t1}_${t2}_start${g}
                    echo "./ccbench -a$a -b$b1 -d$d -t$t1 & ./ccbench -a$a -b$b2 -d$d -t$t2 -g$g"
                    ./ccbench -a$a -b$b1 -d$d -t$t1 > ./two_ccbench/temp1 & ./ccbench -a$a -b$b2 -d$d -t$t2 -g$g > ./two_ccbench/temp2
                    cat ./two_ccbench/temp1 >> ./two_ccbench/duration${a}_threads${b1}_${b2}_placement${d}_test${t1}_${t2}_start${g}
                    cat ./two_ccbench/temp2 >> ./two_ccbench/duration${a}_threads${b1}_${b2}_placement${d}_test${t1}_${t2}_start${g}
                    rm -f ./two_ccbench/temp*
                    g=`expr $g + 1`
                  done
                  ;;
                1)
                  #intra-socket
                  g=$b1
                  while [ `expr $g + $b2` -lt 41 ]
                  do
                    ./ccbench -a$a -b$b1 -d$d -t$t1 > ./two_ccbench/duration${a}_threads${b1}_${b2}_placement${d}_test${t1}_${t2}_start${g}
                    ./ccbench -a$a -b$b2 -d$d -t$t2 -g$g >> ./two_ccbench/duration${a}_threads${b1}_${b2}_placement${d}_test${t1}_${t2}_start${g}
                    echo "./ccbench -a$a -b$b1 -d$d -t$t1 & ./ccbench -a$a -b$b2 -d$d -t$t2 -g$g"
                    ./ccbench -a$a -b$b1 -d$d -t$t1 > ./two_ccbench/temp1 & ./ccbench -a$a -b$b2 -d$d -t$t2 -g$g > ./two_ccbench/temp2
                    cat ./two_ccbench/temp1 >> ./two_ccbench/duration${a}_threads${b1}_${b2}_placement${d}_test${t1}_${t2}_start${g}
                    cat ./two_ccbench/temp2 >> ./two_ccbench/duration${a}_threads${b1}_${b2}_placement${d}_test${t1}_${t2}_start${g}
                    rm -f ./two_ccbench/temp*
                    g=`expr $g + 1`
                  done
                  ;;
                2)
                  #inter-socket
                  g=`expr $b1 / 2`
                  if [[ `expr $b1 % 2` != 0 ]] && [[ $b2 > 1 ]]; then
                    g=`expr $g + 1`
                  fi
                  while [ `expr $g + $(($b2 + 1)) / 2` -lt 41 ]
                  do
                    ./ccbench -a$a -b$b1 -d$d -t$t1 > ./two_ccbench/duration${a}_threads${b1}_${b2}_placement${d}_test${t1}_${t2}_start${g}
                    ./ccbench -a$a -b$b2 -d$d -t$t2 -g$g >> ./two_ccbench/duration${a}_threads${b1}_${b2}_placement${d}_test${t1}_${t2}_start${g}
                    echo "./ccbench -a$a -b$b1 -d$d -t$t1 & ./ccbench -a$a -b$b2 -d$d -t$t2 -g$g"
                    ./ccbench -a$a -b$b1 -d$d -t$t1 > ./two_ccbench/temp1 & ./ccbench -a$a -b$b2 -d$d -t$t2 -g$g > ./two_ccbench/temp2
                    cat ./two_ccbench/temp1 >> ./two_ccbench/duration${a}_threads${b1}_${b2}_placement${d}_test${t1}_${t2}_start${g}
                    cat ./two_ccbench/temp2 >> ./two_ccbench/duration${a}_threads${b1}_${b2}_placement${d}_test${t1}_${t2}_start${g}
                    rm -f ./two_ccbench/temp*
                    if [ $g == 9 ]; then
                      g=`expr $g + 10`
                    fi
                    g=`expr $g + 1`
                  done
                  ;;
              esac
            done
            t2=`expr $t2 + 1`
          done
          b2=`expr $b2 + 1`
        done
        t1=`expr $t1 + 1`
      done
      b1=`expr $b1 + 1`
    done
  done
}

#run_light_weight_two_my_ccbench: run two my_ccbench but light weight
run_lightweight_two_my_ccbench () {
  go_my_ccbench
  build_my_ccbench
  if [ -d "./lightweight_two_ccbench" ]; then
    echo "lightweight_two_ccbench directory already exists, please remove it first"
    exit 1 
  fi
  mkdir lightweight_two_ccbench
  b1=6
    while [ $b1 -lt 41 ] 
    do
      b2=2
      ./ccbench -a30 -b$b1 -d0 > output1
      ./ccbench -a30 -b$b1 -d1 > output2
      ./ccbench -a30 -b$b1 -d2 > output3
        while [ $b2 -lt `expr 41 - $b1` ]
        do
            for d in 0 1 2
            do
              case "$d" in
      	        0)
      	          #hyperthreading
                  g=`expr $b1 / 2`
                  while [ `expr $g + $b2 / 2` -lt 21 ]
                  do
                    cat output1 > lightweight_two_ccbench/ccbench1_${b1}_CPUs_ccbench2_${b2}_CPUs_Hyperthreading_start${g}
                    ./ccbench -a30 -b$b2 -d$d -g$g >> lightweight_two_ccbench/ccbench1_${b1}_CPUs_ccbench2_${b2}_CPUs_Hyperthreading_start${g}
                    echo "./ccbench -a$a -b$b1 -d$d & ./ccbench -a$a -b$b2 -d$d -g$g"
                    ./ccbench -a30 -b$b1 -d$d > temp1 & ./ccbench -a30 -b$b2 -d$d -g$g > temp2
                    sleep 1
                    cat temp1 >> lightweight_two_ccbench/ccbench1_${b1}_CPUs_ccbench2_${b2}_CPUs_Hyperthreading_start${g}
                    cat temp2 >> lightweight_two_ccbench/ccbench1_${b1}_CPUs_ccbench2_${b2}_CPUs_Hyperthreading_start${g}
                    g=`expr $g + 3`
                  done
                  ;;
                1)
                  #intra-socket
                  g=$b1
                  while [ `expr $g + $b2` -lt 40 ]
                  do
                    cat output2 > lightweight_two_ccbench/ccbench1_${b1}_CPUs_ccbench2_${b2}_CPUs_Intra_Socket_start${g}
                    ./ccbench -a30 -b$b2 -d$d -g$g >> lightweight_two_ccbench/ccbench1_${b1}_CPUs_ccbench2_${b2}_CPUs_Intra_Socket_start${g}
                    echo "./ccbench -a$a -b$b1 -d$d & ./ccbench -a$a -b$b2 -d$d -g$g"
                    ./ccbench -a30 -b$b1 -d$d > temp1 & ./ccbench -a30 -b$b2 -d$d -g$g > temp2
                    sleep 1
                    cat temp1 >> lightweight_two_ccbench/ccbench1_${b1}_CPUs_ccbench2_${b2}_CPUs_Intra_Socket_start${g}
                    cat temp2 >> lightweight_two_ccbench/ccbench1_${b1}_CPUs_ccbench2_${b2}_CPUs_Intra_Socket_start${g}                   
                    g=`expr $g + 3`
                  done
                  ;;
                2)
                  #inter-socket
                  g=`expr $b1 / 2`
                  while [ `expr $g + $b2 / 2` -lt 41 ]
                  do
                    cat output3 > lightweight_two_ccbench/ccbench1_${b1}_CPUs_ccbench2_${b2}_CPUs_Inter_Socket_start${g}
                    ./ccbench -a30 -b$b2 -d$d -g$g >> lightweight_two_ccbench/ccbench1_${b1}_CPUs_ccbench2_${b2}_CPUs_Inter_Socket_start${g}
                    echo "./ccbench -a$a -b$b1 -d$d & ./ccbench -a$a -b$b2 -d$d -g$g"
                    ./ccbench -a30 -b$b1 -d$d > temp1 & ./ccbench -a30 -b$b2 -d$d -g$g > temp2
                    sleep 1
                    cat temp1 >> lightweight_two_ccbench/ccbench1_${b1}_CPUs_ccbench2_${b2}_CPUs_Inter_Socket_start${g}
                    cat temp2 >> lightweight_two_ccbench/ccbench1_${b1}_CPUs_ccbench2_${b2}_CPUs_Inter_Socket_start${g}                    
                    if [ `expr $g + 3` -ge 10 ]; then
                      g=`expr $g + 10`
                    fi
                    g=`expr $g + 3`
                  done
                  ;;
              esac
            done
          b2=`expr $b2 + 4`
        done
      b1=`expr $b1 + 6`
    done
}

#run_ccbench_linked_list: run one my_ccbench, one lock-free linked list
run_ccbench_linked_list () {
  go_my_ccbench
  build_my_ccbench
  build_linked_list
  if [ -d "./ccbench_linked_list" ]; then
    echo "ccbench_linked_list directory already exists, please remove it first"
    exit 1 
  fi
  mkdir ccbench_linked_list

  for a in 5 7
  do
    b1=2
    while [ $b1 -lt 41 ] 
    do
      t=12
      while [ $t -lt 24 ]
      do
        b2=2
        while [ $b2 -lt `expr 41 - $b1` ]
        do
            for d in 0 1 2
            do
              case "$d" in
      	        0)
      	          #hyperthreading
                  g=`expr $b1 / 2`
                  if [[ `expr $b1 % 2` != 0 ]] && [[ $b2 > 1 ]]; then
                    g=`expr $g + 1`
                  fi
                  while [ `expr $g + $(($b2 + 1)) / 2` -lt 21 ]
                  do
                    ./linked_list $a $b2 $d $g > ./ccbench_linked_list/duration${a}_threads${b1}_${b2}_placement${d}_test${t}_start${g}
                    ./ccbench -a10000 -b$b1 -d$d -t$t & ./linked_list $a $b2 $d $g >> ./ccbench_linked_list/duration${a}_threads${b1}_${b2}_placement${d}_test${t}_start${g}
                    pid=$(pidof ccbench)
                    kill $pid
                    g=`expr $g + 1`
                  done
                  ;;
                1)
                  #intra-socket
                  g=$b1
                  while [ `expr $g + $b2` -lt 41 ]
                  do
                    ./linked_list $a $b2 $d $g > ./ccbench_linked_list/duration${a}_threads${b1}_${b2}_placement${d}_test${t}_start${g}
                    ./ccbench -a10000 -b$b1 -d$d -t$t & ./linked_list $a $b2 $d $g >> ./ccbench_linked_list/duration${a}_threads${b1}_${b2}_placement${d}_test${t}_start${g}
                    pid=$(pidof ccbench)
                    kill $pid
                    g=`expr $g + 1`
                  done
                  ;;
                2)
                  #inter-socket
                  g=`expr $b1 / 2`
                  if [[ `expr $b1 % 2` != 0 ]] && [[ $b2 > 1 ]]; then
                    g=`expr $g + 1`
                  fi
                  while [ `expr $g + $(($b2 + 1)) / 2` -lt 41 ]
                  do
                    ./linked_list $a $b2 $d $g > ./ccbench_linked_list/duration${a}_threads${b1}_${b2}_placement${d}_test${t}_start${g}
                    ./ccbench -a10000 -b$b1 -d$d -t$t & ./linked_list $a $b2 $d $g >> ./ccbench_linked_list/duration${a}_threads${b1}_${b2}_placement${d}_test${t}_start${g}
                    pid=$(pidof ccbench)
                    kill $pid
                    if [ $g == 9 ]; then
                      g=`expr $g + 10`
                    fi
                    g=`expr $g + 1`
                  done
                  ;;
              esac
            done
          b2=`expr $b2 + 1`
        done
        t1=`expr $t1 + 1`
      done
      b1=`expr $b1 + 1`
    done
  done
}

#run_my_original_ccbench: run one my_ccbench, one original ccbench
run_my_original_ccbench () {
  go_my_ccbench
  build_my_ccbench
  cd ..
  cd ./ccbench
  make clean
  make
  cd ..
  if [ -d "./my_original_ccbench" ]; then
    echo "my_original_ccbench directory already exists, please remove it first"
    exit 1 
  fi
  mkdir my_original_ccbench

  
  b=2
  while [ $b -lt 39 ] 
  do
    t1=12
    while [ $t1 -lt 24 ]
    do
      for t2 in 0 1 3 4 5 7 10
      do
        for d in   2
        do
          case "$d" in
      	    0)
      	      #hyperthreading
              x=`expr $b / 2`
              y=`expr $x + 20`
              if [[ `expr $b % 2` != 0 ]]; then
                x=`expr $x + 1`
                y=`expr $y + 1`
              fi
              while [ $x -lt 20 ]
              do
                ./ccbench/ccbench -c2 -r500000 -t$t2 -x$x -y$y > ./my_original_ccbench/threads${b}_placement${d}_test${t1}_${t2}_start${x}_${y}
         ./Modified-ccbench/ccbench -a1000 -b$b -t$t1 -d$d & ./ccbench/ccbench -c2 -r500000 -t$t2 -x$x -y$y >> ./my_original_ccbench/threads${b}_placement${d}_test${t1}_${t2}_start${x}_${y}
                pid=$(pidof ccbench)
                kill $pid      
                x=`expr $x + 1`
                y=`expr $y + 1`
              done
              ;;
            1)
              #intra-socket
              x=$b
              y=`expr $x + 1`
              while [ $x -lt 40 ]
              do
                 ./ccbench/ccbench -c2 -r500000 -t$t2 -x$x -y$y > ./my_original_ccbench/threads${b}_placement${d}_test${t1}_${t2}_start${x}_${y}
         ./Modified-ccbench/ccbench -a1000 -b$b -t$t1 -d$d & ./ccbench/ccbench -c2 -r500000 -t$t2 -x$x -y$y >> ./my_original_ccbench/threads${b}_placement${d}_test${t1}_${t2}_start${x}_${y}
                pid=$(pidof ccbench)
                kill $pid                   
                x=`expr $x + 1`
                y=`expr $y + 1`
              done
              ;;
            2)
              #inter-socket
              x=`expr $b / 2`
              y=`expr $x + 10`
              if [[ `expr $b % 2` != 0 ]]; then
                x=`expr $x + 1`
                y=`expr $y + 1`
              fi
              while [ $x -lt 30 ]
              do
                 ./ccbench/ccbench -c2 -r500000 -t$t2 -x$x -y$y > ./my_original_ccbench/threads${b}_placement${d}_test${t1}_${t2}_start${x}_${y}
         ./Modified-ccbench/ccbench -a1000 -b$b -t$t1 -d$d & ./ccbench/ccbench -c2 -r500000 -t$t2 -x$x -y$y >> ./my_original_ccbench/threads${b}_placement${d}_test${t1}_${t2}_start${x}_${y}
                pid=$(pidof ccbench)
                kill $pid 
                if [ $x == 9 ]; then
                  x=`expr $x + 10`
                  y=`expr $y + 10`
                fi                  
                x=`expr $x + 1`
                y=`expr $y + 1`
              done              
              ;;
          esac
        done
      done
      t1=`expr $t1 + 1`
    done
    b=`expr $b + 1`
  done
}



allExperiment=0
args=`getopt hae: $*`
specific=$2

if [[ $# == 0 ]]; then
    usage; exit 1
fi

set -- $args

for i; do
  case "$i" in
  -h)
    usage
    exit 0
    shift;;
  -a)
    allExperiment=1
    shift;;
  -e)
    specific=$2
    shift
    number='^[1-6]+$'
    if ! [[ $specific =~ $number ]]; then
      usage
      echo "-e must be followed by a number from 1 to 6"; exit 1
    fi
    shift;;
  --)
    shift; break;;
    esac
done

if [[ $specific != "" ]]; then
  echo "Run experiment $specific"   
  case "$specific" in
    1)
      echo "run one my_ccbench"
      run_single_ccbench
      ;;
    2)
      echo "run one lock-free linked_list"
      run_single_linked_list
      ;;
    3)
      echo "run two my_ccbench"
      run_two_my_ccbench
      ;;
    4)
      echo "run one my_ccbench and one lock-free linked_list"
      run_ccbench_linked_list
      ;;
    5)
      echo "run one my_ccbench and one original_ccbench"
      run_my_original_ccbench
      ;;
    6)
      echo "run two my_ccbench but lightweight"
      run_lightweight_two_my_ccbench
      ;;
  esac
  exit 0
fi

run_single_ccbench
cd ..
run_single_linked_list
cd..
run_two_my_ccbench
cd ..
run_ccbench_linked_list
cd ..
run_my_original_ccbench

exit 0


