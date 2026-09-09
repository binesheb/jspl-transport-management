import 'dart:async';
import 'dart:convert';

import 'package:flutter/material.dart';
import 'package:geolocator/geolocator.dart';
import 'package:http/http.dart' as http;
import 'package:shared_preferences/shared_preferences.dart';
import 'package:vibration/vibration.dart';

const defaultApiBase = String.fromEnvironment(
  'DRIVE_API',
  defaultValue: 'http://10.0.2.2:8000',
);

class Config {
  static String api = defaultApiBase;

  static Future<void> load() async {
    final prefs = await SharedPreferences.getInstance();
    api = prefs.getString('api') ?? defaultApiBase;
  }

  static Future<void> save(String value) async {
    final cleaned = value.trim().replaceFirst(RegExp(r'/$'), '');
    final response = await http
        .get(Uri.parse('$cleaned/health'))
        .timeout(const Duration(seconds: 6));
    if (response.statusCode != 200) {
      throw Exception('Health check failed (${response.statusCode})');
    }
    final prefs = await SharedPreferences.getInstance();
    await prefs.setString('api', cleaned);
    api = cleaned;
  }
}

class Api {
  static Uri uri(String path) => Uri.parse('${Config.api}$path');

  static const headers = {'content-type': 'application/json'};

  static Future<dynamic> get(String path) async {
    final response = await http
        .get(uri(path))
        .timeout(const Duration(seconds: 8));
    if (response.statusCode < 200 || response.statusCode >= 300) {
      throw Exception('GET failed ${response.statusCode}');
    }
    return jsonDecode(response.body);
  }

  static Future<dynamic> post(String path, Map<String, dynamic> body) async {
    final response = await http
        .post(uri(path), headers: headers, body: jsonEncode(body))
        .timeout(const Duration(seconds: 8));
    if (response.statusCode < 200 || response.statusCode >= 300) {
      throw Exception('POST failed ${response.statusCode}: ${response.body}');
    }
    return jsonDecode(response.body);
  }
}

Future<void> main() async {
  WidgetsFlutterBinding.ensureInitialized();
  await Config.load();
  runApp(const App());
}

class App extends StatelessWidget {
  const App({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'Jayalakshmi DRIVE',
      debugShowCheckedModeBanner: false,
      theme: ThemeData(
        useMaterial3: true,
        colorSchemeSeed: const Color(0xFF8B1E3F),
      ),
      home: const Home(),
    );
  }
}

class Home extends StatefulWidget {
  const Home({super.key});

  @override
  State<Home> createState() => _HomeState();
}

class _HomeState extends State<Home> {
  Future<void> openServerSetup() async {
    await Navigator.push(
      context,
      MaterialPageRoute(builder: (_) => const ServerSetup()),
    );
    if (mounted) setState(() {});
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      body: SafeArea(
        child: Padding(
          padding: const EdgeInsets.all(24),
          child: Column(
            children: [
              const Spacer(),
              const Text(
                'JAYALAKSHMI',
                style: TextStyle(fontWeight: FontWeight.w700, letterSpacing: 2),
              ),
              const Text(
                'DRIVE',
                style: TextStyle(fontSize: 44, fontWeight: FontWeight.w900),
              ),
              const SizedBox(height: 8),
              const Text('Manager booking + driver dispatch field PoC'),
              const SizedBox(height: 24),
              Card(
                child: ListTile(
                  leading: const Icon(Icons.cloud_outlined),
                  title: const Text('PoC server'),
                  subtitle: Text(Config.api),
                  trailing: const Icon(Icons.settings),
                  onTap: openServerSetup,
                ),
              ),
              const SizedBox(height: 14),
              RoleCard(
                title: 'Manager / Booker',
                subtitle: 'Request a driver',
                icon: Icons.person_search,
                onTap: () => Navigator.push(
                  context,
                  MaterialPageRoute(builder: (_) => const Manager()),
                ),
              ),
              const SizedBox(height: 12),
              RoleCard(
                title: 'Driver',
                subtitle: 'Go online and receive calls',
                icon: Icons.directions_car,
                onTap: () => Navigator.push(
                  context,
                  MaterialPageRoute(builder: (_) => const Driver()),
                ),
              ),
              const Spacer(),
              const Text('Android field PoC'),
            ],
          ),
        ),
      ),
    );
  }
}

class RoleCard extends StatelessWidget {
  final String title;
  final String subtitle;
  final IconData icon;
  final VoidCallback onTap;

  const RoleCard({
    required this.title,
    required this.subtitle,
    required this.icon,
    required this.onTap,
    super.key,
  });

