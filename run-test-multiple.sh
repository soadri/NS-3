#!/bin/bash

/home/min3/ns-allinone-3.26/ns-3.26/waf --run Test_Multiple 2>&1 | tee result_multiple.txt
awk -f parse-test-multiple-client.awk result_multiple.txt
#gnuplot dash-ploter
