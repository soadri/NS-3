#!/bin/bash

/home/min3/ns-allinone-3.26/ns-3.26/waf --run PANDA 2>&1 | tee result_panda.txt
awk -f parse-dash-panda.awk result_panda.txt
#gnuplot dash-ploter
