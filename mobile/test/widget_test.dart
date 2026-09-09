import 'package:flutter_test/flutter_test.dart';
import 'package:jayalakshmi_drive/main.dart';

void main() {
  testWidgets('role picker renders rider and driver choices', (tester) async {
    await tester.pumpWidget(const JayalakshmiDriveApp());

    expect(find.text('JAYALAKSHMI'), findsOneWidget);
    expect(find.text('DRIVE'), findsOneWidget);
    expect(find.text('Book a ride'), findsOneWidget);
    expect(find.text('Drive'), findsOneWidget);
  });
}
