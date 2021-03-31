import 'dart:async';

import 'package:flutter/services.dart';

class FlutterNativeDesktop {
  static const MethodChannel _channel =
      const MethodChannel('flutter_native_desktop');

  static Future<String> get platformVersion async {
    final String version = await _channel.invokeMethod('getPlatformVersion');
    return version;
  }

  static Future<String> call(String command) async {
    return await _channel.invokeMethod("call", {"command": command});
  }

  static Future run(String command) async {
    await _channel.invokeMethod("run", {"command": command});
  }
}