  @override
  Widget build(BuildContext context) {
    return Card(
      child: ListTile(
        leading: CircleAvatar(child: Icon(icon)),
        title: Text(title, style: const TextStyle(fontWeight: FontWeight.w700)),
        subtitle: Text(subtitle),
        trailing: const Icon(Icons.chevron_right),
        onTap: onTap,
      ),
    );
  }
}

class ServerSetup extends StatefulWidget {
  const ServerSetup({super.key});

  @override
  State<ServerSetup> createState() => _ServerSetupState();
}

class _ServerSetupState extends State<ServerSetup> {
  late final TextEditingController controller =
      TextEditingController(text: Config.api);
  bool busy = false;

  @override
  void dispose() {
    controller.dispose();
    super.dispose();
  }

  Future<void> save() async {
    setState(() => busy = true);
    try {
      await Config.save(controller.text);
      if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          const SnackBar(content: Text('Connected and saved')),
        );
        Navigator.pop(context);
      }
    } catch (error) {
      if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(content: Text('$error')),
        );
      }
    } finally {
      if (mounted) setState(() => busy = false);
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(title: const Text('PoC Server')),
      body: Padding(
        padding: const EdgeInsets.all(20),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text(
              'Server URL',
              style: Theme.of(context)
                  .textTheme
                  .headlineSmall
                  ?.copyWith(fontWeight: FontWeight.w800),
            ),
            const SizedBox(height: 8),
            const Text(
              'For the driver phone on the same Wi-Fi, use your laptop LAN address, e.g. http://192.168.1.20:8000.',
            ),
            const SizedBox(height: 16),
            TextField(
              controller: controller,
              keyboardType: TextInputType.url,
              decoration: const InputDecoration(
                border: OutlineInputBorder(),
                prefixIcon: Icon(Icons.link),
              ),
            ),
            const SizedBox(height: 16),
            SizedBox(
              width: double.infinity,
              child: FilledButton(
                onPressed: busy ? null : save,
                child: Text(busy ? 'TESTING...' : 'TEST & SAVE'),
              ),
            ),
          ],
        ),
      ),
    );
  }
}

class Manager extends StatefulWidget {
  const Manager({super.key});

  @override
  State<Manager> createState() => _ManagerState();
}

class _ManagerState extends State<Manager> {
  final destination = TextEditingController();
  final pickup = TextEditingController(text: 'Jayalakshmi MG Road');
  Timer? timer;
  String? bookingId;
  Map<String, dynamic>? booking;
  bool busy = false;

  @override
  void initState() {
    super.initState();
    timer = Timer.periodic(const Duration(seconds: 2), (_) => refresh());
  }

  @override
  void dispose() {
    timer?.cancel();
    destination.dispose();
    pickup.dispose();
    super.dispose();
  }

  Future<void> refresh() async {
    final id = bookingId;
    if (id == null) return;
    try {
      final response = await Api.get('/api/poc/bookings/$id');
      if (mounted) {
        setState(() => booking = Map<String, dynamic>.from(response));
      }
    } catch (_) {}
  }

  Future<void> book() async {
    if (destination.text.trim().isEmpty || busy) return;
    setState(() => busy = true);
    try {
      final created = Map<String, dynamic>.from(
        await Api.post('/api/poc/bookings', {
          'pickup': pickup.text.trim(),
          'destination': destination.text.trim(),
        }),
      );
      bookingId = created['booking_id'] as String?;

      final drivers = List<Map<String, dynamic>>.from(
        await Api.get('/api/poc/drivers'),
      );
      final available = drivers
          .where((driver) =>
              driver['online'] == true && driver['status'] == 'available')
          .toList();

      if (available.isNotEmpty && bookingId != null) {
        await Api.post('/api/poc/bookings/$bookingId/offer', {
          'driver_id': available.first['driver_id'],
        });
      }
      await refresh();
    } catch (error) {
      if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(content: Text('$error')),
        );
      }
    } finally {
      if (mounted) setState(() => busy = false);
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(title: const Text('Manager / Booker')),
      body: ListView(
        padding: const EdgeInsets.all(18),
        children: [
          const MapBox(),
          const SizedBox(height: 16),
          Text(
            'Where are you going?',
            style: Theme.of(context)
                .textTheme
                .headlineSmall
                ?.copyWith(fontWeight: FontWeight.w800),
          ),
          const SizedBox(height: 12),
          TextField(
            controller: pickup,
            decoration: const InputDecoration(
              labelText: 'Pickup',
              prefixIcon: Icon(Icons.my_location),
              border: OutlineInputBorder(),
            ),
          ),
          const SizedBox(height: 10),
          TextField(
            controller: destination,
            decoration: const InputDecoration(
              labelText: 'Destination',
              hintText: 'Airport, showroom, hotel...',
              prefixIcon: Icon(Icons.search),
              border: OutlineInputBorder(),
            ),
          ),
          const SizedBox(height: 12),
          SizedBox(
            height: 52,
            child: FilledButton.icon(
              onPressed: busy ? null : book,
              icon: const Icon(Icons.flash_on),
              label: Text(busy ? 'REQUESTING...' : 'RIDE NOW'),
            ),
          ),
          if (booking != null) ...[
            const SizedBox(height: 16),
            BookingCard(booking!),
          ],
        ],
      ),
    );
  }
}

