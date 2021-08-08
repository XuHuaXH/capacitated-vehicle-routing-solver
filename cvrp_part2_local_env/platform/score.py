# coding=utf-8

import csv
import math
from argparse import ArgumentParser

argparser = ArgumentParser()
argparser.add_argument(
    "-c",
    "--inputfile_cost",
    required=True,
    help="cost output file",
)

args = argparser.parse_args()

inputfile_cost = args.inputfile_cost

with open(inputfile_cost, "r") as f:
    r = csv.reader(f, delimiter=',')
    for i, row in enumerate(r):
        cost = float(row[1])
        if math.isnan(cost):
            cost = 0
        print(f"[{row[0]}] cost: {cost}")
