# coding=utf-8

import sys
import json
from itertools import groupby
from argparse import ArgumentParser

argparser = ArgumentParser()
argparser.add_argument(
    "-n",
    "--depot_name",
    required=True,
    help="depot name",
)
argparser.add_argument(
    "-l",
    "--inputfile-loads",
    required=True,
    help="input file for loads",
)
argparser.add_argument(
    "-d",
    "--inputfile-time",
    required=True,
    help="input file for time matrix",
)
argparser.add_argument(
    "-r",
    "--inputfile-routes",
    required=True,
    help="input file for routes",
)
argparser.add_argument(
    "-c",
    "--inputfile_cost",
    required=True,
    help="cost output file",
)

args = argparser.parse_args()

depot  = args.depot_name
filename_loads  = args.inputfile_loads
filename_routes = args.inputfile_routes
filename_time   = args.inputfile_time
inputfile_cost   = args.inputfile_cost

vol_1_capacity = 1.000
vol_2_capacity = 1.000

with open(filename_loads, "r") as f:
    loads = json.load(f)

with open(filename_time, "r") as f:
    times = json.load(f)

# Validate routes
print("Validate route format:")
try:
    with open(filename_routes, "r") as f:
        routes = json.load(f)

    for i, route in enumerate(routes):
        if "k" not in route or "route" not in route or "loads" not in route:
            print(f"  error: 'k', 'route', or 'loads' does not exist at route[{i}]")
            exit(1)

        if not isinstance(route["k"], int):
            print(f"error: 'k' {route['k']} must be an integer at route[{i}]")
            exit(1)

        if not route["route"][0].startswith('depot_'):
            print(f"  error: 'route' must start from depot at route[{i}]")
            exit(1)

        if not route["route"][-1].startswith('store_'):
            print(f"  error: 'route' must end at store at route[{i}]")
            exit(1)

        for store in route["route"][1:]:
            if store not in loads:
                print(f"  error: Invalid 'store' {store} at route[{i}]")
                exit(1)

        for node1, node2 in zip(route["route"][:-1] + [route["route"][-1]], route["route"][1:] + [route["route"][0]]):
            if node2 not in times[node1]:
                print(f"  error: node {node1} and node {node2} in 'loads' is not connected at route[{i}]")
                exit(1)

        for load_in_route in route["loads"]:
            store = load_in_route["node"]
            if store not in loads:
                print(f"  error: node {store} in 'loads' not exist at route[{i}]")
                exit(1)

            load_idx_in_node = load_in_route["load_idx_in_node"]
            if not isinstance(load_idx_in_node, int) or len(loads[store]) <= load_idx_in_node:
                print(f"  error: Invalid 'load_idx_in_node' {load_idx_in_node} at route[{i}][{store}]")
                exit(1)

        routes_in_load = [load_in_route['node'] for load_in_route in route["loads"]]
        invalid_node = [node for node in routes_in_load if node not in route["route"]]
        if len(invalid_node) > 0:
            print(f"  error: node {invalid_node} in 'loads' must exist in 'route' at route[{i}]")
            exit(1)

        nodes_in_loads = [x[0] for x in groupby([x["node"] for x in route["loads"]])]  # Remove consecutive duplicates
        nodes_in_route = [x[0] for x in groupby(route["route"][1:])]
        route_idx = 0
        for node_in_load in nodes_in_loads:
            while route_idx < len(nodes_in_route):
                node_in_route = nodes_in_route[route_idx]
                route_idx += 1
                if node_in_route != node_in_load:
                    break
        if len(nodes_in_route) != route_idx:
            print(f"  error: nodes of 'loads' must be defined in the same order as 'route' at route[{i}]")
            exit(1)

except Exception as e:
    print(f"  error: Invalid JSON format {filename_routes}.", e)
    exit(1)

print("  pass")
print("")

print("Check if each load is carried exactly once:")
for store in loads.keys():

    num_loads_at_store = len(loads[store])

    for load_idx in range(num_loads_at_store):

        num_routes_carrying_this_load = 0
        for route in routes:
            for load_in_route in route["loads"]:
                if load_in_route["node"] == store and load_idx == load_in_route["load_idx_in_node"]:
                    num_routes_carrying_this_load += 1

        if num_routes_carrying_this_load < 1:
            print("  load with store ", store, " and load_idx_in_node ", load_idx, " is not carried.")
            exit(1)

        elif num_routes_carrying_this_load > 1:
            print("  load with store ", store, " and load_idx_in_node ", load_idx, " is carried more than once.")
            exit(1)

print("  pass")
print("")

print("Check if each route satisfies vehicle capacity:")
for route in routes:
    vol_1_in_route = 0
    vol_2_in_route = 0
    for load_in_route in route["loads"]:
        store = load_in_route["node"]
        load_idx_in_node = load_in_route["load_idx_in_node"]

        vol_1_in_route += loads[store][load_idx_in_node]["vol_1"]
        vol_2_in_route += loads[store][load_idx_in_node]["vol_2"]

    if vol_1_in_route > vol_1_capacity * route["k"]:
        print(f"  vol_1 capacity violated by route: {vol_1_in_route} > {vol_1_capacity * route['k']}")
        print("    ", route)
        exit(1)
    if vol_2_in_route > vol_2_capacity * route["k"]:
        print(f"  vol_2 capacity violated by route: {vol_2_in_route} > {vol_2_capacity * route['k']}")
        print("    ", route)
        exit(1)

print("  pass")
print("")


cost_of_routes = 0
for route in routes:
    time_of_route = 0
    node_prev = None

    if(route["route"][-1].startswith('store_')) and route["route"][0].startswith('depot_'):
        route["route"].append(route["route"][0])

    for node in route["route"]:
        if node_prev is not None:
            time_of_route += times[node_prev][node]
        node_prev = node
    cost_of_route = time_of_route * route["k"]
    cost_of_routes += cost_of_route

print("cost:", cost_of_routes)

with open(inputfile_cost, "a+") as f: 
    f.write(f"{depot},{cost_of_routes}\n")
