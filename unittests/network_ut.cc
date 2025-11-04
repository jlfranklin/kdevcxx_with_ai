// SPDX-FileCopyrightText: 2024 Artur Bać
// SPDX-License-Identifier: MIT

#include <network.h>
#include <boost/ut.hpp>
#include <string_view>

#include <simple_enum/simple_enum.hpp>
#include <aiprocess/trim_white_space.h>
#include <aiprocess/log.h>
#include <aiprocess/app_settings.h>

#include <cstdlib>
#include <cstdio>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

// Global variable to store the PID of the test server
static pid_t g_test_server_pid = -1;

// Function to start the test server and return its PID
pid_t start_test_server()
{
    pid_t pid = fork();
    if (pid == 0) {
        // Child process - start the test server
        execl("/usr/bin/python3", "python3", "/app/unittests/test_server.py", "8080", (char *)NULL);
        // If execl returns, it means it failed
        _exit(1);
    } else if (pid > 0) {
        // Parent process - return the PID
        sleep(1);
        return pid;
    } else {
        // Fork failed
        return -1;
    }
}

// Function to kill the test server
void kill_test_server()
{
    if (g_test_server_pid > 0) {
        kill(g_test_server_pid, SIGTERM);
        int status;
        waitpid(g_test_server_pid, &status, 0);
    }
}

int main()
  {
  using namespace boost::ut;
  using namespace std::string_view_literals;

  // Start the test server
  g_test_server_pid = start_test_server();
  if (g_test_server_pid == -1) {
      // Failed to start server, exit with error
      return 1;
  }

  aiprocess::setup_loggers(aiprocess::app_settings_t{.log_path = "network_ut"});

  "send_text echo test"_test = []
  {
    std::string_view host = "localhost";
    std::string_view port = "8080";
    bool use_ssl = false;
    std::string_view target = "/";
    std::string_view lapi_key = "INVALID KEY";
    std::string_view text = R"({"test": "data"})";
    int version = 11;  // HTTP/1.1

    auto result = send_text_to_gpt(host, port, use_ssl, target, lapi_key, text, version);

    // Since `expected` is hypothetical here, let's assume `result` is valid if no error is present.
    expect(fatal(result.has_value())) << "Expected a valid result";

    // Assuming the echoed back text contains the original text
    expect(result.value().find("\"test\": \"data\"") != std::string::npos)
      << result.value() << "\nThe response should contain the sent text\n";
  };

  "send_text_to_postman echo"_test = []
  {
    // Define the JSON data using Glaze (pseudo-code, adjust according to Glaze's actual API)
    static constexpr std::string_view json_text = R"({
      "model": "gpt-3.5-turbo-instruct",
      "prompt":
       "[Use C++20, prefer std::ranges and std::views over for loops, use nodiscard, use trailing return, use lower_case convention always,simplify code for c++20 use trailing return, apply lower_case naming convention]\n  ASSERT(value.isMember(szName));\n  if (!value.isMember(szName))\n    return {};\n\n  Json::Value objectTypeValue = value.get(szName, Json::Value::null);\n  if (objectTypeValue == Json::Value::null)\n objectTypeValue = \"\";\n  if (!objectTypeValue.isString())\n    return {};\n\n  return objectTypeValue.asString();",
      "temperature": 0.5,
      "max_tokens": 500,
      "top_p": 1,
      "frequency_penalty": 0,
      "presence_penalty": 0
  })";

    std::string_view host = "localhost";
    std::string_view port = "8080";
    bool use_ssl = false;
    std::string_view target = "/";
    std::string_view lapi_key = "INVALID KEY";
    int version = 11;  // HTTP/1.1

    auto result = send_text_to_gpt(host, port, use_ssl, target, lapi_key, json_text, version);

    // Verify the response contains the sent JSON data
    expect(fatal(result.has_value())) << "Expected a valid result";
    expect(result->find("ASSERT(value.isMember(szName)") != std::string::npos)
      << result.value() << "\nThe response should contain the sent JSON data\n";
  };
#if 0
  "send_text_to_openai"_test = []
  {
    // Define the JSON data using Glaze (pseudo-code, adjust according to Glaze's actual API)
    static constexpr std::string_view json_text = R"(
{
  "model":"gpt-3.5-turbo-instruct",
  "prompt":"[Use C++20, prefer std::ranges and std::views over for loops, use nodiscard, use trailing return, use lower_case convention always,simplify code for c++20 use trailing return, apply lower_case naming convention] \n  ASSERT(value.isMember(szName));\n  if (!value.isMember(szName))\n    return {};\n\n  Json::Value objectTypeValue = value.get(szName, Json::Value::null);\n  if (objectTypeValue == Json::Value::null)\n    objectTypeValue = \"\";\n  if (!objectTypeValue.isString())\n    return {};\n\n  return objectTypeValue.asString();",
  "temperature":0.5,
  "max_tokens":500,
  "top_p":1,
  "frequency_penalty":0,
  "presence_penalty":0
})";

    std::string_view host = "api.openai.com";
    std::string_view port = "443";
    std::string_view target = "/v1/completions";
    int version = 11;  // HTTP/1.1

    auto result = send_text_to_gpt(
      host,
      port,
      target,
      api_key,
      aiprocess::trim_white_space(json_text),
      version
    );

    // Verify the response contains the sent JSON data
    expect(fatal(result.has_value())) << [&result]()
    { return stralgo::stl::merge("Expected a valid result ["sv, simple_enum::enum_name(result.error()), ']'); };
    expect(result->find("objectTypeValue") != std::string::npos)
      << result.value() << "\nThe response should contain the sent JSON data\n";
  };
#endif

  // Ensure server is killed when program exits
  atexit(kill_test_server);

  return 0;
  }
