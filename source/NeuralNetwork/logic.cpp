#include "NeuralNetwork/logic.h"
#include "Utilities/datanormalizator.h"
#include "Utilities/fileparser.h"

#include <chrono>
#include <random>

namespace NeuralNetwork {

bool Logic::performUserRequest(const Utilities::ProgramOptions& user_options)
{
  options = user_options;
  auto data = Utilities::FileParser::ParseInputFile(options.InputDataFilePath, options.NumberOfInputVariables, options.NumberOfOutputVariables);
  if (!data) {
    return false;
  }

  Utilities::DataNormalizator::Normalize(*data, minMax, 0.0, 1.0);  // TODO let user control normalization

  network = Network{options.NumberOfInputVariables, options.NumberOfOutputVariables, std::vector<uint32_t>{500}}; // TODO fix hardcoded value

  if (options.InputNetworkParameters != Utilities::DefaultValues::INPUT_NETWORK_PARAMETERS) {
    torch::load(network, options.InputNetworkParameters);
  }

  trainNetwork(*data);

  network->eval();

  // Output behaviour of network: // TODO user parametrization
  for (auto const& [inputTensor, outputTensor] : *data) {
    auto prediction = network->forward(inputTensor);
    auto loss = torch::mse_loss(prediction, outputTensor);

    torch::Tensor dInputTensor = inputTensor.clone();
    torch::Tensor dOutputTensor = outputTensor.clone();
    torch::Tensor dPrediction = prediction.clone();

    Utilities::DataNormalizator::Denormalize(dInputTensor, inputMinMax,0.0, 1.0, true);
    Utilities::DataNormalizator::Denormalize(dOutputTensor, outputMinMax, 0.0, 1.0, true);
    Utilities::DataNormalizator::Denormalize(dPrediction, outputMinMax, 0.0 , 1.0, true);

    std::cout << "\nx: ";
    for (uint32_t i = 0; i < options.NumberOfInputVariables; ++i) std::cout << inputTensor[i].item<TensorDataType>() << " (" << dInputTensor[i].item<TensorDataType>() << ") ";
    std::cout << "\ny: ";
    for (uint32_t i = 0; i < options.NumberOfOutputVariables; ++i) std::cout << outputTensor[i].item<TensorDataType>() << " (" << dOutputTensor[i].item<TensorDataType>() << ") ";
    std::cout << "\nprediction: ";
    for (uint32_t i = 0; i < options.NumberOfOutputVariables; ++i) std::cout << prediction[i].item<TensorDataType>() << " (" << dPrediction[i].item<TensorDataType>() << ") ";
    std::cout << "\nloss: " << loss.item<double>() << std::endl;
  }

  if (options.OutputNetworkParameters != Utilities::DefaultValues::OUTPUT_NETWORK_PARAMETERS) {
    torch::save(network, options.OutputNetworkParameters);
  }

  if (options.InteractiveMode) {
    performInteractiveMode();
  }

  return true;
}

void Logic::trainNetwork(const DataVector& data)
{
  DataVector randomlyShuffledData(data);
  const auto& numberOfEpochs = options.NumberOfEpochs;
  torch::optim::SGD optimizer(network->parameters(), 0.000001); // TODO fix hardcoded value

  std::random_device rd;
  std::mt19937 g(rd());

  auto lastMeanError = calculateMeanError(data);
  auto currentMeanError = lastMeanError;
  auto start = std::chrono::steady_clock::now();

  bool continueTraining = true;
  int32_t numberOfDeteriorationsInRow = 0;

  auto maxExecutionTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::hours(24));  // TODO remove or use parameter

  for (uint32_t epoch = 1; epoch <= numberOfEpochs || continueTraining; ++epoch) {
//    std::shuffle(randomlyShuffledData.begin(), randomlyShuffledData.end(), g);
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start);
    auto remaining = ((elapsed / std::max(epoch - 1, 1u)) * (numberOfEpochs - epoch + 1));
    lastMeanError = currentMeanError;
    currentMeanError = calculateMeanError(data);

    if (lastMeanError - currentMeanError < options.Epsilon) {
      ++numberOfDeteriorationsInRow;
      if (numberOfDeteriorationsInRow > 3) {
        continueTraining = false;
      }
    } else {
      numberOfDeteriorationsInRow = 0;
    }

    if (options.ShowProgressDuringTraining) {
      if (epoch > numberOfEpochs) { // TODO fix hardcoded value
        std::cout << "\rContinue training. Mean squared error changed from " << lastMeanError << " to " << currentMeanError << " -- epoch: " << epoch;
        std::flush(std::cout);
      } else {
        std::cout << "\rEpoch " << epoch << " of " << numberOfEpochs << ". Current mean squared error: " << currentMeanError << " previous: "
                  << lastMeanError <<
                  " -- Remaining time: " << formatDuration<std::chrono::milliseconds, std::chrono::hours, std::chrono::minutes, std::chrono::seconds>(
          remaining); // TODO better output
        std::flush(std::cout);
      }
    }

    if (elapsed > maxExecutionTime) {
      std::cout << "\nStop execution (timeout)." << std::endl;
      break;
    }

    for (auto const& [x, y] : randomlyShuffledData) {
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
  // TODO show result only if wanted
  std::cout << "\nTraining duration: " << formatDuration<std::chrono::milliseconds, std::chrono::hours, std::chrono::minutes, std::chrono::seconds>
    (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start));
}

void Logic::performInteractiveMode()
{
  std::cout << "Interactive mode activated. Quit with 'q'" << std::endl;
  std::string input{};
  auto inTensor = torch::zeros(options.NumberOfInputVariables, TORCH_DATA_TYPE);
  uint32_t currentVariable = 0;

  while (input != "q") {
    std::cout << "Input variable " << currentVariable << ": ";
    std::flush(std::cout);
    std::cin >> input;

    try {
      TensorDataType value = std::stod(input);
      inTensor[currentVariable++] = value;
    } catch (std::exception const&) {
      if (input != "q") {
        std::cout << "'" << input << "' cannot be cast to double. Quit interactive mode with 'q'." << std::endl;
      }
    }

    if (currentVariable >= options.NumberOfInputVariables) {
      Utilities::DataNormalizator::Normalize(inTensor, inputMinMax, 0.0, 1.0);
      auto output = network->forward(inTensor);
      auto dOutputTensor = output.clone();
      Utilities::DataNormalizator::Denormalize(dOutputTensor, outputMinMax, 0.0, 1.0, true);

      std::cout << "Normalized input: ";
      for (uint32_t i = 0; i < options.NumberOfInputVariables; ++i) {
        std::cout << inTensor[i].item<TensorDataType>() << "  ";
      }

      std::cout << "\nNeural network output: ";
      for (uint32_t i = 0; i < options.NumberOfOutputVariables; ++i) {
        std::cout << dOutputTensor[i].item<TensorDataType>() << " (" << output[i].item<TensorDataType>() << ")  ";
      }
      std::cout << std::endl;
      currentVariable = 0;
    }
  }
}

double Logic::calculateMeanError(const DataVector &testData)
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
