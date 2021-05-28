![CMake result](https://github.com/bergfi/dmc/actions/workflows/master.yml/badge.svg)

# LLMC Model Checker

LLMC [1] is a multi-core model checker of LLVM IR. It translates the input LLVM IR into a model LLVM IR that implements the DMC API, the API of the model checker DMC. This allows LLMC to execute the model's next-state function, instead of interpreting the input LLVM IR, enabling speedups of orders of magnitude.

DMC Model Checker [2] is a modular multi-threaded model checker with a model-agnostic model interface, the DMC API. It uses a compression tree, [dtree](https://github.com/bergfi/dtree), for storing states.

# Dependencies

LLMC depends on:
- [dtree](https://github.com/bergfi/dtree), a concurrent compression tree for variable-length vectors
- [DMC](https://github.com/bergfi/dmc), a modular multi-core model checker [2]
- [libfrugi](https://github.com/bergfi/libfrugi), a general-purpose library
- [LLVM](https://github.com/llvm/llvm-project.git), a collection of modular and reusable compiler and toolchain technologies [3]

Dependencies are downloaded during configuration unless specified otherwise.

# Build

```
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DUSE_SYSTEM_LLVM=1
make -j 4
```

CMake options:
- `-DFETCHCONTENT_SOURCE_DIR_DMC=/path/to/dmc`: use DMC source at the specified location
- `-DFETCHCONTENT_SOURCE_DIR_DTREE=/path/to/dtree`: use DTREE source at the specified location
- `-DFETCHCONTENT_SOURCE_DIR_LIBFRUGI=/path/to/libfrugi`: use libfrugi source at the specified location
- `-DUSE_SYSTEM_LLVM=N`: N=1 means to use the LLVM libraries of the system; N=0 means LLVM will be downloaded and compiled.

# Usage

The primitive command line usage is as follows:
- `-m SEARCHCORE`, where `SEARCHCORE` can be any of `singlecore_simple` or `multicore_bitbetter`
- `-s STATESTORAGE`, where `STATESTORAGE` can be any of `dtree`, `treedbsmod`, `treedbs_cchm`, `cchm` or `stdmap`
- `--threads N`, where `N` is the number of model-checking threads to use
- the positional argument is a filename of an LLVM IR file.

The tests in `/tests/correctness` contains numerous tests in the form of LLVM IR files. 

Likewise, `/tests/performance` contains a number of performance tests, which were used to generate the performance numbers in the first publication [1].

References:
- [1] van der Berg, F. I. (2021) LLMC: Verifying High-performance Software. TBD.
- [2] van der Berg, F. I. (2021) Recursive Variable-Length State Compression for Multi-Core Software Model Checking. 2021 NASA Formal Methods. Preprint.
- [3] https://llvm.org/

# License

LLMC - LLVM IR Model Checker
Copyright © 2013-2021 Freark van der Berg

LLMC is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, version 3 of the License.

LLMC is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with LLMC.  If not, see <https://www.gnu.org/licenses/>.
