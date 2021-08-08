## Run the solution on your machine
1. Build docker image

    ```sh
    docker build ./code -t cvrp_part2
    ```

2. Evaluate your code

    ```sh
    # Host machine
    docker run --rm -it \
      --mount type=bind,src=`pwd`/platform,dst=/platform,readonly \
      --mount type=bind,src=`pwd`/code,dst=/code \
      --mount type=bind,src=`pwd`/dest,dst=/dest \
      --mount type=bind,src=`pwd`/out,dst=/out \
      cvrp_part2 /bin/bash

    # Docker container
    platform/run.sh
    ```

## Submit the solution

1. Make a zip file

    ```sh
    zip -r code.zip code
    ```

    - Don't need to include other than "code" directory.
    - The zip file size must be less than 50MB.
    - Make sure it doesn't include temporary files(e.g. `.venv`) or any personal information.

2. Submit the zip file via Challenge page

    Challenge  page URL: https://www.topcoder.com/challenges/de291e9a-995b-4661-948d-952ee5433a4e


## Directory structure

- `platform`  directory: This is a scoring platform emulator. It's provided only for local development.
- `code` directory: You can edit inside the directory. Do not rename `preprocess.sh` and `solve.sh` files which are the entrypoint, called by a platform.
- `dest`  directory: The scoring process is divided into two phases, "preprocess" and "solve". Each phase runs independently. You can use it a place to store the output of "preprocess" phase then use it at "solve" phase.
- `out` directory: A storage for result files: "routes_depot_****.json".

```
├── code
│   ├── Dockerfile
│   ├── preprocess.sh
│   ├── sample_solution.cpp
│   └── solve.sh
├── code.zip
├── dest
│   ├── empty.txt
├── example_out
│   ├── routes_depot_0002.json
├── out
│   ├── empty.txt
└── platform
    ├── dataset
    │   ├── depot_0002
    │   │   ├── loads_depot_0002.json
    │   │   └── time_matrix_depot_0002.json
    │   ├── depot_0003
    │   │   ├── loads_depot_0003.json
    │   │   └── time_matrix_depot_0003.json
    │   ├── depot_0005
    │   │   ├── loads_depot_0005.json
    │   │   └── time_matrix_depot_0005.json
    │   ├── depot_0021
    │   │   ├── loads_depot_0021.json
    │   │   └── time_matrix_depot_0021.json
    │   └── depot_0055
    │       ├── loads_depot_0055.json
    │       └── time_matrix_depot_0055.json
    ├── evaluate.py
    ├── run.sh
    └── score.py
```
