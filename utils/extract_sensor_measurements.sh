#!/bin/bash
mc_bin_utils extract --in /tmp/mc-control-PandaProsthesis-latest.bin --out /tmp/measurements --keys BoneTag_Sensors
mc_bin_to_log /tmp/measurements.bin /tmp/measurements.csv
