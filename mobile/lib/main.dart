import 'package:flutter/material.dart';

void main() => runApp(const JayalakshmiDriveApp());

class JayalakshmiDriveApp extends StatelessWidget {
  const JayalakshmiDriveApp({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'Jayalakshmi DRIVE',
      debugShowCheckedModeBanner: false,
      theme: ThemeData(
        useMaterial3: true,
        colorSchemeSeed: const Color(0xFF8B1E3F),
        scaffoldBackgroundColor: const Color(0xFFF7F7F5),
      ),
      home: const RolePickerScreen(),
    );
  }
}

class RolePickerScreen extends StatelessWidget {
  const RolePickerScreen({super.key});

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      body: SafeArea(
        child: Padding(
          padding: const EdgeInsets.all(24),
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              const Spacer(),
              Text('JAYALAKSHMI', style: Theme.of(context).textTheme.labelLarge?.copyWith(letterSpacing: 2)),
              const SizedBox(height: 8),
              Text('DRIVE', style: Theme.of(context).textTheme.displaySmall?.copyWith(fontWeight: FontWeight.w800)),
              const SizedBox(height: 12),
              Text('Move people. Move business.', style: Theme.of(context).textTheme.titleMedium),
              const SizedBox(height: 40),
              _RoleCard(title: 'Book a ride', subtitle: 'Employee / passenger experience', icon: Icons.location_on_outlined, onTap: () => Navigator.push(context, MaterialPageRoute(builder: (_) => const RiderHomeScreen()))),
              const SizedBox(height: 16),
              _RoleCard(title: 'Drive', subtitle: 'Driver experience', icon: Icons.directions_car_outlined, onTap: () => Navigator.push(context, MaterialPageRoute(builder: (_) => const DriverHomeScreen()))),
              const Spacer(),
              Center(child: Text('Phase 02 • UI prototype', style: Theme.of(context).textTheme.bodySmall)),
            ],
          ),
        ),
      ),
    );
  }
}

class _RoleCard extends StatelessWidget {
  final String title;
  final String subtitle;
  final IconData icon;
  final VoidCallback onTap;
  const _RoleCard({required this.title, required this.subtitle, required this.icon, required this.onTap});

  @override
  Widget build(BuildContext context) {
    return Card(
      clipBehavior: Clip.antiAlias,
      child: InkWell(
        onTap: onTap,
        child: Padding(
          padding: const EdgeInsets.all(20),
          child: Row(children: [
            CircleAvatar(radius: 25, child: Icon(icon)),
            const SizedBox(width: 16),
            Expanded(child: Column(crossAxisAlignment: CrossAxisAlignment.start, children: [
              Text(title, style: Theme.of(context).textTheme.titleLarge?.copyWith(fontWeight: FontWeight.w700)),
              const SizedBox(height: 4),
              Text(subtitle),
            ])),
            const Icon(Icons.arrow_forward_ios_rounded, size: 18),
          ]),
        ),
      ),
    );
  }
}

class RiderHomeScreen extends StatelessWidget {
  const RiderHomeScreen({super.key});

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(title: const Text('Jayalakshmi DRIVE')),
      body: Column(children: [
        Expanded(child: Container(color: const Color(0xFFE8E8E3), child: const Center(child: Icon(Icons.map_outlined, size: 72)))),
        Material(
          elevation: 8,
          borderRadius: const BorderRadius.vertical(top: Radius.circular(24)),
          child: Padding(
            padding: const EdgeInsets.fromLTRB(20, 20, 20, 28),
            child: Column(crossAxisAlignment: CrossAxisAlignment.start, children: [
              Text('Where are you going?', style: Theme.of(context).textTheme.headlineSmall?.copyWith(fontWeight: FontWeight.w700)),
              const SizedBox(height: 16),
              TextField(decoration: InputDecoration(prefixIcon: const Icon(Icons.search), hintText: 'Enter destination', filled: true, border: OutlineInputBorder(borderRadius: BorderRadius.circular(14), borderSide: BorderSide.none))),
              const SizedBox(height: 12),
              Row(children: [
                Expanded(child: FilledButton.icon(onPressed: () {}, icon: const Icon(Icons.flash_on_outlined), label: const Text('RIDE NOW'))),
                const SizedBox(width: 10),
                Expanded(child: OutlinedButton.icon(onPressed: () {}, icon: const Icon(Icons.schedule_outlined), label: const Text('SCHEDULE'))),
              ]),
            ]),
          ),
        ),
      ]),
    );
  }
}

class DriverHomeScreen extends StatelessWidget {
  const DriverHomeScreen({super.key});

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(title: const Text('Driver')),
      body: ListView(padding: const EdgeInsets.all(20), children: [
        Card(child: Padding(padding: const EdgeInsets.all(20), child: Row(children: [
          const CircleAvatar(child: Icon(Icons.person)), const SizedBox(width: 14),
          const Expanded(child: Column(crossAxisAlignment: CrossAxisAlignment.start, children: [Text('Driver Name', style: TextStyle(fontWeight: FontWeight.w700)), Text('Vehicle • KL-XX-0000')])),
          Switch(value: true, onChanged: (_) {}),
        ]))),
        const SizedBox(height: 16),
        Text('Ready for trips', style: Theme.of(context).textTheme.headlineSmall?.copyWith(fontWeight: FontWeight.w700)),
        const SizedBox(height: 12),
        Card(child: ListTile(leading: const Icon(Icons.notifications_active_outlined), title: const Text('Incoming requests'), subtitle: const Text('No active requests'), trailing: const Icon(Icons.chevron_right))),
        const SizedBox(height: 12),
        Card(child: ListTile(leading: const Icon(Icons.calendar_today_outlined), title: const Text('Upcoming trips'), subtitle: const Text('No scheduled trips'), trailing: const Icon(Icons.chevron_right))),
      ]),
    );
  }
}
