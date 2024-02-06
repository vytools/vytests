// This is the test program your coded will be run against. If you modify it
// locally just remember your changes won't be included when the grading test is run.
#include <vector>
#include <cmath>
#include <iostream>
#include <fstream>
#include <sstream>
#include <random>
#include <iomanip>

#include "nlohmann/json.hpp"

namespace vy_tests {

namespace {
  nlohmann::json scatter_(std::string label, std::string mode, nlohmann::json &x, nlohmann::json &y) {
    return {{"type", "scatter"}, {"mode",mode}, {"name", label}, {"x",x}, {"y",y}};
  }

  std::pair<nlohmann::json,nlohmann::json> separate_(std::vector<std::pair<double,double>> data) {
    auto x = nlohmann::json::array();
    auto y = nlohmann::json::array();
    for (auto &d : data) {
      x.push_back(d.first);
      y.push_back(d.second);
    }
    return {x,y};
  }
}

std::default_random_engine generator{static_cast<long unsigned int>(1)};
std::uniform_real_distribution<double> distribution(0.0,1.0);

double random_uniform(double lo, double hi) {
    return lo + distribution(generator)*(hi-lo);
}

// Utility function for reading a json file
nlohmann::json parse_json_file(char *input_file_path) {
    std::ifstream jsonfilein;
    jsonfilein.open(input_file_path);
    std::string jsonstr(static_cast<std::stringstream const&>(std::stringstream() << jsonfilein.rdbuf()).str());
    return nlohmann::json::parse(jsonstr);
}

void write_json_file(char *output_file_path, uint8_t width, nlohmann::json &data) {
  std::ofstream file(output_file_path);
  if (width > 0) {
    file << std::setw(1) << data;
  } else {
    file << data;
  }
  file.close();
}

bool grade_check(const nlohmann::json &actual, nlohmann::json &expctd, double tol) {
  bool success = false;
  // std::cout << actual.dump() << " " << expctd.dump() << std::endl;
  try {
    if (actual.is_array() && expctd.is_array() && actual.size() == expctd.size()) {
      success = true;
      for (uint32_t ii = 0; ii < actual.size(); ii++) {
        success &= grade_check(actual[ii], expctd[ii], tol);
      }
    } else if (actual.is_object() && expctd.is_object()) {
      success = true;
      for (auto it = actual.begin(); it != actual.end(); ++it) {
        success &= expctd.find(it.key()) != expctd.end() && 
                    grade_check(it.value(), expctd[it.key()], tol);
      }
    } else if (actual.is_null() && expctd.is_null()) {
      success = true;
    } else if (actual.is_string() && expctd.is_string()) {
      success = actual == expctd;
    } else if (actual.is_boolean() && expctd.is_boolean()) {
      success = actual == expctd;
    } else if (actual.is_number() && expctd.is_number()) {
      double act = std::stod(actual.dump());
      double exp = std::stod(expctd.dump());
      success = (tol > 0) ? std::fabs(act-exp) <= tol : actual == expctd;
    } else if (actual.is_number() && expctd.is_boolean()) { // matlab does 1 and 0 instead of true/false
      success = ((expctd) ? 1 : 0) == std::stoi(actual.dump());
    } else if (expctd.is_number() && actual.is_boolean()) { // matlab does 1 and 0 instead of true/false
      success = ((actual) ? 1 : 0) == std::stoi(expctd.dump());
    }
  } catch (std::exception &exc) {
    success = false;
  }
  return success;
}

// Utility function for checking an output value and scoring it
void grade_problem(nlohmann::json &outputs, std::string output, nlohmann::json actual) {
  // {"actual":, "expected":, "tolerance":, "points_possible":, "points_earned":}
  bool success = false;
  if (outputs.find(output) != outputs.end()) {
    outputs[output]["actual"] = actual;
    if (outputs[output].find("points_possible") == outputs[output].end()) {
      outputs[output]["points_possible"] = 0.0;
    }
    double tolerance = outputs[output].value("tolerance",0.0);
    if (outputs[output].find("expected") != outputs[output].end()) {
      success = grade_check(actual, {outputs[output]["expected"]}, tolerance);
    }
    outputs[output]["points_earned"] = (success) ? double(outputs[output]["points_possible"]) : 0.0;
  }
}

nlohmann::json new_scatter_plot(
  std::string title,
  std::string xlabel,
  std::string ylabel,
  int32_t padding_left,
  int32_t padding_right)
{
  nlohmann::json plot = {
    {"data", nlohmann::json::array()},
    {"layout",{
      {"autosize",true},
      {"title",title},
      {"xlabel",{{"title",xlabel}}},
      {"xlabel",{{"title",ylabel}}},
      {"margin",{{"l",padding_left},{"r",padding_right}}}
    }},
    {"config", {{"responsive",true}}},
  };
  return plot;
}

std::vector<std::pair<double,double>> sample(
  double xlo,
  double xhi,
  uint32_t n,
  double f(double))
{
  uint32_t niters = std::fmax(2,n);
  std::vector<std::pair<double,double>> data;
  for (uint32_t ii = 0; ii <= niters; ii++) {
    double x = xlo + static_cast<double>(ii)/static_cast<double>(niters-1)*(xhi-xlo);
    data.push_back({x,f(x)});
  }
  return data;
}

nlohmann::json scatter_dataseries(
  std::string label,
  std::string color,
  uint32_t line_width,
  uint32_t marker_size,
  std::vector<std::pair<double, double>> xy)
{
  auto xy_ = separate_(xy);
  std::string mode = (line_width == 0) ? "markers" : (marker_size == 0) ? "lines" : "lines+markers";
  auto x = scatter_(label, mode, xy_.first, xy_.second);
  if (marker_size != 0) {
    nlohmann::json marker = {{"size", marker_size}};
    if (color != "") marker["color"] = color;
    x["marker"] = marker;
  }
  if (line_width != 0) {
    nlohmann::json line = {{"width", line_width}};
    if (color != "") line["color"] = color;
    x["line"] = line;
  }
  return x;
}

std::string hsl(uint8_t h, uint8_t s, uint8_t l) {
  return "hsl(" + std::to_string(h) + "," + std::to_string(s) + "\%," + std::to_string(l) + "\%)";
}

}
