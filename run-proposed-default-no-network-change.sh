#!/bin/bash

/home/min3/ns-allinone-3.26/ns-3.26/waf --run PROPOSED 2>&1 | tee result_proposed_default_no_network_change.txt
awk -f parse-dash-proposed-default-no-network-change.awk result_proposed_default_no_network_change.txt
#gnuplot dash-ploter
