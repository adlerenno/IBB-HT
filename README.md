# IBB-HT - IBB for alphabets other than DNA

## Building

To build the test and benchmark, run

	make


## Tests

To run the tests, execute

	make test

or manually execute the compiled `test_bwt` binary.


## Benchmark

The benchmark runs the compression multiple times with different configurations
and records the time each run took.
After a configuration test completes, it's mean, maximum and minimum time are
printed.
The parameters of the benchmark are specified at the top of `benchmark.c`:

```c
/* Benchmark parameters */
#define RUN_COUNT 10
static struct {
  int alphabet_size;
  int max_depth;
} params[] = {
    {2, 10}, {4, 10}, {8, 9}, {16, 6}, {32, 5}, {64, 4}, {128, 3}, {255, 3},
};
```

The benchmark expects to be run with some input files to compress specified as
command line arguments:

	Usage: ./benchmark FILE...


## Third-Party Dependencies

This project depends on the following third-party dependencies,
contained in the `deps` directory:

- [greatest](https://github.com/silentbicycle/greatest), a small testing library
- [A port of Timsort to C](https://github.com/patperry/timsort) as an implementation of a stable sorting algorithm
- [`stb_ds.h`](https://nothings.org/stb_ds/) ([GitHub Repository](https://github.com/nothings/stb/blob/master/stb_ds.h)) for dynamic arrays
