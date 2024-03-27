# Regiodesics

# Tools

## direction\_vectors

Generate and save brain region direction vectors.

Unit direction vectors are obtained as an 'average' of the directions of straight line segments joining the bottom shell to the top shell.
These segments join every voxel of the bottom shell to one of its closest voxels in the top shell, and vice versa.
The direction vector of a voxel is the sum of the unit direction vectors of its first `average-size` closest segments divided by its Euclidean norm.

```
    Usage: ./direction_vectors --shells SHELLS

    where SHELLS is the path to a nrrd file containing the definitions
    of a bottom and a top shell. SHELLS holds a 3D array of type char
     (int8) where bottom voxels are labeled with 3 and top voxels
    are labeled with 4.

    Options:
      -h [ --help ]                         Produce this help message.
      -v [ --version ]                      Show program description and exit.
      -s [ --shells ] arg                   Shells volume, that is, a 3D array of
                                            type char (int8), where bottom voxels
                                            are labeled with 3 and top voxels are
                                            labeled with 4.
      -a [ --average-size ] lines (=1000)   Size of k-nearest neighbour query of
                                            top to bottom lines used to approximate
                                            direction vectors.
      -o [ --output-path ] arg (=direction_vectors.nrrd)
                                            File path of the 3D unit vector field
                                            to save.
```

## Building

A `docker` file is included that can be used to build the software:

    # build the docker image
    $ docker build -t regiodesics .

    # launch the docker container
    $ docker run -it -v`pwd`:/opt/regiodesics regiodesics

    # from inside the docker container
    $ mkdir -p /opt/regiodesics/build
    $ cd /opt/regiodesics/build
    $ cmake .. -GNinja
    $ ninja

    # applications can be found in /opt/regiodesics/build/apps

## Acknowledgements

The development of this software was supported by funding to the Blue Brain Project, a research center of the École polytechnique fédérale de Lausanne (EPFL), from the Swiss government’s ETH Board of the Swiss Federal Institutes of Technology.

## License

For license and authors, see LICENSE.txt.

Copyright © 2017-2024 Blue Brain Project/EPFL
