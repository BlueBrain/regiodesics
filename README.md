# Regiodesics

Tools to generate relative distance and orientation fields in 3D voxel atlases.

# Apps

## layer\_segmenter

![Screenshot of layer_segmenter application.](https://github.com/seirios/regiodesics/assets/7497849/c60e2adb-0e7b-4664-85bb-70d0978cc99d)

Multi-purpose interactive application that can be used to generate top and bottom shells of a brain region.
To produce shells, we provide an input NRRD file (uint16, RAW encoding) containing the brain region of interest.
On launch, a GL viewer appears showing border voxels of the brain region rendered as cubes. These cubes can be
assigned to the top or bottom shells by "painting" them using the mouse controls:

+ `Left-Click + drag` to rotate camera
+ `Middle-Click + drag` to displace camera
+ `Right-Click + drag` or `Mouse-Wheel` to zoom camera
+ `Ctrl + Left-Click` to paint cubes with bottom label (yellow, value 3)
+ `Shift + Left-Click` to paint cubes with top label (blue, value 4)
+ `Ctrl + Shift + Left-Click` to remove label from painted cubes
+ `s` to save result as `shell.nrrd`
+ `+` to increase brush size
+ `-` to decrease brush size
+ `Esc` to quit (without saving)

In addition to the top/bottom shells, the output file has values: 0 for background voxels, 1 for interior voxels and 2 for non-painted boundary voxels (sides).

This application can also be used to compute relative distances (to the top shell) and perform layer segmentation
based on user-defined layer thicknesses.

```
Usage: ./build/apps/layer_segmenter input [options]
Options:
  -h [ --help ]                         Produce help message.
  -v [ --version ]                      Show program name/version banner and 
                                        exit.
  -s [ --shell ] arg                    Load a saved painted shell.
  -f [ --flip ]                         Flip 'top' and 'bottom' voxels in the 
                                        shell dataset.
  -b [ --bottom-up ]                    Enumerate layers from the bottom to the
                                        top, instead of top to bottom.
  -t [ --thickness ] arg                Layer thicknesses (absolute or 
                                        relative). Must contain at least two 
                                        values.
  -a [ --average-size ] lines           Size of k-nearest neighbour query of 
                                        top to bottom lines used to approximate
                                        relative voxel positions.
  -x [ --crop-x ] <min>[:<max>]         Optional crop range for x axis.
  -y [ --crop-y ] <min>[:<max>]         Optional crop range for y axis.
  -z [ --crop-z ] <min>[:<max>]         Optional crop range for z axis.
  --segment                             Proceed directly segmentation, shell 
                                        volume must be provided.
  -r [ --output-relative-distances ] arg
                                        File path used to save of the relative 
                                        distances created by segmentation. 
                                        Defaults to saving file 
                                        "relativeDistances.nrrd" in the current
                                        directory.
  -l [ --output-layers ] arg            File path used to save the layers 
                                        created by segmentation. Defaults to 
                                        saving file "layer.nrrd" in the current
                                        directory.
```

## direction\_vectors

Generate and save brain region direction vectors.

Unit direction vectors are obtained as the directions of straight line segments joining the bottom shell to the top shell.
These segments join every voxel of the bottom shell to their closest voxels in the top shell, and vice versa.
The direction vector of a voxel is the average of the unit direction vectors of its `average-size` closest segments.

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

Alternatively, the software can be built locally by installing the following dependencies (listed here for Debian-based distros, equivalent ones for other distros):

    build-essential              : C++ compiler and runtime
    ninja-build                  : Ninja build system
    cmake                        : CMake build system
    libboost-filesystem-dev      : Boost.Filesystem library
    libboost-program-options-dev : Boost.Program_options library
    libopenscenegraph-dev        : OpenSceneGraph library

and running (from the source code root):

    $ mkdir build
    $ cd build
    $ cmake .. -GNinja

## Acknowledgements

The development of this software was supported by funding to the Blue Brain Project, a research center of the École polytechnique fédérale de Lausanne (EPFL), from the Swiss government’s ETH Board of the Swiss Federal Institutes of Technology.

## License

For license and authors, see LICENSE.txt.

Copyright © 2017-2024 Blue Brain Project/EPFL
