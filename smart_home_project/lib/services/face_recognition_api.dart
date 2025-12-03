import 'dart:convert';
import 'dart:io';
import 'package:http/http.dart' as http;

class FaceRecognitionAPI {
  // Server running on your Raspberry Pi
  static const String baseUrl = 'http://192.168.100.148:5000';
  // If using Android emulator on same PC, use: 'http://10.0.2.2:5000'
  
  /// Upload person's face images to the backend server
  /// 
  /// This automatically:
  /// 1. Sends all images to Raspberry Pi
  /// 2. Saves them in the correct folder structure
  /// 3. Triggers model retraining
  static Future<bool> uploadPerson(String name, Map<String, String> imagePaths) async {
    try {
      // Convert images to base64
      Map<String, String> base64Images = {};
      
      for (var entry in imagePaths.entries) {
        final file = File(entry.value);
        if (await file.exists()) {
          final bytes = await file.readAsBytes();
          base64Images[entry.key] = base64Encode(bytes);
        }
      }
      
      if (base64Images.isEmpty) {
        print('No images to upload');
        return false;
      }
      
      // Send to server
      final response = await http.post(
        Uri.parse('$baseUrl/api/upload_person'),
        headers: {'Content-Type': 'application/json'},
        body: jsonEncode({
          'name': name,
          'images': base64Images,
        }),
      ).timeout(const Duration(seconds: 30));
      
      if (response.statusCode == 200) {
        final data = jsonDecode(response.body);
        print('Upload successful: ${data['message']}');
        return true;
      } else {
        print('Upload failed: ${response.statusCode} - ${response.body}');
        return false;
      }
      
    } catch (e) {
      print('Error uploading to server: $e');
      return false;
    }
  }
  
  /// Get server status and list of registered persons
  static Future<Map<String, dynamic>?> getStatus() async {
    try {
      final response = await http.get(
        Uri.parse('$baseUrl/api/status'),
      ).timeout(const Duration(seconds: 10));
      
      if (response.statusCode == 200) {
        return jsonDecode(response.body);
      }
      return null;
    } catch (e) {
      print('Error getting status: $e');
      return null;
    }
  }
  
  /// Trigger manual model training
  static Future<bool> trainModel() async {
    try {
      final response = await http.post(
        Uri.parse('$baseUrl/api/train'),
      ).timeout(const Duration(minutes: 5));
      
      return response.statusCode == 200;
    } catch (e) {
      print('Error training model: $e');
      return false;
    }
  }
}
