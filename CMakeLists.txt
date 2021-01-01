# For now, this file governs testing only, not building.
# As a result, we initialize the project with no language support (C and CXX
# support would otherwise be enabled by default).

project(tenyr NONE)

cmake_minimum_required(VERSION 3.12)

# Define the versions of CMake that this file is written for.
cmake_policy(VERSION 3.12...3.19)

enable_testing()

# TODO add_test()

