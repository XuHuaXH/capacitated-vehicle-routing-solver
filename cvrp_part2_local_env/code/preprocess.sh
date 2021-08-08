#!/usr/bin/env bash

cd /code

DEPOT_NAME=$1
TIME_MATRIX_JSON_PATH=$2
TIME_LIMIT_SEC=$3
PROCESSED_PATH=/dest/time_matrix_$DEPOT_NAME.json


#echo $PROCESSED_PATH
python3 preprocess.py $TIME_MATRIX_JSON_PATH $PROCESSED_PATH

# Sample command
TIME_MATRIX_JSON=$(<$TIME_MATRIX_JSON_PATH)
g++ GA.cpp -o /dest/solution -O3 -std=c++20
