import 'dart:ui';
import 'package:flutter/material.dart';

class RoomDetailPage extends StatefulWidget {
  final String roomName;
  final IconData roomIcon;
  final List<Color> gradientColors;

  const RoomDetailPage({
    super.key,
    required this.roomName,
    required this.roomIcon,
    required this.gradientColors,
  });

  @override
  State<RoomDetailPage> createState() => _RoomDetailPageState();
}

class _RoomDetailPageState extends State<RoomDetailPage> {
  // Device states
  Map<String, bool> deviceStates = {
    'Light Bulbs': true,
    'CCTV Cameras': true,
    'Smart TV': false,
    'Air conditioner': true,
  };

  Map<String, int> deviceCounts = {
    'Light Bulbs': 3,
    'CCTV Cameras': 2,
    'Smart TV': 1,
    'Air conditioner': 1,
  };

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      body: Stack(
        children: [
          // 1. Full Screen Background Image
          Positioned.fill(
            child: Image.asset(
              _getRoomImage(),
              fit: BoxFit.cover,
              errorBuilder: (context, error, stackTrace) {
                return Container(
                  color: Colors.grey[900],
                  child: Center(
                    child: Icon(
                      widget.roomIcon,
                      size: 100,
                      color: Colors.white.withValues(alpha: 0.1),
                    ),
                  ),
                );
              },
            ),
          ),

          // 2. Gradient Overlay for readability
          Positioned.fill(
            child: Container(
              decoration: BoxDecoration(
                gradient: LinearGradient(
                  begin: Alignment.topCenter,
                  end: Alignment.bottomCenter,
                  colors: [
                    Colors.black.withValues(alpha: 0.3),
                    Colors.transparent,
                    Colors.black.withValues(alpha: 0.6),
                    Colors.black.withValues(alpha: 0.9),
                  ],
                  stops: const [0.0, 0.4, 0.7, 1.0],
                ),
              ),
            ),
          ),

          // 3. Content
          SafeArea(
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                // Header
                Padding(
                  padding: const EdgeInsets.all(20.0),
                  child: Row(
                    mainAxisAlignment: MainAxisAlignment.spaceBetween,
                    children: [
                      // Back Button
                      Container(
                        decoration: BoxDecoration(
                          color: Colors.white.withValues(alpha: 0.2),
                          shape: BoxShape.circle,
                          border: Border.all(
                            color: Colors.white.withValues(alpha: 0.3),
                            width: 1,
                          ),
                        ),
                        child: IconButton(
                          icon: const Icon(Icons.arrow_back, color: Colors.white),
                          onPressed: () => Navigator.pop(context),
                        ),
                      ),
                      
                      // Edit/Settings Button (Visual only)
                      Container(
                        decoration: BoxDecoration(
                          color: Colors.white.withValues(alpha: 0.2),
                          shape: BoxShape.circle,
                          border: Border.all(
                            color: Colors.white.withValues(alpha: 0.3),
                            width: 1,
                          ),
                        ),
                        child: IconButton(
                          icon: const Icon(Icons.more_vert, color: Colors.white),
                          onPressed: () {},
                        ),
                      ),
                    ],
                  ),
                ),

                const Spacer(),

                // Room Title & Info
                Padding(
                  padding: const EdgeInsets.symmetric(horizontal: 24.0),
                  child: Column(
                    crossAxisAlignment: CrossAxisAlignment.start,
                    children: [
                      Row(
                        children: [
                          Text(
                            widget.roomName,
                            style: const TextStyle(
                              fontSize: 42,
                              fontWeight: FontWeight.w300,
                              color: Colors.white,
                              letterSpacing: -1.0,
                            ),
                          ),
                          const SizedBox(width: 16),
                          Container(
                            padding: const EdgeInsets.symmetric(horizontal: 12, vertical: 6),
                            decoration: BoxDecoration(
                              color: widget.gradientColors[0].withValues(alpha: 0.8),
                              borderRadius: BorderRadius.circular(20),
                            ),
                            child: Row(
                              children: [
                                const Icon(Icons.thermostat, color: Colors.white, size: 16),
                                const SizedBox(width: 4),
                                const Text(
                                  "24°C",
                                  style: TextStyle(
                                    color: Colors.white,
                                    fontWeight: FontWeight.bold,
                                  ),
                                ),
                              ],
                            ),
                          ),
                        ],
                      ),
                      const SizedBox(height: 8),
                      Text(
                        "${_getActiveDeviceCount()} devices active",
                        style: TextStyle(
                          color: Colors.white.withValues(alpha: 0.8),
                          fontSize: 16,
                          fontWeight: FontWeight.w400,
                        ),
                      ),
                    ],
                  ),
                ),

                const SizedBox(height: 32),

                // Devices Control Panel (Glassmorphism)
                Container(
                  width: double.infinity,
                  padding: const EdgeInsets.only(top: 24, left: 24, right: 24, bottom: 24),
                  decoration: BoxDecoration(
                    color: Colors.white.withValues(alpha: 0.1),
                    borderRadius: const BorderRadius.only(
                      topLeft: Radius.circular(32),
                      topRight: Radius.circular(32),
                    ),
                    border: Border(
                      top: BorderSide(color: Colors.white.withValues(alpha: 0.2), width: 1),
                    ),
                  ),
                  child: ClipRRect(
                    borderRadius: const BorderRadius.only(
                      topLeft: Radius.circular(32),
                      topRight: Radius.circular(32),
                    ),
                    child: BackdropFilter(
                      filter: ImageFilter.blur(sigmaX: 10, sigmaY: 10),
                      child: Column(
                        crossAxisAlignment: CrossAxisAlignment.start,
                        children: [
                          const Text(
                            "SCENES & DEVICES",
                            style: TextStyle(
                              color: Colors.white54,
                              fontSize: 12,
                              fontWeight: FontWeight.bold,
                              letterSpacing: 1.5,
                            ),
                          ),
                          const SizedBox(height: 20),
                          SizedBox(
                            height: 140, // Height for horizontal scroll
                            child: ListView(
                              scrollDirection: Axis.horizontal,
                              physics: const BouncingScrollPhysics(),
                              children: [
                                _deviceCard(
                                  'Light Bulbs',
                                  Icons.lightbulb_outline_rounded,
                                  deviceStates['Light Bulbs']!,
                                  deviceCounts['Light Bulbs']!,
                                  [const Color(0xFFFFD54F), const Color(0xFFFFA726)],
                                ),
                                const SizedBox(width: 16),
                                _deviceCard(
                                  'Smart TV',
                                  Icons.tv_rounded,
                                  deviceStates['Smart TV']!,
                                  deviceCounts['Smart TV']!,
                                  [const Color(0xFF00b0ff), const Color(0xFF0091ea)],
                                ),
                                const SizedBox(width: 16),
                                _deviceCard(
                                  'AC Unit',
                                  Icons.ac_unit_rounded,
                                  deviceStates['Air conditioner']!,
                                  deviceCounts['Air conditioner']!,
                                  [const Color(0xFF00e676), const Color(0xFF00c853)],
                                ),
                                const SizedBox(width: 16),
                                _deviceCard(
                                  'Cameras',
                                  Icons.videocam_outlined,
                                  deviceStates['CCTV Cameras']!,
                                  deviceCounts['CCTV Cameras']!,
                                  [const Color(0xFFE040FB), const Color(0xFFAB47BC)],
                                ),
                              ],
                            ),
                          ),
                        ],
                      ),
                    ),
                  ),
                ),
              ],
            ),
          ),
        ],
      ),
    );
  }

  int _getActiveDeviceCount() {
    return deviceStates.values.where((state) => state).length;
  }

  Widget _deviceCard(
    String name,
    IconData icon,
    bool isActive,
    int deviceCount,
    List<Color> gradientColors,
  ) {
    return GestureDetector(
      onTap: () {
        setState(() {
          // Map generic names back to state keys if needed
          String key = name;
          if (name == 'AC Unit') key = 'Air conditioner';
          if (name == 'Cameras') key = 'CCTV Cameras';
          
          deviceStates[key] = !deviceStates[key]!;
        });
      },
      child: AnimatedContainer(
        duration: const Duration(milliseconds: 300),
        width: 130,
        padding: const EdgeInsets.all(16),
        decoration: BoxDecoration(
          color: isActive 
              ? gradientColors[0].withValues(alpha: 0.9) 
              : Colors.white.withValues(alpha: 0.1),
          borderRadius: BorderRadius.circular(24),
          border: Border.all(
            color: isActive 
                ? Colors.transparent 
                : Colors.white.withValues(alpha: 0.2),
            width: 1,
          ),
          boxShadow: isActive
              ? [
                  BoxShadow(
                    color: gradientColors[0].withValues(alpha: 0.4),
                    blurRadius: 12,
                    offset: const Offset(0, 8),
                  )
                ]
              : [],
        ),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          mainAxisAlignment: MainAxisAlignment.spaceBetween,
          children: [
            Container(
              padding: const EdgeInsets.all(8),
              decoration: BoxDecoration(
                color: Colors.white.withValues(alpha: 0.2),
                shape: BoxShape.circle,
              ),
              child: Icon(
                icon,
                color: Colors.white,
                size: 20,
              ),
            ),
            const SizedBox(height: 12),
            Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Text(
                  name,
                  style: const TextStyle(
                    color: Colors.white,
                    fontSize: 14,
                    fontWeight: FontWeight.bold,
                  ),
                  maxLines: 1,
                  overflow: TextOverflow.ellipsis,
                ),
                const SizedBox(height: 4),
                Text(
                  isActive ? "ON" : "OFF",
                  style: TextStyle(
                    color: Colors.white.withValues(alpha: 0.7),
                    fontSize: 12,
                    fontWeight: FontWeight.w500,
                  ),
                ),
              ],
            ),
          ],
        ),
      ),
    );
  }

  String _getRoomImage() {
    switch (widget.roomName) {
      case 'Kitchen':
        return 'assets/images/kitchen.jpg';
      case 'Living Room':
        return 'assets/images/living_room.jpg';
      case 'Bedroom':
        return 'assets/images/bghome.jpg';
      default:
        return 'assets/images/bghome.jpg';
    }
  }
}
