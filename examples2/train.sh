#!/bin/bash
cd "$(dirname "$0")"

../build/NNApproximator --input AND2_data.csv --numberIn 2 --numberOut 4 --epochs 1500 --outWeights AND2_data.weights --printBehaviour
