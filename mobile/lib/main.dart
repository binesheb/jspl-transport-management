import 'dart:async';
import 'dart:convert';

import 'package:flutter/material.dart';
import 'package:geolocator/geolocator.dart';
import 'package:http/http.dart' as http;
import 'package:vibration/vibration.dart';

const apiBase = String.fromEnvironment('DRIVE_API', defaultValue: 'http://10.0.2.2:8000');

class DriveApi {
  static Uri uri(String path) => Uri.parse('$apiBase$path');

  static Future<List<Map<String, dynamic>>> drivers() async {
    final r = await http.get(uri('/api/poc/drivers')).timeout(const Duration(seconds: 8));
    if (r.statusCode != 200) throw Exception('Driver list failed (${r.statusCode})');
    return List<Map<String, dynamic>>.from(jsonDecode(r.body));
  }

  static Future<Map<String, dynamic>> online(String id, String name, String vehicle) async {
    final r = await http.post(uri('/api/poc/drivers/online'), headers: {'content-type': 'application/json'}, body: jsonEncode({'driver_id': id, 'name': name, 'vehicle': vehicle})).timeout(const Duration(seconds: 8));
    if (r.statusCode != 200) throw Exception('Could not go online (${r.statusCode})');
    return Map<String, dynamic>.from(jsonDecode(r.body));
  }

  static Future<void> offline(String id) async {
    await http.post(uri('/api/poc/drivers/offline'), headers: {'content-type': 'application/json'}, body: jsonEncode({'driver_id': id})).timeout(const Duration(seconds: 8));
  }

  static Future<void> location(String id, Position p) async {
    await http.post(uri('/api/poc/drivers/location'), headers: {'content-type': 'application/json'}, body: jsonEncode({'driver_id': id, 'latitude': p.latitude, 'longitude': p.longitude, 'accuracy': p.accuracy, 'speed': p.speed, 'heading': p.heading})).timeout(const Duration(seconds: 8));
  }

  static Future<Map<String, dynamic>> book(String pickup, String destination, {String requestedFor = 'now'}) async {
    final r = await http.post(uri('/api/poc/bookings'), headers: {'content-type': 'application/json'}, body: jsonEncode({'pickup': pickup, 'destination': destination, 'requested_for': requestedFor})).timeout(const Duration(seconds: 8));
    if (r.statusCode != 201) throw Exception('Booking failed (${r.statusCode})');
    return Map<String, dynamic>.from(jsonDecode(r.body));
  }

  static Future<Map<String, dynamic>> offer(String bookingId, String driverId) async {
    final r = await http.post(uri('/api/poc/bookings/$bookingId/offer'), headers: {'content-type': 'application/json'}, body: jsonEncode({'driver_id': driverId})).timeout(const Duration(seconds: 8));
    if (r.statusCode != 200) throw Exception('Dispatch failed (${r.statusCode})');
    return Map<String, dynamic>.from(jsonDecode(r.body));
  }

  static Future<Map<String, dynamic>> accept(String bookingId, String driverId) async {
    final r = await http.post(uri('/api/poc/bookings/$bookingId/accept'), headers: {'content-type': 'application/json'}, body: jsonEncode({'driver_id': driverId})).timeout(const Duration(seconds: 8));
    if (r.statusCode != 200) throw Exception('Accept failed (${r.statusCode})');
    return Map<String, dynamic>.from(jsonDecode(r.body));
  }

  static Future<Map<String, dynamic>> decline(String bookingId, String driverId) async {
    final r = await http.post(uri('/api/poc/bookings/$bookingId/decline'), headers: {'content-type': 'application/json'}, body: jsonEncode({'driver_id': driverId})).timeout(const Duration(seconds: 8));
    return Map<String, dynamic>.from(jsonDecode(r.body));
  }

  static Future<List<Map<String, dynamic>>> bookings() async {
    final r = await http.get(uri('/api/poc/bookings')).timeout(const Duration(seconds: 8));
    if (r.statusCode != 200) throw Exception('Bookings failed (${r.statusCode})');
    return List<Map<String, dynamic>>.from(jsonDecode(r.body));
  }

  static Future<Map<String, dynamic>> status(String id) async {
    final r = await http.get(uri('/api/poc/bookings/$id')).timeout(const Duration(seconds: 8));
    if (r.statusCode != 200) throw Exception('Booking status failed (${r.statusCode})');
    return Map<String, dynamic>.from(jsonDecode(r.body));
  }

