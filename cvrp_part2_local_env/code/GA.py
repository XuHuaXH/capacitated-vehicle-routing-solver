from solve import Solver, Parcel, Route
import sys
import random
import heapq
import time


depot_name = sys.argv[1]
time_matrix_path = sys.argv[2]
loads_path = sys.argv[3]
out_path = sys.argv[4]
unordered_time_matrix_path = sys.argv[5]


class Solution:
    # gene is just a single solution
    def __init__(self, gene):
        self.gene = gene
        self.score = 0



class GeneticSolver(Solver):


    def __init__(self, poplulation_size, elite_size, mutation_rate, number_of_mutations,
                 improvement_rounds, initialization_rounds, foreigner_rate):
        super().__init__()

        # contains a list of solutions
        self.population = []

        self.start_time = time.time()

        self.poplulation_size = poplulation_size
        self.elite_size = elite_size
        self.mutation_rate = mutation_rate
        self.number_of_mutations = number_of_mutations
        self.improvement_rounds = improvement_rounds
        self.initialization_rounds = initialization_rounds
        self.foreigner_rate = foreigner_rate

    # takes a Solution object
    def compute_score(self, solution):
        return self.compute_solution_cost(solution.gene)


    def generate_solution(self):
        self.close_neighbors_combine_greedy()
        solution = Solution(self.solution)
        solution.score = self.compute_score(solution)
        return solution



    # returns a list of Solutions ranked high to low by fitness score
    def generatePopulation(self):
        population = []

        max_heap = []
        for i in range(self.initialization_rounds):
            solution = self.generate_solution()
            if len(max_heap) < self.poplulation_size:
                heapq.heappush(max_heap, (-solution.score, solution))
            elif (-solution.score) > max_heap[0][0]:
                heapq.heappop(max_heap)
                heapq.heappush(max_heap, (-solution.score, solution))

        for (score, solution) in max_heap:
            population.append(solution)

        population.sort(key=(lambda x: x.score))
        return population


    # The best elite_size many individuals are selected directly.
    # For the rest in the population, each one is selected with
    # probability based on its score
    def select(self, ranked_population):
        roulette_wheel = []
        scores = [i.score for i in ranked_population]
        highest_score = scores[len(scores) - 1] + 3
        cumulative_sum = 0
        for i in range(len(scores)):
            cumulative_sum += int(highest_score - scores[i])
            roulette_wheel.append(cumulative_sum)

        # the ith individual in the population will be selected
        # if the number drawn is in [roulette_wheel[i - 1], roulette_wheel[i])
        selected = []

        for i in range(self.elite_size):
            selected.append(ranked_population[i])
        print('cumulative sum: ', cumulative_sum)
        for i in range(self.elite_size, len(ranked_population)):
            draw = random.randint(0, cumulative_sum - 1)
            for j in range(len(roulette_wheel)):
                if draw < roulette_wheel[j]:
                    selected.append(ranked_population[i])
                    break

        selected.sort(key=(lambda x: x.score))
        # for sol in selected:
        #     print('in selected: ', sol.score)
        print('checking unique in select...')
        self.check_if_unique(selected)
        return selected


    # def generate_next_map(self, solution):
    #     if len(solution) == 0:
    #         print('length of the solution is too small!')
    #         exit()
    #
    #     dict = {}
    #     ends = set(()) # all parcels at the end of a route
    #     first_parcel = solution[0].loads[0]
    #     last_parcel = first_parcel
    #
    #     for route in solution:
    #         for i in range(len(route.loads)):
    #             parcel = route.loads[i]
    #             if i == len(route.loads) - 1:
    #                 ends.add(parcel)
    #             if parcel == first_parcel:
    #                 continue
    #             dict[last_parcel] = parcel
    #             last_parcel = parcel
    #     return ends, dict


    # # implementing AEX crossover of parents' genes
    # # parents are Solution objects
    # def AEX_breed(self, parent1, parent2):
    #
    #     ends1, dict1 = self.generate_next_map(parent1.gene)
    #     ends2, dict2 = self.generate_next_map(parent2.gene)
    #
    #     child_gene = []
    #     unused_parcels = set(())
    #
    #     curr_parcel = parent1.gene[0].loads[0]
    #     curr_route = Route()
    #     curr_route.add(curr_parcel)
    #
    #     for route in parent1.gene:
    #         for parcel in route.loads:
    #             if parcel != curr_parcel:
    #                 unused_parcels.add(parcel)
    #     for route in parent2.gene:
    #         for parcel in route.loads:
    #             unused_parcels.add(parcel)
    #
    #     curr_dict = dict1
    #     curr_ends = ends1
    #     while len(unused_parcels) > 0:
    #         if curr_parcel not in curr_dict or curr_dict[curr_parcel] not in unused_parcels:
    #             # randomly select a next parcel from the unused
    #             next_parcel = random.choice(list(unused_parcels))
    #         else:
    #             next_parcel = curr_dict[curr_parcel]
    #
    #         # check if curr_parcel is an end
    #         if curr_parcel in curr_ends:
    #             child_gene.append(curr_route)
    #             curr_route = Route()
    #
    #         curr_route.add(next_parcel)
    #         unused_parcels.remove(next_parcel)
    #         curr_parcel = next_parcel
    #         curr_dict = dict2 if curr_dict == dict1 else dict1
    #         curr_ends = ends2 if curr_ends == ends1 else ends1
    #
    #     child_gene.append(curr_route)
    #     child = Solution(child_gene)
    #     return child
    #
    #

    # take the first n routes from parent1, then put in the
    # rest of the parcels in the order of parent2
    # parents are Solution objects
    def OX_breed(self, parent1, parent2):

        child_gene = []
        added_parcels = set(())
        cut_index = random.randint(0, len(parent1.gene) - 1)
        for i in range(cut_index + 1):
            route = parent1.gene[i]
            for parcel in route.loads:
                added_parcels.add(parcel)
            child_gene.append(route)

        for route in parent2.gene:
            r = Route()
            for parcel in route.loads:
                if parcel not in added_parcels:
                    r.add(parcel)
            if len(r.loads) > 0:
                child_gene.append(r)

        child = Solution(child_gene)
        return child




    # elite_size of ranked_selected goes directly into the
    # next generation, others are children of the selected parents
    def breedPopulation(self, ranked_selected):
        next_population = []
        for i in range(self.elite_size):
            next_population.append(ranked_selected[i])
        while len(next_population) < self.poplulation_size:
            parent1 = ranked_selected[random.randint(0, len(ranked_selected) - 1)]
            parent2 = ranked_selected[random.randint(0, len(ranked_selected) - 1)]
            child = self.OX_breed(parent1, parent2)
            if child not in next_population:
                next_population.append(child)
        print('checking unique in breed population...')
        self.check_if_unique(next_population)
        return next_population



    def mutate(self, solution):
        return self.swap_parcels(solution, number_of_swaps=self.number_of_mutations)

    def mutatePopulation(self, next_generation):
        # only mutate those non-elites
        for i in range(self.elite_size, len(next_generation)):
            draw = random.random()
            if draw < self.mutation_rate:
                new_solution = self.mutate(next_generation[i].gene)
                next_generation[i] = Solution(new_solution)

        # for sol in next_generation:
        #     print('in mutate population: ', sol.score)
        print('checking unique in mutate...')
        self.check_if_unique(next_generation)
        return next_generation

    def improvePopulation(self, population, number_of_rounds):
        # print('improving the new population...')
        for round in range(number_of_rounds):
            for i in range(len(population)):
                population[i].gene = solver.combine_routes(population[i].gene, count=200)

                # split routes multiple times
                for j in range(20):
                    population[i].gene = solver.split_routes(population[i].gene)

                population[i].gene = solver.swap_parcels(population[i].gene, number_of_swaps=3000)
                population[i].gene = solver.reorder_routes(population[i].gene)

        # print('finished improving population')
        print('checking unique in improve...')
        self.check_if_unique(population)
        return population



    def evolve(self):
        ranked_selected = self.select(self.population)
        next_generation = self.breedPopulation(ranked_selected)
        self.population = self.mutatePopulation(next_generation)
        # self.population = self.improvePopulation(self.population,number_of_rounds=self.improvement_rounds)

        # compute scores and rank the entire poplulation
        for individual in self.population:
            individual.score = self.compute_score(individual)
        self.population.sort(key=(lambda x: x.score))

    # generate the initial generation
    def initialize(self):
        self.population = self.generatePopulation()
        print('checking unique in initialize...')
        self.check_if_unique(self.population)

    def introduce_foreigners(self):
        for i in range(int((1 - self.foreigner_rate) * self.poplulation_size),
                       self.poplulation_size):
            self.population[i] = self.generate_solution()

        # compute scores and rank the entire poplulation
        for individual in self.population:
            individual.score = self.compute_score(individual)
        self.population.sort(key=(lambda x: x.score))


    def check_if_unique(self, population):
        for sol in population:
            seen = set(())
            for route in sol.gene:
                for parcel in route.loads:
                    if parcel in seen:
                        print('repeated parcels!')
                        exit()
                    seen.add(parcel)

    def solve(self, number_of_generations):
        for i in range(number_of_generations):

            if i > 0 and i % 200 == 0:
                self.introduce_foreigners()

            self.evolve()

            if i > 0 and i % 5 == 0:
                self.improvePopulation(self.population, number_of_rounds=self.improvement_rounds)

            if i > 0 and i % 1000 == 0:
                self.mutation_rate += 0.03

            if i > 0 and i % 500 == 0:
                self.number_of_mutations += 200
                # self.poplulation_size += 10
                # self.elite_size = int(0.1 * self.poplulation_size)

            if i > 0 and i % 500 == 0:
                self.solution = self.population[0].gene
                self.write_solution_to_file()

            minutes_so_far = (time.time() - self.start_time) / 60
            if minutes_so_far > 12:
                return


            print('lowest cost in generation ' + str(i) + ' : ' + str(self.population[0].score))


if __name__ == "__main__":
    random.seed()
    solver = GeneticSolver(poplulation_size=8, elite_size=2, mutation_rate=0.03,
                           number_of_mutations=400, improvement_rounds=2,
                           initialization_rounds=50, foreigner_rate=0.2)
    solver.initialize()
    solver.solve(number_of_generations=10)
    solver.solution = solver.population[0].gene
    solver.write_solution_to_file()