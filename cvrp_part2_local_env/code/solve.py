import json
import math
import sys
import random
import copy

depot_name = sys.argv[1]
time_matrix_path = sys.argv[2]
loads_path = sys.argv[3]
out_path = sys.argv[4]
unordered_time_matrix_path = sys.argv[5]
# time_matrix_path = '../platform/dataset/depot_0002/time_matrix_depot_0002.json'
# loads_path = '../platform/dataset/depot_0002/loads_depot_0002.json'
# out_path =  '../out/routes_depot_0002.json'

class Parcel:
    def __init__(self, store_name, load_index, vol1, vol2):
        self.store_name = store_name
        self.load_index = load_index
        self.vol1 = vol1
        self.vol2 = vol2

class Route:
    def __init__(self):
        self.loads = []
        self.vol1 = 0
        self.vol2 = 0

    def add(self, parcel):
        self.loads.append(parcel)
        self.vol1 += parcel.vol1
        self.vol2 += parcel.vol2

    def remove(self, parcel):
        self.loads.remove(parcel)
        self.vol1 -= parcel.vol1
        self.vol2 -= parcel.vol2

    def get_truck_size(self):
        return math.ceil(max(self.vol1, self.vol2))

    def __repr__(self):
        result_str = 'New Route: \n'
        for parcel in self.loads:
            result_str += parcel.store_name
            result_str += ' '
        return result_str


