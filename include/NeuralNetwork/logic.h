#pragma once

#include "NeuralNetwork/neuralnetwork.h"
#include "Utilities/constants.h"
#include "Utilities/programoptions.h"

namespace NeuralNetwork {

class Logic
{
public:
  [[nodiscard]]
  bool performUserRequest(Utilities::ProgramOptions const& options);

private:
  void trainNetwork(DataVector const& data);
  void performInteractiveMode();
  [[nodiscard]]
  double calculateMeanError(DataVector const& testData);
  [[nodiscard]]
  double calculateR2Score(DataVector const& testData);
  [[nodiscard]]
  double calculateR2ScoreAlternate(DataVector const& testData);
  [[nodiscard]]
  std::pair<DataVector, DataVector> splitData(DataVector const& inputData, double trainingPercentage) const;
  void outputBehaviour(DataVector const& data);

private:
  Network network {nullptr};
  Utilities::ProgramOptions options {};

  std::pair<MinMaxVector, MinMaxVector> minMax = std::make_pair(MinMaxVector(), MinMaxVector());
  MinMaxVector& inputMinMax = minMax.first;
  MinMaxVector& outputMinMax = minMax.second;
};

}
