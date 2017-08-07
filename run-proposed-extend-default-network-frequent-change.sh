#!/bin/bash

/home/min3/ns-allinone-3.26/ns-3.26/waf --run PROPOSED_EXTEND 2>&1 | tee result_proposed_extend_default_network_frequent_change.txt
awk -f parse-dash-proposed-extend-default-network-frequent-change.awk result_proposed_extend_default_network_frequent_change.txt
#gnuplot dash-ploter