class Driver extends StatefulWidget {
  const Driver({super.key});

  @override
  State<Driver> createState() => _DriverState();
}

class _DriverState extends State<Driver> {
  static const driverId = 'driver-demo-01';
  bool online = false;
  bool busy = false;
  Timer? pollTimer;
  Timer? locationTimer;
  Map<String, dynamic>? offer;
  String? activeBookingId;
  Position? position;

  @override
  void initState() {
    super.initState();
    pollTimer = Timer.periodic(const Duration(seconds: 2), (_) => checkForOffer());
  }

  @override
  void dispose() {
    pollTimer?.cancel();
    locationTimer?.cancel();
    if (online) {
      Api.post('/api/poc/drivers/offline', {'driver_id': driverId});
    }
    super.dispose();
  }

  Future<void> ensureLocationPermission() async {
    if (!await Geolocator.isLocationServiceEnabled()) {
      throw Exception('Turn on phone Location');
    }
    var permission = await Geolocator.checkPermission();
    if (permission == LocationPermission.denied) {
      permission = await Geolocator.requestPermission();
    }
    if (permission == LocationPermission.denied ||
        permission == LocationPermission.deniedForever) {
      throw Exception('Location permission is required');
    }
  }

  Future<void> toggleOnline() async {
    if (busy) return;
    setState(() => busy = true);
    try {
      if (!online) {
        await ensureLocationPermission();
        await Api.post('/api/poc/drivers/online', {
          'driver_id': driverId,
          'name': 'Driver Demo',
          'vehicle': 'Toyota Innova • KL-XX-0000',
        });
        setState(() => online = true);
        startLocationUpdates();
      } else {
        await Api.post('/api/poc/drivers/offline', {'driver_id': driverId});
        locationTimer?.cancel();
        setState(() => online = false);
      }
    } catch (error) {
      if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(content: Text('$error')),
        );
      }
    } finally {
      if (mounted) setState(() => busy = false);
    }
  }

  void startLocationUpdates() {
    locationTimer?.cancel();
    locationTimer = Timer.periodic(const Duration(seconds: 5), (_) async {
      try {
        final current = await Geolocator.getCurrentPosition();
        position = current;
        await Api.post('/api/poc/drivers/location', {
          'driver_id': driverId,
          'latitude': current.latitude,
          'longitude': current.longitude,
          'accuracy': current.accuracy,
          'speed': current.speed,
          'heading': current.heading,
        });
        if (mounted) setState(() {});
      } catch (_) {}
    });
  }

  Future<void> checkForOffer() async {
    if (!online) return;
    try {
      final bookings = List<Map<String, dynamic>>.from(
        await Api.get('/api/poc/bookings'),
      );
      final matching = bookings.where((booking) =>
          booking['status'] == 'offered' &&
          booking['offered_to'] == driverId);
      if (matching.isNotEmpty && offer == null) {
        if (await Vibration.hasVibrator()) {
          Vibration.vibrate(duration: 1200);
        }
        if (mounted) setState(() => offer = matching.first);
      }
    } catch (_) {}
  }

  Future<void> answerOffer(bool accept) async {
    final currentOffer = offer;
    if (currentOffer == null) return;
    try {
      final action = accept ? 'accept' : 'decline';
      final response = Map<String, dynamic>.from(
        await Api.post(
          '/api/poc/bookings/${currentOffer['booking_id']}/$action',
          {'driver_id': driverId},
        ),
      );
      if (mounted) {
        setState(() {
          offer = null;
          activeBookingId = accept ? response['booking_id'] as String? : null;
        });
      }
    } catch (error) {
      if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(content: Text('$error')),
        );
      }
    }
  }

  Future<void> updateTripState(String state) async {
    final id = activeBookingId;
    if (id == null) return;
    try {
      await Api.post('/api/poc/bookings/$id/status', {'status': state});
      if (state == 'completed' && mounted) {
        setState(() => activeBookingId = null);
      }
    } catch (error) {
      if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(content: Text('$error')),
        );
      }
    }
  }

  @override
  Widget build(BuildContext context) {
    final gpsText = position == null
        ? 'GPS waiting'
        : 'GPS ${position!.latitude.toStringAsFixed(5)}, ${position!.longitude.toStringAsFixed(5)}';

    return Scaffold(
      appBar: AppBar(title: const Text('Driver')),
      body: ListView(
        padding: const EdgeInsets.all(18),
        children: [
          Card(
            child: ListTile(
              leading: const CircleAvatar(child: Icon(Icons.person)),
              title: const Text(
                'Driver Demo',
                style: TextStyle(fontWeight: FontWeight.w800),
              ),
              subtitle: const Text('Toyota Innova • KL-XX-0000'),
              trailing: Switch(
                value: online,
                onChanged: busy ? null : (_) => toggleOnline(),
              ),
            ),
          ),
          Card(
            child: ListTile(
              leading: Icon(online ? Icons.wifi : Icons.wifi_off),
              title: Text(online ? 'ONLINE' : 'OFFLINE'),
              subtitle: Text(gpsText),
            ),
          ),
          if (offer != null) ...[
            const SizedBox(height: 12),
            Card(
              color: Theme.of(context).colorScheme.primaryContainer,
              child: Padding(
                padding: const EdgeInsets.all(18),
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    const Text(
                      '🔔 NEW TRIP',
                      style: TextStyle(fontSize: 20, fontWeight: FontWeight.w900),
                    ),
                    const SizedBox(height: 8),
                    Text(
                      '${offer!['destination']}',
                      style: Theme.of(context)
                          .textTheme
                          .headlineSmall
                          ?.copyWith(fontWeight: FontWeight.w800),
                    ),
                    Text('Pickup: ${offer!['pickup']}'),
                    const SizedBox(height: 14),
                    Row(
                      children: [
                        Expanded(
                          child: OutlinedButton(
                            onPressed: () => answerOffer(false),
                            child: const Text('DECLINE'),
                          ),
                        ),
                        const SizedBox(width: 10),
                        Expanded(
                          child: FilledButton(
                            onPressed: () => answerOffer(true),
                            child: const Text('ACCEPT'),
                          ),
                        ),
                      ],
                    ),
                  ],
                ),
              ),
            ),
          ],
          if (activeBookingId != null) ...[
            const SizedBox(height: 12),
            Card(
              child: Padding(
                padding: const EdgeInsets.all(18),
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    const Text(
                      'ACTIVE TRIP',
                      style: TextStyle(fontWeight: FontWeight.w800),
                    ),
                    Text(activeBookingId!),
                    const SizedBox(height: 8),
                    Wrap(
                      spacing: 8,
                      runSpacing: 8,
                      children: [
                        OutlinedButton(
                          onPressed: () => updateTripState('driver_en_route'),
                          child: const Text('EN ROUTE'),
                        ),
                        OutlinedButton(
                          onPressed: () => updateTripState('arrived'),
                          child: const Text('ARRIVED'),
                        ),
                        FilledButton(
                          onPressed: () => updateTripState('in_trip'),
                          child: const Text('START'),
                        ),
                        FilledButton.tonal(
                          onPressed: () => updateTripState('completed'),
                          child: const Text('COMPLETE'),
                        ),
                      ],
                    ),
                  ],
                ),
              ),
            ),
          ],
        ],
      ),
    );
  }
}

