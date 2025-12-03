import 'package:flutter/material.dart';
import 'package:camera/camera.dart';
import '../../services/face_recognition_api.dart';

enum CaptureDirection {
  center,
  up,
  down,
  left,
  right,
}

class AddPersonPage extends StatefulWidget {
  const AddPersonPage({super.key});

  @override
  State<AddPersonPage> createState() => _AddPersonPageState();
}

class _AddPersonPageState extends State<AddPersonPage> {
  CameraController? controller;
  List<CameraDescription>? cameras;
  final TextEditingController _nameController = TextEditingController();
  bool _isProcessing = false;
  bool _isCapturing = false;
  
  int _currentStep = 0;
  final List<CaptureDirection> _captureSequence = [
    CaptureDirection.center,
    CaptureDirection.up,
    CaptureDirection.down,
    CaptureDirection.left,
    CaptureDirection.right,
  ];
  
  final Map<CaptureDirection, XFile> _capturedImages = {};

  @override
  void initState() {
    super.initState();
    _initCamera();
  }

  Future<void> _initCamera() async {
    try {
      print('=== Initializing camera ===');
      cameras = await availableCameras();
      print('Available cameras: ${cameras?.length ?? 0}');
      
      if (cameras!.isNotEmpty) {
        // Find front camera
        CameraDescription? frontCamera;
        for (var camera in cameras!) {
          print('Camera: ${camera.name}, direction: ${camera.lensDirection}');
          if (camera.lensDirection == CameraLensDirection.front) {
            frontCamera = camera;
            break;
          }
        }
        
        // Use front camera if found, otherwise use first available
        final selectedCamera = frontCamera ?? cameras![0];
        print('Selected camera: ${selectedCamera.name}');
        
        controller = CameraController(selectedCamera, ResolutionPreset.high);
        await controller!.initialize();
        print('Camera initialized successfully');
        
        if (mounted) {
          setState(() {});
        }
      } else {
        print('ERROR: No cameras available');
      }
    } catch (e) {
      print('ERROR initializing camera: $e');
      if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(
            content: Text('Camera error: ${e.toString()}'),
            backgroundColor: Colors.red,
          ),
        );
      }
    }
  }

  @override
  void dispose() {
    controller?.dispose();
    _nameController.dispose();
    super.dispose();
  }

  String _getDirectionText(CaptureDirection direction) {
    switch (direction) {
      case CaptureDirection.center:
        return "Look at the camera";
      case CaptureDirection.up:
        return "Look up";
      case CaptureDirection.down:
        return "Look down";
      case CaptureDirection.left:
        return "Look left";
      case CaptureDirection.right:
        return "Look right";
    }
  }

  IconData _getDirectionIcon(CaptureDirection direction) {
    switch (direction) {
      case CaptureDirection.center:
        return Icons.center_focus_strong;
      case CaptureDirection.up:
        return Icons.arrow_upward;
      case CaptureDirection.down:
        return Icons.arrow_downward;
      case CaptureDirection.left:
        return Icons.arrow_back;
      case CaptureDirection.right:
        return Icons.arrow_forward;
    }
  }

  String _getDirectionPrefix(CaptureDirection direction) {
    switch (direction) {
      case CaptureDirection.center:
        return "center";
      case CaptureDirection.up:
        return "up";
      case CaptureDirection.down:
        return "down";
      case CaptureDirection.left:
        return "left";
      case CaptureDirection.right:
        return "right";
    }
  }

  Future<void> _startCaptureSequence() async {
    try {
      print('=== Start Capture Sequence Called ===');
      
      final name = _nameController.text.trim();
      print('Name entered: "$name"');
      
      if (name.isEmpty) {
        print('ERROR: Name is empty');
        ScaffoldMessenger.of(context).showSnackBar(
          const SnackBar(
            content: Text("Please enter a name"),
            backgroundColor: Colors.red,
          ),
        );
        return;
      }

      print('Starting capture sequence...');
      setState(() {
        _isCapturing = true;
        _currentStep = 0;
        _capturedImages.clear();
      });
      print('Capture sequence started! isCapturing: $_isCapturing, currentStep: $_currentStep');
    } catch (e, stackTrace) {
      print('=== ERROR in _startCaptureSequence ===');
      print('Error: $e');
      print('Stack trace: $stackTrace');
      
      if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(
            content: Text("Error: ${e.toString()}"),
            backgroundColor: Colors.red,
          ),
        );
      }
    }
  }

  Future<void> _captureCurrentDirection() async {
    if (controller == null || !controller!.value.isInitialized) return;

    setState(() {
      _isProcessing = true;
    });

    try {
      final direction = _captureSequence[_currentStep];
      final XFile photo = await controller!.takePicture();
      
      // Store the XFile directly
      _capturedImages[direction] = photo;

      // Move to next step or finish
      if (_currentStep < _captureSequence.length - 1) {
        setState(() {
          _currentStep++;
          _isProcessing = false;
        });
        
        // Auto-advance after a short delay
        await Future.delayed(const Duration(milliseconds: 800));
      } else {
        // All captures done, upload to API
        await _uploadToRaspberryPi();
      }
    } catch (e) {
      if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(
            content: Text("Error: ${e.toString()}"),
            backgroundColor: Colors.red,
          ),
        );
        setState(() {
          _isProcessing = false;
          _isCapturing = false;
        });
      }
    }
  }

  Future<void> _uploadToRaspberryPi() async {
    setState(() {
      _isProcessing = true;
    });

    try {
      print('=== Starting upload to Raspberry Pi ===');

      final name = _nameController.text.trim();
      print('Uploading person: $name');
      print('Number of captured images: ${_capturedImages.length}');
      
      if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          const SnackBar(
            content: Text("Uploading images to Raspberry Pi..."),
            backgroundColor: Colors.blue,
            duration: Duration(seconds: 2),
          ),
        );
      }
      
      // Prepare image paths for upload
      Map<String, String> imagePaths = {};
      for (var entry in _capturedImages.entries) {
        final prefix = _getDirectionPrefix(entry.key);
        imagePaths[prefix] = entry.value.path;
      }
      
      // Upload to Raspberry Pi
      final uploaded = await FaceRecognitionAPI.uploadPerson(name, imagePaths);
      
      if (mounted) {
        if (uploaded) {
          ScaffoldMessenger.of(context).showSnackBar(
            SnackBar(
              content: Text("✓ $name uploaded successfully to Raspberry Pi!"),
              backgroundColor: Colors.green,
              duration: const Duration(seconds: 3),
            ),
          );
          
          _nameController.clear();
          await Future.delayed(const Duration(seconds: 2));
          if (mounted) {
            Navigator.pop(context);
          }
        } else {
          ScaffoldMessenger.of(context).showSnackBar(
            const SnackBar(
              content: Text("Failed to upload. Check Raspberry Pi connection."),
              backgroundColor: Colors.red,
              duration: Duration(seconds: 3),
            ),
          );
        }
      }
    } catch (e, stackTrace) {
      print('=== CRITICAL ERROR in _uploadToRaspberryPi ===');
      print('Error: $e');
      print('Stack trace: $stackTrace');
      
      if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(
            content: Text("Upload error: ${e.toString()}"),
            backgroundColor: Colors.red,
            duration: const Duration(seconds: 4),
          ),
        );
      }
    } finally {
      if (mounted) {
        setState(() {
          _isProcessing = false;
          _isCapturing = false;
        });
      }
    }
  }

  @override
  Widget build(BuildContext context) {
    final screenSize = MediaQuery.of(context).size;
    
    return Scaffold(
      body: Container(
        decoration: BoxDecoration(
          gradient: LinearGradient(
            begin: Alignment.topCenter,
            end: Alignment.bottomCenter,
            colors: [
              Colors.grey[900]!,
              Colors.black,
            ],
          ),
        ),
        child: SafeArea(
          child: Column(
            children: [
              // Header with back button
              Padding(
                padding: const EdgeInsets.all(16.0),
                child: Row(
                  children: [
                    Container(
                      decoration: BoxDecoration(
                        color: Colors.grey[800],
                        shape: BoxShape.circle,
                      ),
                      child: IconButton(
                        icon: const Icon(Icons.arrow_back, color: Colors.white),
                        onPressed: _isCapturing ? null : () => Navigator.pop(context),
                      ),
                    ),
                  ],
                ),
              ),
              
              // Title and progress
              Padding(
                padding: const EdgeInsets.symmetric(horizontal: 24.0),
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    const Text(
                      "Add Person",
                      style: TextStyle(
                        color: Colors.white,
                        fontSize: 28,
                        fontWeight: FontWeight.bold,
                      ),
                    ),
                    const SizedBox(height: 8),
                    Text(
                      _isCapturing 
                          ? "Capturing ${_currentStep + 1}/${_captureSequence.length}"
                          : "Enter name and capture face from multiple angles",
                      style: TextStyle(
                        color: Colors.grey[400],
                        fontSize: 14,
                      ),
                    ),
                  ],
                ),
              ),
              
              const SizedBox(height: 24),
              
              // Name input field (only show when not capturing)
              if (!_isCapturing)
                Padding(
                  padding: const EdgeInsets.symmetric(horizontal: 24.0),
                  child: TextField(
                    controller: _nameController,
                    style: const TextStyle(color: Colors.white),
                    decoration: InputDecoration(
                      hintText: "Enter person's name",
                      hintStyle: TextStyle(color: Colors.grey[500]),
                      filled: true,
                      fillColor: Colors.grey[800],
                      border: OutlineInputBorder(
                        borderRadius: BorderRadius.circular(12),
                        borderSide: BorderSide.none,
                      ),
                      prefixIcon: Icon(Icons.person, color: Colors.grey[400]),
                    ),
                    enabled: !_isProcessing,
                  ),
                ),
              
              if (!_isCapturing) const SizedBox(height: 24),
              
              // Directional prompt (only show when capturing)
              if (_isCapturing)
                Padding(
                  padding: const EdgeInsets.symmetric(horizontal: 24.0, vertical: 16.0),
                  child: Container(
                    padding: const EdgeInsets.all(16),
                    decoration: BoxDecoration(
                      color: Colors.green.withValues(alpha: 0.2),
                      borderRadius: BorderRadius.circular(12),
                      border: Border.all(color: Colors.green, width: 2),
                    ),
                    child: Row(
                      mainAxisAlignment: MainAxisAlignment.center,
                      children: [
                        Icon(
                          _getDirectionIcon(_captureSequence[_currentStep]),
                          color: Colors.green,
                          size: 32,
                        ),
                        const SizedBox(width: 12),
                        Text(
                          _getDirectionText(_captureSequence[_currentStep]),
                          style: const TextStyle(
                            color: Colors.green,
                            fontSize: 20,
                            fontWeight: FontWeight.bold,
                          ),
                        ),
                      ],
                    ),
                  ),
                ),
              
              // Camera preview with circular overlay
              Expanded(
                child: controller == null || !controller!.value.isInitialized
                    ? Center(
                        child: Column(
                          mainAxisAlignment: MainAxisAlignment.center,
                          children: [
                            // Modern scanning frame
                            Stack(
                              alignment: Alignment.center,
                              children: [
                                // Outer frame
                                Container(
                                  width: screenSize.width * 0.6,
                                  height: screenSize.width * 0.6,
                                  decoration: BoxDecoration(
                                    border: Border.all(
                                      color: Colors.green.withValues(alpha: 0.3),
                                      width: 2,
                                    ),
                                    borderRadius: BorderRadius.circular(20),
                                  ),
                                ),
                                // Center camera icon
                                Icon(
                                  Icons.camera_alt_outlined,
                                  size: 80,
                                  color: Colors.green.withValues(alpha: 0.5),
                                ),
                              ],
                            ),
                            const SizedBox(height: 32),
                            const CircularProgressIndicator(
                              color: Colors.green,
                              strokeWidth: 3,
                            ),
                            const SizedBox(height: 16),
                            Text(
                              'Initializing camera...',
                              style: TextStyle(
                                color: Colors.grey[400],
                                fontSize: 16,
                                fontWeight: FontWeight.w500,
                              ),
                            ),
                          ],
                        ),
                      )
                    : Stack(
                        alignment: Alignment.center,
                        children: [
                          // Camera preview
                          Center(
                            child: ClipRRect(
                              borderRadius: BorderRadius.circular(20),
                              child: SizedBox(
                                width: screenSize.width - 48,
                                height: (screenSize.width - 48) * controller!.value.aspectRatio,
                                child: CameraPreview(controller!),
                              ),
                            ),
                          ),
                          
                          // Circular face overlay
                          CustomPaint(
                            size: Size(
                              screenSize.width * 0.6,
                              screenSize.width * 0.6,
                            ),
                            painter: CircularFaceOverlayPainter(
                              isCapturing: _isCapturing,
                            ),
                          ),
                        ],
                      ),
              ),
              
              const SizedBox(height: 24),
              
              // Progress indicator (when capturing)
              if (_isCapturing)
                Padding(
                  padding: const EdgeInsets.symmetric(horizontal: 24.0),
                  child: Row(
                    mainAxisAlignment: MainAxisAlignment.center,
                    children: List.generate(_captureSequence.length, (index) {
                      return Container(
                        margin: const EdgeInsets.symmetric(horizontal: 4),
                        width: 12,
                        height: 12,
                        decoration: BoxDecoration(
                          shape: BoxShape.circle,
                          color: index < _currentStep
                              ? Colors.green
                              : index == _currentStep
                                  ? Colors.green.withValues(alpha: 0.5)
                                  : Colors.grey[700],
                        ),
                      );
                    }),
                  ),
                ),
              
              if (_isCapturing) const SizedBox(height: 16),
              
              // Action button
              Padding(
                padding: const EdgeInsets.all(24.0),
                child: SizedBox(
                  width: double.infinity,
                  height: 56,
                  child: ElevatedButton(
                    onPressed: _isProcessing 
                        ? null 
                        : (_isCapturing ? _captureCurrentDirection : () => _startCaptureSequence()),
                    style: ElevatedButton.styleFrom(
                      backgroundColor: Colors.green,
                      disabledBackgroundColor: Colors.grey[700],
                      shape: RoundedRectangleBorder(
                        borderRadius: BorderRadius.circular(12),
                      ),
                    ),
                    child: _isProcessing
                        ? const SizedBox(
                            height: 24,
                            width: 24,
                            child: CircularProgressIndicator(
                              color: Colors.white,
                              strokeWidth: 2,
                            ),
                          )
                        : Text(
                            _isCapturing ? "Capture" : "Start Capture",
                            style: const TextStyle(
                              fontSize: 18,
                              fontWeight: FontWeight.bold,
                              color: Colors.white,
                            ),
                          ),
                  ),
                ),
              ),
            ],
          ),
        ),
      ),
    );
  }
}