  static Future<void> updateStatus(String id, String status) async {
    await http.post(uri('/api/poc/bookings/$id/status'), headers: {'content-type': 'application/json'}, body: jsonEncode({'status': status})).timeout(const Duration(seconds: 8));
  }
}

void main() => runApp(const JayalakshmiDriveApp());

class JayalakshmiDriveApp extends StatelessWidget {
  const JayalakshmiDriveApp({super.key});
  @override
  Widget build(BuildContext context) => MaterialApp(title: 'Jayalakshmi DRIVE', debugShowCheckedModeBanner: false, theme: ThemeData(useMaterial3: true, colorSchemeSeed: const Color(0xFF8B1E3F), scaffoldBackgroundColor: const Color(0xFFF7F7F5)), home: const RolePickerScreen());
}

class RolePickerScreen extends StatelessWidget {
  const RolePickerScreen({super.key});
  @override
  Widget build(BuildContext context) => Scaffold(body: SafeArea(child: Padding(padding: const EdgeInsets.all(24), child: Column(crossAxisAlignment: CrossAxisAlignment.start, children: [const Spacer(), const Text('JAYALAKSHMI', style: TextStyle(fontWeight: FontWeight.w700, letterSpacing: 2)), const SizedBox(height: 6), const Text('DRIVE', style: TextStyle(fontSize: 42, fontWeight: FontWeight.w900)), const SizedBox(height: 10), const Text('Private transport, dispatched like a ride-hailing service.'), const SizedBox(height: 34), _RoleCard(title: 'Manager / Booker', subtitle: 'Request and follow a driver', icon: Icons.person_search_outlined, onTap: () => Navigator.push(context, MaterialPageRoute(builder: (_) => const ManagerScreen()))), const SizedBox(height: 14), _RoleCard(title: 'Driver', subtitle: 'Go online and receive trips', icon: Icons.directions_car_outlined, onTap: () => Navigator.push(context, MaterialPageRoute(builder: (_) => const DriverScreen()))), const Spacer(), Center(child: Text('Android field PoC • API $apiBase', style: Theme.of(context).textTheme.bodySmall)), ]))));
}

class _RoleCard extends StatelessWidget {
  final String title, subtitle;
  final IconData icon;
  final VoidCallback onTap;
  const _RoleCard({required this.title, required this.subtitle, required this.icon, required this.onTap});
  @override
  Widget build(BuildContext context) => Card(clipBehavior: Clip.antiAlias, child: InkWell(onTap: onTap, child: Padding(padding: const EdgeInsets.all(20), child: Row(children: [CircleAvatar(radius: 25, child: Icon(icon)), const SizedBox(width: 16), Expanded(child: Column(crossAxisAlignment: CrossAxisAlignment.start, children: [Text(title, style: const TextStyle(fontSize: 18, fontWeight: FontWeight.w700)), const SizedBox(height: 4), Text(subtitle)])), const Icon(Icons.arrow_forward_ios_rounded, size: 18)]))));
}

class ManagerScreen extends StatefulWidget { const ManagerScreen({super.key}); @override State<ManagerScreen> createState() => _ManagerScreenState(); }
class _ManagerScreenState extends State<ManagerScreen> {
  final destination = TextEditingController();
  final pickup = TextEditingController(text: 'Jayalakshmi MG Road');
  Timer? timer;
  String? bookingId;
  Map<String, dynamic>? booking;
  bool busy = false;
  @override void initState() { super.initState(); timer = Timer.periodic(const Duration(seconds: 2), (_) => refresh()); }
  @override void dispose() { timer?.cancel(); destination.dispose(); pickup.dispose(); super.dispose(); }
  Future<void> refresh() async { if (!mounted) return; try { if (bookingId != null) { final b = await DriveApi.status(bookingId!); if (mounted) setState(() => booking = b); } } catch (_) {} }
  Future<void> requestRide() async {
    if (destination.text.trim().isEmpty || busy) return;
    setState(() => busy = true);
    try {
      final b = await DriveApi.book(pickup.text.trim(), destination.text.trim());
      bookingId = b['booking_id'];
      final ds = await DriveApi.drivers();
      final available = ds.where((d) => d['online'] == true && d['status'] == 'available').toList();
      if (available.isNotEmpty) await DriveApi.offer(bookingId!, available.first['driver_id']);
      if (mounted) setState(() { booking = b; busy = false; });
      await refresh();
    } catch (e) { if (mounted) { setState(() => busy = false); ScaffoldMessenger.of(context).showSnackBar(SnackBar(content: Text('$e'))); } }
  }
  @override Widget build(BuildContext context) => Scaffold(appBar: AppBar(title: const Text('Manager / Booker')), body: ListView(padding: const EdgeInsets.all(18), children: [const _MapPlaceholder(), const SizedBox(height: 16), Text('Where are you going?', style: Theme.of(context).textTheme.headlineSmall?.copyWith(fontWeight: FontWeight.w800)), const SizedBox(height: 12), TextField(controller: pickup, decoration: const InputDecoration(prefixIcon: Icon(Icons.my_location), labelText: 'Pickup', border: OutlineInputBorder())), const SizedBox(height: 10), TextField(controller: destination, decoration: const InputDecoration(prefixIcon: Icon(Icons.search), labelText: 'Destination', hintText: 'Airport, showroom, hotel...', border: OutlineInputBorder())), const SizedBox(height: 12), FilledButton.icon(onPressed: busy ? null : requestRide, icon: const Icon(Icons.flash_on), label: Text(busy ? 'REQUESTING...' : 'RIDE NOW')), if (booking != null) ...[const SizedBox(height: 18), _BookingCard(booking: booking!)] ]));
}

