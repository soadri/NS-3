#!/bin/bash

/home/min3/ns-allinone-3.26/ns-3.26/waf --run Test 2>&1 | tee result_test.txt
awk -f parse-test.awk result_test.txt
#gnuplot dash-ploter
