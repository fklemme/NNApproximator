#include "Utilities/fileparser.h"

#include <iostream>

namespace Utilities {

std::optional<DataVector> FileParser::ParseInputFile(const std::string& path, uint32_t numberOfInputNodes, uint32_t numberOfOutputNodes)
{
  auto data = DataVector();

  if (path.empty()) {
    std::cout << "Error: \"" << path << "\" is not a valid path to a file for the input data." << std::endl;
    return std::nullopt;
  }

  std::ifstream inputFile(path);

  std::string line;
  if (!std::getline(inputFile, line)) {
    std::cout << "Error: Inputfile is empty or not valid." << std::endl;
    return std::nullopt;
  }

  // TODO: Use Header of input file

  while (std::getline(inputFile, line)) {
    line.erase (std::remove(line.begin(), line.end(), ','), line.end());  // remove ',' from string
    std::istringstream iss(line);
    TensorDataType value;

    // get input:
    auto inTensor = torch::zeros(numberOfInputNodes, TORCH_DATA_TYPE);
    for (uint32_t i = 0; i < numberOfInputNodes; ++i) {
      if (!(iss >> value)) {
        std::cout << "Error: Unable to parse input data." << std::endl;
        return std::nullopt;
      }
      inTensor[i] = value;
    }

    // get output:
    auto outTensor = torch::zeros(numberOfOutputNodes, TORCH_DATA_TYPE);
    for (uint32_t i = 0; i < numberOfOutputNodes; ++i) {
      if (!(iss >> value)) {
        std::cout << "Error: Unable to parse output data." << std::endl;
        return std::nullopt;
      }
      outTensor[i] = value;
    }

    data.emplace_back(std::make_pair(inTensor, outTensor));
  }

  inputFile.close();
  return std::make_optional(data);
}

void FileParser::SaveData(DataVector const &data, std::string const &outputFilePath)
{
  if (data.empty()) {
    return;
  }
  std::ofstream outputFile(outputFilePath);

  for (int64_t x = 1; x <= data[0].first.size(0); ++x) {
    if (x == 1) {
      outputFile << "x" << std::to_string(x);
    } else {
      outputFile << " x" << std::to_string(x);
    }
  }

  for (int64_t y = 1; y <= data[0].second.size(0); ++y) {
    if (y == 1) {
      outputFile << " y" << std::to_string(y);
    } else {
      outputFile << " y" << std::to_string(y);
    }
  }
  outputFile << "\n";

  for (auto const& [inputTensor, outputTensor] : data) {
    outputFile << std::to_string(inputTensor[0].item<TensorDataType>());

    for (int64_t i = 1; i < inputTensor.size(0); ++i) {
      outputFile << " " << std::to_string(inputTensor[i].item<TensorDataType>());
    }

    for (int64_t i = 0; i < outputTensor.size(0); ++i) {
      outputFile << " " << std::to_string(outputTensor[i].item<TensorDataType>());
    }

    outputFile << "\n";
  }

  outputFile.close();
}

}
