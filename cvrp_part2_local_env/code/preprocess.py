import json
import sys

# time_matrix_path = '../platform/dataset/depot_0002/time_matrix_depot_0002.json'
time_matrix_path = '..' + sys.argv[1]
out_path = sys.argv[2]
data = {}

with open(time_matrix_path, "r") as f:
    file_str = f.read()
    data = json.loads(file_str)

    for source in data:
        data[source] = [[key, value] for key, value in sorted(data[source].items(), key=lambda x : x[
            1])]

with open(out_path, "w") as f:
    # json.dump(data, f)

    f.write('{')
    for source in data:
        f.write('"%s": {' % source)
        for i in range(len(data[source])):
            pair = data[source][i]
            line = '"' + pair[0] + '"' + ': ' + str(pair[1])
            if i < len(data[source]) - 1:
                line += ','
            f.write(line)
        f.write('}')
    f.write('}')


# print(data)


