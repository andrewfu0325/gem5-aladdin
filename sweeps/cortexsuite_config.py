#!/usr/bin/env python
# CortexSuite benchmark definitions

from design_sweep_types import *

disparity = Benchmark("disparity", "disparity/src/c/script_disparity")
disparity.set_kernels(["computeSAD", "integralImage2D2D",
                       "finalSAD", "findDisparity"])
disparity.add_loop("computeSAD", 16, UNROLL_ONE)
disparity.add_loop("computeSAD", 18)
disparity.add_loop("integralImage2D2D", 16)
disparity.add_loop("integralImage2D2D", 19, UNROLL_ONE)
disparity.add_loop("integralImage2D2D", 20)
disparity.add_loop("integralImage2D2D", 25, UNROLL_ONE)
disparity.add_loop("integralImage2D2D", 26)
disparity.add_loop("finalSAD", 18, UNROLL_ONE)
disparity.add_loop("finalSAD", 20)
disparity.add_loop("findDisparity", 16, UNROLL_ONE)
disparity.add_loop("findDisparity", 18)
disparity.add_array("Ileft", 12742, 4, PARTITION_CYCLIC)
disparity.add_array("Iright_moved", 12742, 4, PARTITION_CYCLIC)
disparity.add_array("SAD", 12742, 4, PARTITION_CYCLIC)
disparity.add_array("SAD", 12742, 4, PARTITION_CYCLIC)
disparity.add_array("integralImg", 12742, 4, PARTITION_CYCLIC)
disparity.add_array("Ileft", 12742, 4, PARTITION_CYCLIC)
disparity.add_array("Iright_moved", 12742, 4, PARTITION_CYCLIC)
disparity.add_array("SAD", 12742, 4, PARTITION_CYCLIC)
disparity.add_array("integralImg", 12742, 4, PARTITION_CYCLIC)
disparity.add_array("retSAD", 12290, 4, PARTITION_CYCLIC)
disparity.add_array("integralImg", 12742, 4, PARTITION_CYCLIC)
disparity.add_array("retSAD", 12290, 4, PARTITION_CYCLIC)
disparity.add_array("retSAD", 12290, 4, PARTITION_CYCLIC)
disparity.add_array("minSAD", 12290, 4, PARTITION_CYCLIC)
disparity.add_array("retDisp", 12290, 4, PARTITION_CYCLIC)
disparity.generate_separate_kernels(separate=True, enforce_order=True)
disparity.use_local_makefile()

localization = Benchmark("localization",
                         "localization/src/c/script_localization")
localization.set_kernels(["weightedSample_worker"])
localization.add_loop("weightedSample_worker", 20, 3)
localization.add_loop("weightedSample_worker", 22, 3)
localization.add_loop("weightedSample_worker", 27, 3)
localization.add_array("seed", 500, 4, PARTITION_CYCLIC)
localization.add_array("bin", 500, 4, PARTITION_CYCLIC)
localization.add_array("w", 500, 4, PARTITION_CYCLIC)
localization.use_local_makefile()

# Input size: sim
sift = Benchmark("sift", "sift/src/c/script_sift")
sift.set_kernels(["imsmooth_worker"])
sift.add_array("temp", 27, 4, PARTITION_CYCLIC)
# The sizes of these arrays change over the iterations, so I've specified the
# largest size observed.
sift.add_array("buffer", 64*64, 4, PARTITION_CYCLIC)
sift.add_array("array", 64*64, 4, PARTITION_CYCLIC)
sift.add_array("out", 64*64, 4, PARTITION_CYCLIC)
sift.add_loop("imsmooth_worker", 62, UNROLL_FLATTEN)
sift.add_loop("imsmooth_worker", 68, UNROLL_FLATTEN)
sift.add_loop("imsmooth_worker", 77, UNROLL_ONE)
sift.add_loop("imsmooth_worker", 79)
sift.add_loop("imsmooth_worker", 101, UNROLL_ONE)
sift.add_loop("imsmooth_worker", 103)
sift.use_local_makefile()

# for sqcif: srtdPnts: 1632, suppressR: 544, supId: 544, tempp: 1632, temps: 544 interestPnts: 1632
# for sim: srtdPnts: 27, suppressR: 9, supId: 9, tempp: 27, temps: 9 interestPnts: 27
stitch = Benchmark("stitch", "stitch/src/c/script_stitch")
stitch.set_kernels(["getANMS_worker"])
stitch.add_array("srtdPnts", 27, 4, PARTITION_CYCLIC)
stitch.add_array("suppressR", 9, 4, PARTITION_CYCLIC)
stitch.add_array("supId", 9, 4, PARTITION_CYCLIC)
stitch.add_array("tempp", 27, 4, PARTITION_CYCLIC)
stitch.add_array("temps", 9, 4, PARTITION_CYCLIC)
stitch.add_array("interestPnts", 27, 4, PARTITION_CYCLIC)
stitch.add_array("validCount", 1, 4, PARTITION_COMPLETE)
stitch.add_array("num_interest_pts", 1, 4, PARTITION_COMPLETE)
stitch.add_loop("getANMS_worker", 121)
stitch.add_loop("getANMS_worker", 142)
stitch.add_loop("getANMS_worker", 157)
stitch.add_loop("getANMS_worker", 180)
stitch.add_loop("getANMS_worker", 193)
stitch.use_local_makefile()

CORTEXSUITE = [disparity, localization, sift, stitch]