class DriverScreen extends StatefulWidget { const DriverScreen({super.key}); @override State<DriverScreen> createState() => _DriverScreenState(); }
class _DriverScreenState extends State<DriverScreen> {
  final id = 'driver-demo-01';
  bool online = false, sending = false;
  Timer? pollTimer, locationTimer;
  Map<String, dynamic>? offer;
  String? activeBooking;
  Position? lastPosition;
  @override void initState() { super.initState(); pollTimer = Timer.periodic(const Duration(seconds: 2), (_) => poll()); }
  @override void dispose() { pollTimer?.cancel(); locationTimer?.cancel(); if (online) DriveApi.offline(id); super.dispose(); }
  Future<void> toggle() async {
    if (sending) return; setState(() => sending = true);
    try {
      if (!online) { await _ensureLocation(); await DriveApi.online(id, 'Driver Demo', 'Toyota Innova • KL-XX-0000'); setState(() => online = true); _startLocation(); }
      else { await DriveApi.offline(id); locationTimer?.cancel(); setState(() { online = false; offer = null; }); }
    } catch (e) { if (mounted) ScaffoldMessenger.of(context).showSnackBar(SnackBar(content: Text('$e'))); }
    if (mounted) setState(() => sending = false);
  }
  Future<void> _ensureLocation() async {
    if (!await Geolocator.isLocationServiceEnabled()) throw Exception('Turn on Location on the driver phone.');
    var p = await Geolocator.checkPermission();
    if (p == LocationPermission.denied) p = await Geolocator.requestPermission();
    if (p == LocationPermission.denied || p == LocationPermission.deniedForever) throw Exception('Location permission is required.');
  }
  void _startLocation() { locationTimer?.cancel(); locationTimer = Timer.periodic(const Duration(seconds: 5), (_) async { try { final p = await Geolocator.getCurrentPosition(); lastPosition = p; await DriveApi.location(id, p); if (mounted) setState(() {}); } catch (_) {} }); }
  Future<void> poll() async {
    if (!online) return;
    try {
      final bs = await DriveApi.bookings();
      final match = bs.where((b) => b['status'] == 'offered' && b['offered_to'] == id).toList();
      if (match.isNotEmpty && offer == null) { if (await Vibration.hasVibrator()) Vibration.vibrate(duration: 1200); if (mounted) setState(() => offer = match.first); }
    } catch (_) {}
  }
  Future<void> respond(bool accept) async {
    if (offer == null) return;
    try { final b = accept ? await DriveApi.accept(offer!['booking_id'], id) : await DriveApi.decline(offer!['booking_id'], id); if (mounted) setState(() { offer = null; activeBooking = accept ? b['booking_id'] : null; }); } catch (e) { if (mounted) ScaffoldMessenger.of(context).showSnackBar(SnackBar(content: Text('$e'))); }
  }
  Future<void> advance(String status) async { if (activeBooking == null) return; await DriveApi.updateStatus(activeBooking!, status); if (status == 'completed') setState(() => activeBooking = null); }
  @override Widget build(BuildContext context) => Scaffold(appBar: AppBar(title: const Text('Driver')), body: ListView(padding: const EdgeInsets.all(18), children: [Card(child: Padding(padding: const EdgeInsets.all(18), child: Row(children: [const CircleAvatar(radius: 28, child: Icon(Icons.person)), const SizedBox(width: 14), const Expanded(child: Column(crossAxisAlignment: CrossAxisAlignment.start, children: [Text('Driver Demo', style: TextStyle(fontWeight: FontWeight.w800, fontSize: 18)), Text('Toyota Innova • KL-XX-0000')])), Switch(value: online, onChanged: sending ? null : (_) => toggle())])), const SizedBox(height: 12), Card(child: ListTile(leading: Icon(online ? Icons.wifi : Icons.wifi_off), title: Text(online ? 'You are ONLINE' : 'You are OFFLINE'), subtitle: Text(lastPosition == null ? 'Location not yet reported' : 'GPS ${lastPosition!.latitude.toStringAsFixed(5)}, ${lastPosition!.longitude.toStringAsFixed(5)}'))), if (offer != null) ...[const SizedBox(height: 14), _IncomingOffer(offer: offer!, onAccept: () => respond(true), onDecline: () => respond(false))], if (activeBooking != null) ...[const SizedBox(height: 14), Card(child: Padding(padding: const EdgeInsets.all(18), child: Column(crossAxisAlignment: CrossAxisAlignment.start, children: [Text('ACTIVE TRIP', style: Theme.of(context).textTheme.labelLarge), const SizedBox(height: 8), Text('Booking $activeBooking', style: const TextStyle(fontWeight: FontWeight.w800)), const SizedBox(height: 14), Wrap(spacing: 8, runSpacing: 8, children: [OutlinedButton(onPressed: () => advance('driver_en_route'), child: const Text('EN ROUTE')), OutlinedButton(onPressed: () => advance('arrived'), child: const Text('ARRIVED')), FilledButton(onPressed: () => advance('in_trip'), child: const Text('START')), FilledButton.tonal(onPressed: () => advance('completed'), child: const Text('COMPLETE'))])]))]) ]));
}

class _MapPlaceholder extends StatelessWidget { const _MapPlaceholder(); @override Widget build(BuildContext context) => Container(height: 190, decoration: BoxDecoration(color: const Color(0xFFE5E5DF), borderRadius: BorderRadius.circular(18)), child: const Center(child: Column(mainAxisSize: MainAxisSize.min, children: [Icon(Icons.map_outlined, size: 54), SizedBox(height: 8), Text('OpenStreetMap map layer • Phase 2')]))); }
class _IncomingOffer extends StatelessWidget { final Map<String, dynamic> offer; final VoidCallback onAccept, onDecline; const _IncomingOffer({required this.offer, required this.onAccept, required this.onDecline}); @override Widget build(BuildContext context) => Card(color: Theme.of(context).colorScheme.primaryContainer, child: Padding(padding: const EdgeInsets.all(18), child: Column(crossAxisAlignment: CrossAxisAlignment.start, children: [Text('🔔 NEW TRIP', style: Theme.of(context).textTheme.titleLarge?.copyWith(fontWeight: FontWeight.w900)), const SizedBox(height: 12), Text(offer['destination'] ?? '', style: Theme.of(context).textTheme.headlineSmall?.copyWith(fontWeight: FontWeight.w800)), const SizedBox(height: 5), Text('Pickup: ${offer['pickup']}'), const SizedBox(height: 16), Row(children: [Expanded(child: OutlinedButton(onPressed: onDecline, child: const Text('DECLINE'))), const SizedBox(width: 10), Expanded(child: FilledButton(onPressed: onAccept, child: const Text('ACCEPT')))])]))); }
class _BookingCard extends StatelessWidget { final Map<String, dynamic> booking; const _BookingCard({required this.booking}); @override Widget build(BuildContext context) { final status = booking['status'] ?? 'searching'; return Card(child: ListTile(leading: CircleAvatar(child: Icon(status == 'assigned' ? Icons.check : Icons.hourglass_top)), title: Text(status.toString().toUpperCase(), style: const TextStyle(fontWeight: FontWeight.w800)), subtitle: Text('${booking['pickup']} → ${booking['destination']}\n${booking['driver_id'] == null ? 'Searching for driver…' : 'Driver: ${booking['driver_id']}'}'))); } }
