import 'dart:io';
import 'dart:convert';
import 'package:flutter/material.dart';

void main() {
  runApp(ParkingApp());
}

class ParkingApp extends StatelessWidget {
  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      debugShowCheckedModeBanner: false,
      home: ParkingLotScreen(),
    );
  }
}

class ParkingLotScreen extends StatefulWidget {
  @override
  _ParkingLotScreenState createState() => _ParkingLotScreenState();
}

class _ParkingLotScreenState extends State<ParkingLotScreen> {
  List<int> parkingSpots = List.filled(4, 0); // 초기 상태: 모두 사용 불가
  String serverIp = '192.168.199.29'; // 라즈베리파이의 IP 주소
  final int serverPort = 8080;
  Socket? _socket;
  bool isConnected = false; // 서버 연결 상태

  @override
  void initState() {
    super.initState();
    _connectToServer();
  }

  @override
  void dispose() {
    _socket?.close(); // 소켓 닫기
    super.dispose();
  }

  Future<void> _connectToServer() async {
    if (_socket != null) {
      await _socket?.close();
      print('Previous connection closed.');
    }

    try {
      _socket = await Socket.connect(serverIp, serverPort);
      setState(() {
        isConnected = true;
      });
      print('Connected to server');

      final jsonData = jsonEncode({"class": 2});
      _socket?.write(jsonData);
      print('Sent JSON to server: $jsonData');

      _socket?.listen((data) {
        try {
          final message = utf8.decode(data);
          final response = jsonDecode(message);

          if (response['index'] is List) {
            setState(() {
              parkingSpots = List<int>.from(response['index']);
            });
          }

          print('Received response from server: $response');
        } catch (e) {
          print('Error parsing response JSON: $e');
        }
      }, onDone: () {
        print('Connection closed by server');
        setState(() {
          isConnected = false;
        });
        _socket?.close();
      }, onError: (error) {
        print('Error: $error');
        setState(() {
          isConnected = false;
        });
        _socket?.close();
      });
    } catch (e) {
      print('Error connecting to server: $e');
      setState(() {
        isConnected = false;
      });
    }
  }

  @override
  Widget build(BuildContext context) {
    final totalSpots = parkingSpots.length;
    final availableSpots = 4 - parkingSpots.where((spot) => spot == 1).length;

    return Scaffold(
      body: Center(
        child: Column(
          mainAxisAlignment: MainAxisAlignment.spaceAround,
          children: [
            // 첫 번째 층
            Column(
              children: [
                Container(
                  padding: EdgeInsets.all(8),
                  decoration: BoxDecoration(
                    color: Colors.blue,
                    borderRadius: BorderRadius.circular(8),
                  ),
                  child: Text(
                    "1F",
                    style: TextStyle(
                      color: Colors.white,
                      fontSize: 16,
                      fontWeight: FontWeight.bold,
                    ),
                  ),
                ),
                Row(
                  mainAxisAlignment: MainAxisAlignment.spaceAround,
                  children: [
                    _buildParkingSpot(0),
                    _buildParkingSpot(1),
                  ],
                ),
              ],
            ),
            // 두 번째 층
            Column(
              children: [
                Container(
                  padding: EdgeInsets.all(8),
                  decoration: BoxDecoration(
                    color: Colors.blue,
                    borderRadius: BorderRadius.circular(8),
                  ),
                  child: Text(
                    "2F",
                    style: TextStyle(
                      color: Colors.white,
                      fontSize: 16,
                      fontWeight: FontWeight.bold,
                    ),
                  ),
                ),
                Row(
                  mainAxisAlignment: MainAxisAlignment.spaceAround,
                  children: [
                    _buildParkingSpot(2),
                    _buildParkingSpot(3),
                  ],
                ),
              ],
            ),
            // 주차 가능 대수 표시
            Container(
              padding: EdgeInsets.symmetric(horizontal: 20, vertical: 10),
              decoration: BoxDecoration(
                color: Colors.blue,
                borderRadius: BorderRadius.circular(8),
              ),
              child: Text(
                "주차 가능 대수: $availableSpots",
                style: TextStyle(
                  color: Colors.white,
                  fontSize: 18,
                  fontWeight: FontWeight.bold,
                ),
              ),
            ),
          ],
        ),
      ),
      bottomNavigationBar: Container(
        padding: EdgeInsets.all(8),
        color: Colors.white,
        child: Row(
          mainAxisAlignment: MainAxisAlignment.center,
          children: [
            Text(
              'Current IP: $serverIp',
              style: TextStyle(fontSize: 12, fontWeight: FontWeight.normal),
            ),
          ],
        ),
      ),
      floatingActionButton: Stack(
        children: [
          Positioned(
            bottom: 80,
            right: 16,
            child: Container(
              width: 24,
              height: 24,
              decoration: BoxDecoration(
                shape: BoxShape.circle,
                color: isConnected ? Colors.green : Colors.red,
              ),
            ),
          ),
          Positioned(
            bottom: 16,
            right: 16,
            child: FloatingActionButton(
              onPressed: _connectToServer,
              child: Icon(Icons.refresh),
              tooltip: 'Reconnect to Server',
            ),
          ),
          Positioned(
            bottom: 16,
            left: 16,
            child: FloatingActionButton(
              onPressed: () => _showIpDialog(context),
              child: Icon(Icons.settings),
              tooltip: 'Edit IP Address',
            ),
          ),
        ],
      ),
      floatingActionButtonLocation: FloatingActionButtonLocation.endFloat,
    );
  }

  void _showIpDialog(BuildContext context) {
    final TextEditingController ipController =
    TextEditingController(text: serverIp);

    showDialog(
      context: context,
      builder: (BuildContext context) {
        return AlertDialog(
          title: Text('Edit IP Address'),
          content: TextField(
            controller: ipController,
            decoration: InputDecoration(
              labelText: 'Server IP',
              border: OutlineInputBorder(),
            ),
          ),
          actions: [
            TextButton(
              onPressed: () {
                Navigator.pop(context);
              },
              child: Text('Cancel'),
            ),
            TextButton(
              onPressed: () {
                setState(() {
                  serverIp = ipController.text;
                });
                Navigator.pop(context);
              },
              child: Text('Save'),
            ),
          ],
        );
      },
    );
  }

  Widget _buildParkingSpot(int index) {
    return Container(
      width: 100,
      height: 150,
      margin: EdgeInsets.all(8),
      decoration: BoxDecoration(
        color: parkingSpots[index] == 1
            ? Colors.red.withOpacity(0.7)
            : Colors.green.withOpacity(0.7),
        borderRadius: BorderRadius.circular(8),
      ),
      child: Center(
        child: Text(
          "${index + 1}",
          style: TextStyle(
            color: Colors.white,
            fontWeight: FontWeight.bold,
            fontSize: 20,
          ),
        ),
      ),
    );
  }
}
