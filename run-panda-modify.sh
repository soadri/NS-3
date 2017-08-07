#!/bin/bash

/home/min3/ns-allinone-3.26/ns-3.26/waf --run PANDA_MODIFY 2>&1 | tee result_panda_modify.txt
awk -f parse-dash-panda-modify.awk result_panda_modify.txt
#gnuplot dash-ploter
