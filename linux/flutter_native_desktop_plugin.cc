#include "include/flutter_native_desktop/flutter_native_desktop_plugin.h"

#include <flutter_linux/flutter_linux.h>
#include <gtk/gtk.h>
#include <sys/utsname.h>

#include <cstring>
#include <iostream>
#include <stdexcept>
#include <stdio.h>
#include <string>

const char kCommandKey[] = "command";

#define FLUTTER_NATIVE_DESKTOP_PLUGIN(obj)                                     \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), flutter_native_desktop_plugin_get_type(), \
                              FlutterNativeDesktopPlugin))

struct _FlutterNativeDesktopPlugin {
  GObject parent_instance;
};

G_DEFINE_TYPE(FlutterNativeDesktopPlugin, flutter_native_desktop_plugin,
              g_object_get_type())

std::string exec(const char *cmd) {
  char buffer[128];
  std::string result = "";
  FILE *pipe = popen(cmd, "r");
  if (!pipe)
    throw std::runtime_error("popen() failed!");
  try {
    while (fgets(buffer, sizeof buffer, pipe) != NULL) {
      result += buffer;
    }
  } catch (...) {
    pclose(pipe);
    throw;
  }
  pclose(pipe);
  return result;
}

// Called when a method call is received from Flutter.
static void flutter_native_desktop_plugin_handle_method_call(
    FlutterNativeDesktopPlugin *self, FlMethodCall *method_call) {
  g_autoptr(FlMethodResponse) response = nullptr;

  const gchar *method = fl_method_call_get_name(method_call);
  FlValue *args = fl_method_call_get_args(method_call);
  if (strcmp(method, "getPlatformVersion") == 0) {
    struct utsname uname_data = {};
    uname(&uname_data);
    g_autofree gchar *version = g_strdup_printf("Linux %s", uname_data.version);
    g_autoptr(FlValue) result = fl_value_new_string(version);
    response = FL_METHOD_RESPONSE(fl_method_success_response_new(result));
  } else if (strcmp(method, "call") == 0) {
    FlValue *command_value = fl_value_lookup_string(args, kCommandKey);
    char *command = g_strdup(fl_value_get_string(command_value));
    std::string c = exec(command);
    g_autoptr(FlValue) result = fl_value_new_string(c.c_str());
    response = FL_METHOD_RESPONSE(fl_method_success_response_new(result));

  } else if (strcmp(method, "run") == 0) {
    FlValue *command_value = fl_value_lookup_string(args, kCommandKey);
    char *command = g_strdup(fl_value_get_string(command_value));
    exec(command);
    g_autoptr(FlValue) result = fl_value_new_string("");
    response = FL_METHOD_RESPONSE(fl_method_success_response_new(result));

  } else {
    response = FL_METHOD_RESPONSE(fl_method_not_implemented_response_new());
  }

  fl_method_call_respond(method_call, response, nullptr);
}

static void flutter_native_desktop_plugin_dispose(GObject *object) {
  G_OBJECT_CLASS(flutter_native_desktop_plugin_parent_class)->dispose(object);
}

static void flutter_native_desktop_plugin_class_init(
    FlutterNativeDesktopPluginClass *klass) {
  G_OBJECT_CLASS(klass)->dispose = flutter_native_desktop_plugin_dispose;
}

static void
flutter_native_desktop_plugin_init(FlutterNativeDesktopPlugin *self) {}

static void method_call_cb(FlMethodChannel *channel, FlMethodCall *method_call,
                           gpointer user_data) {
  FlutterNativeDesktopPlugin *plugin = FLUTTER_NATIVE_DESKTOP_PLUGIN(user_data);
  flutter_native_desktop_plugin_handle_method_call(plugin, method_call);
}

void flutter_native_desktop_plugin_register_with_registrar(
    FlPluginRegistrar *registrar) {
  FlutterNativeDesktopPlugin *plugin = FLUTTER_NATIVE_DESKTOP_PLUGIN(
      g_object_new(flutter_native_desktop_plugin_get_type(), nullptr));

  g_autoptr(FlStandardMethodCodec) codec = fl_standard_method_codec_new();
  g_autoptr(FlMethodChannel) channel =
      fl_method_channel_new(fl_plugin_registrar_get_messenger(registrar),
                            "flutter_native_desktop", FL_METHOD_CODEC(codec));
  fl_method_channel_set_method_call_handler(
      channel, method_call_cb, g_object_ref(plugin), g_object_unref);

  g_object_unref(plugin);
}
