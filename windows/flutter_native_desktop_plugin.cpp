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


int exec(const char* cmd, std::string& outOutput) {

    HANDLE g_hChildStd_OUT_Rd = NULL;
    HANDLE g_hChildStd_OUT_Wr = NULL;
    HANDLE g_hChildStd_ERR_Rd = NULL;
    HANDLE g_hChildStd_ERR_Wr = NULL;

    SECURITY_ATTRIBUTES sa;
    // Set the bInheritHandle flag so pipe handles are inherited.
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = NULL;
    if (!CreatePipe(&g_hChildStd_ERR_Rd, &g_hChildStd_ERR_Wr, &sa, 0))     { return 1; } // Create a pipe for the child process's STDERR.
    if (!SetHandleInformation(g_hChildStd_ERR_Rd, HANDLE_FLAG_INHERIT, 0)) { return 1; } // Ensure the read handle to the pipe for STDERR is not inherited.
    if (!CreatePipe(&g_hChildStd_OUT_Rd, &g_hChildStd_OUT_Wr, &sa, 0))     { return 1; } // Create a pipe for the child process's STDOUT.
    if (!SetHandleInformation(g_hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0)) { return 1; } // Ensure the read handle to the pipe for STDOUT is not inherited

    PROCESS_INFORMATION piProcInfo;
    STARTUPINFO siStartInfo;
    bool bSuccess = FALSE;

    // Set up members of the PROCESS_INFORMATION structure.
    ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));

    // Set up members of the STARTUPINFO structure.
    // This structure specifies the STDERR and STDOUT handles for redirection.
    ZeroMemory(&siStartInfo, sizeof(STARTUPINFO));
    siStartInfo.cb = sizeof(STARTUPINFO);
    siStartInfo.hStdError  = g_hChildStd_ERR_Wr;
    siStartInfo.hStdOutput = g_hChildStd_OUT_Wr;
    siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

    // Create the child process.
    bSuccess = CreateProcess(
        NULL,             // program name
        (char*)cmd,       // command line
        NULL,             // process security attributes
        NULL,             // primary thread security attributes
        TRUE,             // handles are inherited
        CREATE_NO_WINDOW, // creation flags (this is what hides the window)
        NULL,             // use parent's environment
        NULL,             // use parent's current directory
        &siStartInfo,     // STARTUPINFO pointer
        &piProcInfo       // receives PROCESS_INFORMATION
    );

    CloseHandle(g_hChildStd_ERR_Wr);
    CloseHandle(g_hChildStd_OUT_Wr);

    // read output
    #define BUFSIZE 4096
    DWORD dwRead;
    CHAR chBuf[BUFSIZE];
    bool bSuccess2 = FALSE;
    for (;;) { // read stdout
        bSuccess2 = ReadFile(g_hChildStd_OUT_Rd, chBuf, BUFSIZE, &dwRead, NULL);
        if(!bSuccess2 || dwRead == 0) break;
        std::string s(chBuf, dwRead);
        outOutput += s;
    }
    dwRead = 0;
    for (;;) { // read stderr
        bSuccess2 = ReadFile(g_hChildStd_ERR_Rd, chBuf, BUFSIZE, &dwRead, NULL);
        if(!bSuccess2 || dwRead == 0) break;
        std::string s(chBuf, dwRead);
        outOutput += s;
    }

    // The remaining open handles are cleaned up when this process terminates.
    // To avoid resource leaks in a larger application,
    // close handles explicitly.
    return 0;
}

// std::string exec(const char *cmd) {
//   char buffer[128];
//   std::string result = "";
//   FILE *pipe = _popen(cmd, "r");
//   if (!pipe)
//     throw std::runtime_error("_popen() failed!");
//   try {
//     while (fgets(buffer, sizeof buffer, pipe) != NULL) {
//       result += buffer;
//     }
//   } catch (...) {
//     _pclose(pipe);
//     throw;
//   }
//   _pclose(pipe);
//   return result;
// }

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
    std::string output;
    exec(command.c_str(), output);
    result->Success(flutter::EncodableValue(output));
  } else if (method_call.method_name().compare("run") == 0) {
    std::string command;
    auto command_it = arguments->find(EncodableValue(kCommandKey));
    command = std::get<std::string>(command_it->second);
    std::system(command.c_str());
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