// Custom painter for circular face overlay with modern design
class CircularFaceOverlayPainter extends CustomPainter {
  final bool isCapturing;

  CircularFaceOverlayPainter({required this.isCapturing});

  @override
  void paint(Canvas canvas, Size size) {
    final center = Offset(size.width / 2, size.height / 2);
    final radius = size.width / 2;

    // Draw outer glow
    if (isCapturing) {
      final glowPaint = Paint()
        ..color = Colors.green.withValues(alpha: 0.15)
        ..style = PaintingStyle.stroke
        ..strokeWidth = 20
        ..maskFilter = const MaskFilter.blur(BlurStyle.normal, 15);
      
      canvas.drawCircle(center, radius + 10, glowPaint);
    }

    // Draw main circle with gradient effect
    final paint = Paint()
      ..color = isCapturing 
          ? const Color(0xFF4CAF50)
          : Colors.white
      ..style = PaintingStyle.stroke
      ..strokeWidth = 4
      ..strokeCap = StrokeCap.round;

    canvas.drawCircle(center, radius, paint);

    // Draw inner circle for depth
    final innerPaint = Paint()
      ..color = (isCapturing ? const Color(0xFF4CAF50) : Colors.white).withValues(alpha: 0.2)
      ..style = PaintingStyle.stroke
      ..strokeWidth = 2;
    
    canvas.drawCircle(center, radius - 8, innerPaint);

    // Draw corner marks with modern style
    final markLength = 35.0;
    final markDistance = radius - 5;
    final markPaint = Paint()
      ..color = isCapturing ? const Color(0xFF4CAF50) : Colors.white
      ..style = PaintingStyle.stroke
      ..strokeWidth = 5
      ..strokeCap = StrokeCap.round;

    // Top mark
    canvas.drawLine(
      Offset(center.dx, center.dy - markDistance - markLength),
      Offset(center.dx, center.dy - markDistance),
      markPaint,
    );

    // Bottom mark
    canvas.drawLine(
      Offset(center.dx, center.dy + markDistance),
      Offset(center.dx, center.dy + markDistance + markLength),
      markPaint,
    );

    // Left mark
    canvas.drawLine(
      Offset(center.dx - markDistance - markLength, center.dy),
      Offset(center.dx - markDistance, center.dy),
      markPaint,
    );

    // Right mark
    canvas.drawLine(
      Offset(center.dx + markDistance, center.dy),
      Offset(center.dx + markDistance + markLength, center.dy),
      markPaint,
    );

    // Draw corner dots for modern look
    final dotPaint = Paint()
      ..color = isCapturing ? const Color(0xFF4CAF50) : Colors.white
      ..style = PaintingStyle.fill;

    final dotRadius = 4.0;
    
    // Top dot
    canvas.drawCircle(
      Offset(center.dx, center.dy - markDistance - markLength - 8),
      dotRadius,
      dotPaint,
    );

    // Bottom dot
    canvas.drawCircle(
      Offset(center.dx, center.dy + markDistance + markLength + 8),
      dotRadius,
      dotPaint,
    );

    // Left dot
    canvas.drawCircle(
      Offset(center.dx - markDistance - markLength - 8, center.dy),
      dotRadius,
      dotPaint,
    );

    // Right dot
    canvas.drawCircle(
      Offset(center.dx + markDistance + markLength + 8, center.dy),
      dotRadius,
      dotPaint,
    );
  }

  @override
  bool shouldRepaint(CircularFaceOverlayPainter oldDelegate) {
    return oldDelegate.isCapturing != isCapturing;
  }
}
