#include "Utilities/datasplitter.h"

#include <random>

namespace Utilities {

std::pair<DataVector, DataVector> DataSplitter::splitDataRandomly(DataVector const& inputData, double trainingPercentage)
{
  if (trainingPercentage == 0) {
    return std::make_pair(DataVector(), inputData);
  }
  DataVector trainingData{};
  DataVector validationData{};

  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<double> dis(0.0, 100.0);

  for (auto const& entry : inputData) {
    if (dis(gen) <= trainingPercentage) {
      trainingData.push_back(entry);
    } else {
      validationData.push_back(entry);
    }
  }

  return std::make_pair(trainingData, validationData);
}

std::pair<DataVector, DataVector> DataSplitter::splitDataWithThreshold(DataVector const& data, uint32_t thresholdVariable, TensorDataType threshold)
{
  DataVector belowAndEqualThreshold{};
  DataVector aboveThreshold{};

  for (auto const& row : data) {
    if (row.first[thresholdVariable].item<TensorDataType>() <= threshold) {
      belowAndEqualThreshold.push_back(row);
    } else {
      aboveThreshold.push_back(row);
    }
  }

  return std::make_pair(belowAndEqualThreshold, aboveThreshold);
}

}
