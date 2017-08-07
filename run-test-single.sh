#!/bin/bash

/home/min3/ns-allinone-3.26/ns-3.26/waf --run Test_Single 2>&1 | tee result_single.txt
awk -f parse-test-single-client.awk result_single.txt
#gnuplot dash-ploter
