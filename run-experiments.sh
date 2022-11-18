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
  mkdir two_my_ccbench
  for a in 5 10 30 60 120
  do
    b1=2
    while [ $b1 -lt 41 ] 
    do
      t1=12
      while [ $t1 -lt 24 ]
      do
        b2=1
        while [ $b2 -lt (41 - $b1) ]
        do
          t2=12
          while [ $t2 -lt 24 ]
          do 
            for d in 0 1 2 3
            do
              case "$d" in
      	        0)
      	          #hyperthreading
                  g=($b1 / 2)
                  if [[ ($b1 % 2) != 0 ]] && [[ ($b2 > 1) ]]; then
                    g=`exper $g + 1`
                  fi
                  while [ ($g + ($b2 / 2)) -lt 21 ]
                  ./ccbench -a$a -b$b1 -d$d -t$t1 > ./two_ccbench/duration${a}_threads${b1}_${b2}_placement${d}_test${t1}_${t2}_start${g}

                  run_single_ccbench
                  ;;
                1)
                  #intra-socket
                  run_single_linked_list
                  ;;
                2)
                  #inter-socket
                  ;;
                3)
                  #hyperthreading and inter-socket
                  echo "run one my_ccbench and one lock-free linked_list"
                  ;;
              esac
            done
            t2=`exper $t2 + 1`
          done
          b2=`exper $b2 + 1`
        done
        t1=`exper $t1 + 1`
      done
    b1=`expr $b1 + 1`
    done
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
    number='^[1-5]+$'
    if ! [[ $specific =~ $number ]]; then
      usage
      echo "-e must be followed by a number from 1 to 5"; exit 1
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
      ;;
    4)
      echo "run one my_ccbench and one lock-free linked_list"
      ;;
    5)
      echo "run one my_ccbench and one original_ccbench"
      ;;
  esac
  exit 0
fi


exit 0





