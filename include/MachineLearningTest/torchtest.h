#pragma once

#include "neuralnetwork.h"

namespace TorchTest {

using TensorDataType = float;

class TorchTest
{
public:
  static TensorDataType TargetFunction(TensorDataType x);

public:
  TorchTest();

public:
  void run();

private:
  void initialize();

private:
  Network network;
  std::vector<std::pair<torch::Tensor, torch::Tensor>> testData;
};

}