class MapBox extends StatelessWidget {
  const MapBox({super.key});

  @override
  Widget build(BuildContext context) {
    return Container(
      height: 190,
      decoration: BoxDecoration(
        color: const Color(0xFFE5E5DF),
        borderRadius: BorderRadius.circular(18),
      ),
      child: const Center(
        child: Column(
          mainAxisSize: MainAxisSize.min,
          children: [
            Icon(Icons.map_outlined, size: 54),
            SizedBox(height: 8),
            Text('OpenStreetMap layer — next UI phase'),
          ],
        ),
      ),
    );
  }
}

class BookingCard extends StatelessWidget {
  final Map<String, dynamic> booking;

  const BookingCard(this.booking, {super.key});

  @override
  Widget build(BuildContext context) {
    final status = (booking['status'] ?? 'searching').toString();
    final driver = booking['driver_id'];
    return Card(
      child: ListTile(
        leading: CircleAvatar(
          child: Icon(status == 'assigned' ? Icons.check : Icons.hourglass_top),
        ),
        title: Text(
          status.toUpperCase(),
          style: const TextStyle(fontWeight: FontWeight.w800),
        ),
        subtitle: Text(
          '${booking['pickup']} → ${booking['destination']}\n'
          '${driver == null ? 'Searching for driver…' : 'Driver: $driver'}',
        ),
      ),
    );
  }
}
