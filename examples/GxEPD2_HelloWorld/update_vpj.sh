#!/bin/sh

VPJ_FILE=../../vs/GxEPD2_HelloWorld.vpj

dot_d_2vs.sh ${VPJ_FILE} .pio/build/seeed
cat ${VPJ_FILE} | sed -e 's!\\ ! !g' > e.vpj
mv e.vpj ${VPJ_FILE}

