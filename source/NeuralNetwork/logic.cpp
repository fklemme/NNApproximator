#include "NeuralNetwork/logic.h"
#include "Utilities/datanormalizator.h"
#include "Utilities/fileparser.h"

#include <chrono>

namespace NeuralNetwork {

bool Logic::performUserRequest(const Utilities::ProgramOptions& user_options)
{
  options = user_options;
  auto data = Utilities::FileParser::ParseInputFile(options.InputDataFilePath, options.NumberOfInputVariables, options.NumberOfOutputVariables);
  if (!data) {
    return false;
  }

  auto minMax = std::make_pair(MinMaxVector(), MinMaxVector());
  MinMaxVector& inputMinMax = minMax.first;
  MinMaxVector& outputMinMax = minMax.second;

  Utilities::DataNormalizator::Normalize(*data, minMax, 0.0, 1.0);  // TODO let user control normalization

  Network network{options.NumberOfInputVariables, options.NumberOfOutputVariables, std::vector<uint32_t>{4000}}; // TODO fix hardcoded value

  if (options.InputNetworkParameters != Utilities::DefaultValues::INPUT_NETWORK_PARAMETERS) {
    torch::load(network, options.InputNetworkParameters);
  }

  trainNetwork(network, *data);

  network->eval();

  // Output behaviour of network: // TODO user parametrization
  for (auto const& [inputTensor, outputTensor] : *data) {
    auto prediction = network->forward(inputTensor);
    auto loss = torch::mse_loss(prediction, outputTensor);

    auto dInputTensor = inputTensor;
    auto dOutputTensor = outputTensor;

    Utilities::DataNormalizator::Denormalize(dInputTensor, inputMinMax,0.0, 1.0);
    Utilities::DataNormalizator::Denormalize(dOutputTensor, outputMinMax, 0.0, 1.0);
    Utilities::DataNormalizator::Denormalize(prediction, outputMinMax, 0.0 , 1.0);

    std::cout << "\nx: ";
    for (uint32_t i = 0; i < options.NumberOfInputVariables; ++i) std::cout << inputTensor[i].item<TensorDataType>() << " ";
    std::cout << "\ny: ";
    for (uint32_t i = 0; i < options.NumberOfOutputVariables; ++i) std::cout << outputTensor[i].item<TensorDataType>() << " ";
    std::cout << "\nprediction: ";
    for (uint32_t i = 0; i < options.NumberOfOutputVariables; ++i) std::cout << prediction[i].item<TensorDataType>() << " ";
    std::cout << "\nloss: " << loss.item<double>() << std::endl;
  }

  if (options.OutputNetworkParameters != Utilities::DefaultValues::OUTPUT_NETWORK_PARAMETERS) {
    torch::save(network, options.OutputNetworkParameters);
  }

  return true;
}

void Logic::trainNetwork(Network& network, const DataVector& data)
{
  const auto& numberOfEpochs = options.NumberOfEpochs;
  torch::optim::SGD optimizer(network->parameters(), 0.000001); // TODO fix hardcoded value

  auto lastMeanError = calculateMeanError(network, data);
  auto currentMeanError = lastMeanError;
  auto start = std::chrono::steady_clock::now();

  for (size_t epoch = 1; epoch <= numberOfEpochs; ++epoch) {
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start);
    auto remaining = ((elapsed / std::max(epoch - 1, 1ul)) * (numberOfEpochs - epoch + 1));
    lastMeanError = currentMeanError;
    currentMeanError = calculateMeanError(network, data);

    if (epoch == numberOfEpochs && lastMeanError - currentMeanError > 0.00001) { // TODO fix hardcoded value
      std::cout << "\rContinue training, because mean error changed from " << lastMeanError << " to " << currentMeanError;
      std::flush(std::cout);
      --epoch;
    } else {
      std::cout << "\rEpoch " << epoch << " of " << numberOfEpochs << ". Current mean error: " << currentMeanError <<
                " -- Remaining time: " << formatDuration<std::chrono::milliseconds, std::chrono::hours, std::chrono::minutes, std::chrono::seconds>(remaining); // TODO better output + only if wanted
      std::flush(std::cout);
    }

    for (auto const& [x, y] : data) {
      auto prediction = network->forward(x);

//      prediction = prediction.toType(torch::ScalarType::Long);
//      auto target = y.toType(torch::ScalarType::Long);
      auto loss = torch::mse_loss(prediction, y);
//      auto loss = torch::kl_div(prediction, y);
//      auto loss = torch::nll_loss(prediction, y);

      optimizer.zero_grad();

      loss.backward();
      optimizer.step();
    }
  }
  std::cout << "\nTraining duration: " << formatDuration<std::chrono::milliseconds, std::chrono::hours, std::chrono::minutes, std::chrono::seconds>
    (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start));
}

double Logic::calculateMeanError(Network& network, const DataVector &testData)
{
  double error = 0;
  for (auto const& [x, y] : testData) {
    auto prediction = network->forward(x);
    auto loss = torch::mse_loss(prediction, y);
    error += loss.item<double>();
  }
  return error / testData.size();
}

}