class Solver:
    def __init__(self):

        self.depot = depot_name

        # load parcels
        self.parcels = []
        with open(loads_path, "r") as f:
            data = f.read()
            data = json.loads(data)
            for store_name in data:
                parcel_list = data[store_name]
                for i in range(len(parcel_list)):
                    parcel_dict = parcel_list[i]
                    p = Parcel(store_name, i, parcel_dict['vol_1'], parcel_dict['vol_2'])
                    self.parcels.append(p)

        # compute store to parcels mapping
        self.parcels_by_store = {}
        for parcel in self.parcels:
            name = parcel.store_name
            if name not in self.parcels_by_store:
                self.parcels_by_store[name] = []
            self.parcels_by_store[name].append(parcel)

        # load dist matrix
        self.dist = {}
        with open(time_matrix_path, "r") as f:
            data = f.read()
            self.dist = json.loads(data)

        # load unordered dist matrix
        self.unordered_dist = {}
        with open(unordered_time_matrix_path, "r") as f:
            data = f.read()
            self.unordered_dist = json.loads(data)


    def write_solution_to_file(self):
        sol = []
        for route in self.solution:
            stops = [self.depot]
            loads = []
            k = route.get_truck_size()
            for parcel in route.loads:
                stops.append(parcel.store_name)
                dict = {'node': parcel.store_name, 'load_idx_in_node': parcel.load_index}
                loads.append(dict)
            sol.append({'route': stops, 'k': k, 'loads': loads})


        with open(out_path, "w") as f:
            json.dump(sol, f)

    def get_total_vol(self, store_name):
        v1 = 0
        v2 = 0
        for parcel in self.parcels_by_store[store_name]:
            v1 += parcel.vol1
            v2 += parcel.vol2
        return v1, v2

    def get_time(self, stop_name1, stop_name2):
        return self.unordered_dist[stop_name1][stop_name2]


    def compute_route_time(self, route):
        total_time = 0
        for i in range(len(route.loads) - 1):
            total_time += self.get_time(route.loads[i].store_name, route.loads[i + 1].store_name)
        total_time += self.get_time(self.depot, route.loads[0].store_name)
        total_time += self.get_time(route.loads[len(route.loads) - 1].store_name, self.depot)
        return total_time

    def compute_route_cost(self, route):
        route_time = self.compute_route_time(route)
        route_truck_size = math.ceil(max(route.vol1, route.vol2))
        return route_time * route_truck_size

    def compute_solution_cost(self, solution):
        total_cost = 0
        for route in solution:
            total_cost += self.compute_route_cost(route)
        return total_cost




    def should_combine(self, route, store_name):
        route_truck_size = math.ceil(max(route.vol1, route.vol2))
        route_time = self.compute_route_time(route)
        store_route = Route()
        for parcel in self.parcels_by_store[store_name]:
            store_route.add(parcel)
        store_time = self.get_time(self.depot, store_name) + self.get_time(store_name, self.depot)
        store_truck_size = math.ceil(max(store_route.vol1, store_route.vol2))
        old_cost = route_time * route_truck_size + store_time * store_truck_size


        new_route = Route()
        for parcel in route.loads:
            new_route.add(parcel)
        for parcel in self.parcels_by_store[store_name]:
            new_route.add(parcel)
        new_truck_size = math.ceil(max(new_route.vol1, new_route.vol2))
        new_time = self.compute_route_time(new_route)
        new_cost = new_time * new_truck_size

        # print('old cost is ', old_cost, ' new cost is ', new_cost)
        return new_cost < old_cost


    def should_split(self, route, cut_index):
        if cut_index < 1 or cut_index > len(route.loads) - 2:
            return False, 0, None, None
        old_cost = self.compute_route_cost(route)

        seg1 = Route()
        seg2 = Route()
        for i in range(cut_index + 1):
            seg1.add(route.loads[i])
        for i in range(cut_index + 1, len(route.loads)):
            seg2.add(route.loads[i])
        new_cost = self.compute_route_cost(seg1) + self.compute_route_cost(seg2)
        # print('old cost is ', old_cost, ' new cost is ', new_cost)
        if new_cost < old_cost:
            # print('splitting')
            return True, new_cost, seg1, seg2
        else:
            return False, old_cost, None, None


    def split_routes(self, solution):
        for i in reversed(range(0, len(solution))):
            route = solution[i]
            if len(route.loads) < 2:
                continue
            cut_index = random.randint(1, max(1, len(route.loads) - 2))
            split, cost, seg1, seg2 = self.should_split(route, cut_index)
            if split:
                # print('splitting...')
                solution.pop(i)
                solution.append(seg1)
                solution.append(seg2)
        return solution


    def should_two_routes_combine(self, route1, route2):
        old_cost = self.compute_route_cost(route1) + self.compute_route_cost(route2)

        route1_copy = copy.deepcopy(route1)
        for parcel in route2.loads:
            route1_copy.add(parcel)
        new_cost = self.compute_route_cost(route1_copy)

        if new_cost < old_cost:
            return True, route1_copy
        else:
            return False, None


    def combine_routes(self, solution, count):
        for i in range(count):
            idx1 = random.randint(0, len(solution) - 1)
            idx2 = idx1
            while idx2 == idx1:
                idx2 = random.randint(0, len(solution) - 1)

            should_combine, combined = self.should_two_routes_combine(solution[idx1], solution[
                idx2])
            if should_combine:
                # print(i, ' th iteration: combining...')
                solution[idx1] = combined
                solution.pop(idx2)
        return solution



    def swap(self, route, idx1, idx2):
        tmp = route.loads[idx1]
        route.loads[idx1] = route.loads[idx2]
        route.loads[idx2] = tmp

    def reorder_single_route(self, route):

        new_route = copy.deepcopy(route)

        # take the stop closest to depot to be the first
        min_time = self.get_time(self.depot, new_route.loads[0].store_name)
        min_index = 0

        for i in range(1, len(new_route.loads)):
            new_time = self.get_time(self.depot, new_route.loads[i].store_name)
            if new_time < min_time:
                min_time = new_time
                min_index = i
        self.swap(new_route, 0, min_index)

        curr_index = 0
        while curr_index < len(new_route.loads) - 2:
            min_time = self.get_time(new_route.loads[curr_index].store_name, new_route.loads[
                curr_index + 1].store_name)
            min_index = curr_index + 1
            for i in range(curr_index + 2, len(new_route.loads)):
                new_time = self.get_time(new_route.loads[curr_index].store_name, new_route.loads[
                    i].store_name)
                if new_time < min_time:
                    min_time = new_time
                    min_index = i
            self.swap(new_route, curr_index + 1, min_index)
            curr_index += 1

        # check if the reordered route is better
        old_cost = self.compute_route_cost(route)
        new_cost = self.compute_route_cost(new_route)
        if new_cost < old_cost:
            # print('old cost is ', old_cost, ' new cost is ', new_cost)
            # print('reorder to a better route...')
            return new_route
        else:
            return route




    def reorder_routes(self, solution):
        for i in range(len(solution)):
            # print('old cost of route is ', self.compute_route_cost(self.solution[i]))
            solution[i] = self.reorder_single_route(solution[i])
            # print('new cost of route is ', self.compute_route_cost(self.solution[i]))
            return solution


    def swap_parcels(self, solution, number_of_swaps):
        for i in range(number_of_swaps):

            # choose routes to swap
            r_idx1 = random.randint(0, len(solution) - 1)
            r_idx2 = r_idx1
            while r_idx2 == r_idx1:
                r_idx2 = random.randint(0, len(solution) - 1)

            route1 = solution[r_idx1]
            route2 = solution[r_idx2]
            new_route1 = copy.deepcopy(route1)
            new_route2 = copy.deepcopy(route2)

            # choose parcels to swap
            p_idx1 = random.randint(0, len(new_route1.loads) - 1)
            p_idx2 = random.randint(0, len(new_route2.loads) - 1)

            parcel1 = new_route1.loads[p_idx1]
            parcel2 = new_route2.loads[p_idx2]
            new_route1.remove(parcel1)
            new_route2.remove(parcel2)
            new_route1.add(parcel2)
            new_route2.add(parcel1)

            new_route1 = self.reorder_single_route(new_route1)
            new_route2 = self.reorder_single_route(new_route2)

            # check if result is better
            old_cost = self.compute_route_cost(route1) + self.compute_route_cost(route2)
            new_cost = self.compute_route_cost(new_route1) + self.compute_route_cost(new_route2)

            if new_cost < old_cost:
                # print('old cost is ', old_cost, ' new cost is ', new_cost)
                # print('swapping to a better result...')
                solution[r_idx1] = new_route1
                solution[r_idx2] = new_route2

        return solution

    def reverse_routes(self, solution):
        for i in range(len(solution)):
            route = solution[i]
            new_route = copy.deepcopy(route)
            new_route.loads.reverse()

            old_cost = self.compute_route_cost(route)
            new_cost = self.compute_route_cost(new_route)
            if new_cost < old_cost:
                solution[i] = new_route
        return solution



    def close_neighbors_combine_greedy(self):
        total_cost = 0
        routes = []
        added_stores = set(())

        # store_names = [l[0] for l in self.dist[self.depot]]

        store_names = list((self.parcels_by_store.keys()))
        random.shuffle(store_names)

        for store_name in store_names:
            if store_name == self.depot or store_name in added_stores:
                continue
            r = Route()
            for parcel in self.parcels_by_store[store_name]:
                r.add(parcel)
            added_stores.add(store_name)

            # check the closest neighbors
            neighbors = self.dist[store_name]
            for i in range(min(10, len(neighbors))):
                neighbor = neighbors[i]
                if neighbor[0] == self.depot or neighbor[0] == store_name or neighbor[0] in \
                        added_stores:
                    continue

                if self.should_combine(r, neighbor[0]):
                    # print('combining...')
                    added_stores.add(neighbor[0])
                    for parcel in self.parcels_by_store[neighbor[0]]:
                        r.add(parcel)
            routes.append(r)
            total_cost += self.compute_route_time(r) * math.ceil(max(r.vol1, r.vol2))
        self.solution = routes
        return total_cost







if __name__ == '__main__':
    random.seed()
    solver = Solver()
    min_cost = solver.close_neighbors_combine_greedy()
    solver.write_solution_to_file()

    # run greedy multiple times
    for i in range(5):
        cost = solver.close_neighbors_combine_greedy()
        print(i, ' th iteration: ', min_cost)
        if cost < min_cost:
            min_cost = cost
            solver.write_solution_to_file()


    for i in range(600000):
        print(solver.compute_solution_cost(solver.solution))

        # split routes multiple times
        for i in range(50):
            solver.solution = solver.split_routes(solver.solution)
        solver.solution = solver.combine_routes(solver.solution, count=30)

        solver.solution = solver.swap_parcels(solver.solution, number_of_swaps=50)

        solver.solution = solver.reorder_routes(solver.solution)
        # solver.solution = solver.reverse_routes(solver.solution)

    solver.write_solution_to_file()





