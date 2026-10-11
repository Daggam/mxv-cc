CORES ?= 4
N ?= 20
M ?= 20
RAND_N ?= 10
M_DENSITY ?= 0.3

MPICC_FLAGS = -DN=$(N) -DM=$(M) -DRAND_N=$(RAND_N) -DM_DENSITY=$(M_DENSITY)

mxv_parallel_deps = utils.c parallel_utils.c

parallel pll p: 	
			mpicc  $(MPICC_FLAGS) mxv_parallel.c $(mxv_parallel_deps) -o mxv_parallel.out && \
			mpirun --allow-run-as-root -np $(CORES) ./mxv_parallel.out

sequential seq s:
			mpicc $(MPICC_FLAGS) mxv_sequential.c utils.c -o mxv_sequential.out && \
			mpirun --allow-run-as-root -np 1 ./mxv_sequential.out
