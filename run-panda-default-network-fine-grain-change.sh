#!/bin/bash

/home/min3/ns-allinone-3.26/ns-3.26/waf --run PANDA_DEFAULT 2>&1 | tee result_panda_default_network_fine_grain_change.txt
awk -f parse-dash-panda-default-network-fine-grain-change.awk result_panda_default_network_fine_grain_change.txt
#gnuplot dash-ploter
