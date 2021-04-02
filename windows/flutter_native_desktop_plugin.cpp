#include "include/flutter_native_desktop/flutter_native_desktop_plugin.h"

// This must be included before many other Windows headers.
#include <windows.h>

// For getPlatformVersion; remove unless needed for your plugin implementation.
#include <VersionHelpers.h>

#include <flutter/method_channel.h>
#include <flutter/plugin_registrar_windows.h>
#include <flutter/standard_method_codec.h>

#include <map>
#include <memory>
#include <sstream>
#include <iostream>
#include <stdexcept>
#include <stdio.h>
#include <string>

using flutter::EncodableMap;
using flutter::EncodableValue;

const char kCommandKey[] = "command";

std::string exec(const char *cmd) {
  char buffer[128];
  std::string result = "";
  FILE *pipe = _popen(cmd, "r");
  if (!pipe)
    throw std::runtime_error("_popen() failed!");
  try {
    while (fgets(buffer, sizeof buffer, pipe) != NULL) {
      result += buffer;
    }
  } catch (...) {
    _pclose(pipe);
    throw;
  }
  _pclose(pipe);
  return result;
}

namespace {

class FlutterNativeDesktopPlugin : public flutter::Plugin {
 public:
  static void RegisterWithRegistrar(flutter::PluginRegistrarWindows *registrar);

  FlutterNativeDesktopPlugin();

  virtual ~FlutterNativeDesktopPlugin();

 private:
  // Called when a method is called on this plugin's channel from Dart.
  void HandleMethodCall(
      const flutter::MethodCall<flutter::EncodableValue> &method_call,
      std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result);
};

// static
void FlutterNativeDesktopPlugin::RegisterWithRegistrar(
    flutter::PluginRegistrarWindows *registrar) {

  auto channel =
      std::make_unique<flutter::MethodChannel<flutter::EncodableValue>>(
          registrar->messenger(), "flutter_native_desktop",
          &flutter::StandardMethodCodec::GetInstance());

  auto plugin = std::make_unique<FlutterNativeDesktopPlugin>();

  channel->SetMethodCallHandler(
      [plugin_pointer = plugin.get()](const auto &call, auto result) {
        plugin_pointer->HandleMethodCall(call, std::move(result));
      });

  registrar->AddPlugin(std::move(plugin));
}

FlutterNativeDesktopPlugin::FlutterNativeDesktopPlugin() {}

FlutterNativeDesktopPlugin::~FlutterNativeDesktopPlugin() {}

void FlutterNativeDesktopPlugin::HandleMethodCall(
    const flutter::MethodCall<flutter::EncodableValue> &method_call,
    std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result) {

   const auto* arguments = std::get_if<EncodableMap>(method_call.arguments());
   
  if (method_call.method_name().compare("getPlatformVersion") == 0) {
    std::ostringstream version_stream;
    version_stream << "Windows ";
    if (IsWindows10OrGreater()) {
      version_stream << "10+";
    } else if (IsWindows8OrGreater()) {
      version_stream << "8";
    } else if (IsWindows7OrGreater()) {
      version_stream << "7";
    }
    result->Success(flutter::EncodableValue(version_stream.str()));
  } else if (method_call.method_name().compare("call") == 0) {
    std::string command;
    auto command_it = arguments->find(EncodableValue(kCommandKey));
    command = std::get<std::string>(command_it->second);
    auto output = exec(command.c_str());
    result->Success(flutter::EncodableValue(output));
  } else if (method_call.method_name().compare("run") == 0) {
    std::string command;
    auto command_it = arguments->find(EncodableValue(kCommandKey));
    command = std::get<std::string>(command_it->second);
    std::system(command);
    result->Success(flutter::EncodableValue(""));}
     else {
    result->NotImplemented();
  }
}

}  // namespace

void FlutterNativeDesktopPluginRegisterWithRegistrar(
    FlutterDesktopPluginRegistrarRef registrar) {
  FlutterNativeDesktopPlugin::RegisterWithRegistrar(
      flutter::PluginRegistrarManager::GetInstance()
          ->GetRegistrar<flutter::PluginRegistrarWindows>(registrar));
}
