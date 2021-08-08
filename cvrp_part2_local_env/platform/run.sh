#!/usr/bin/env bash

cd /platform

__COSTS=/out/cost.txt
[ -f $__COSTS ] && rm $__COSTS

for i in "02" "03" "05" "21" "55"
do
    echo --- depot_00$i
    __MATRIX=/platform/dataset/depot_00$i/time_matrix_depot_00$i.json
    __LOADS=/platform/dataset/depot_00$i/loads_depot_00$i.json
    __OUT=/out/routes_depot_00$i.json

    [ -f $__OUT ] && rm $__OUT

    cd /code

    # Preprocess phase
    echo "[preprocess] Start"
    time timeout --foreground 605s ./preprocess.sh depot_00$i $__MATRIX 600
    if [ $? -ne 0 ]; then
        echo "[Preprocess] Failed"
        exit 1
    fi
    echo "[preprocess] End"

    # Solve phase
    echo "[solve] Start"
    time timeout --foreground 905s ./solve.sh depot_00$i $__MATRIX $__LOADS $__OUT 900
    __SOLVE_EXIT_CODE=$?
    if [ $__SOLVE_EXIT_CODE -ne 0 ]; then
        [ $__SOLVE_EXIT_CODE == 124 ] && echo "[solve] Timed out" || echo "[solve] Failed" 
        exit 1
    fi
    echo "[solve] End"
    echo

    # Evaluate phase
    echo "[evaluate] Start"
    cd /platform
    python evaluate.py -n depot_00$i -l $__LOADS -d $__MATRIX -r $__OUT -c $__COSTS
    if [ $? -ne 0 ]; then
        echo "[evaluate] Failed"
        exit 1
    fi
    echo "[evaluate] End"
    echo
done

echo ---
python score.py -c $__COSTS
