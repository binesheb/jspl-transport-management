import 'package:flutter_test/flutter_test.dart';
import 'package:jayalakshmi_drive/main.dart';

void main() {
  testWidgets('field PoC role picker renders manager and driver', (tester) async {
    await tester.pumpWidget(const App());

    expect(find.text('JAYALAKSHMI'), findsOneWidget);
    expect(find.text('DRIVE'), findsOneWidget);
    expect(find.text('Manager / Booker'), findsOneWidget);
    expect(find.text('Driver'), findsOneWidget);
  });
}
