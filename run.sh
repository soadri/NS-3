#!/bin/bash

/home/min3/ns-allinone-3.26/ns-3.26/waf --run kiiseCom 2>&1 | tee result.txt
awk -f parse-dash.awk result.txt
#gnuplot dash-ploter
