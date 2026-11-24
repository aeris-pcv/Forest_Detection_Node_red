import unittest
from twin.twin import Twin

class TwinTests(unittest.TestCase):
    def test_safe_warning_alarm_recovery(self):
        twin = Twin()
        self.assertEqual(twin.sample(27, 65, 4095, False, now=0)["state"], "SAFE")
        self.assertEqual(twin.sample(39, 25, 4095, False, now=1)["state"], "WARNING")
        self.assertEqual(twin.sample(39, 25, 1200, True, now=2)["state"], "ALARM")
        self.assertEqual(twin.sample(27, 65, 4095, False, now=3)["state"], "ALARM")
        self.assertEqual(twin.sample(27, 65, 4095, False, now=6)["state"], "SAFE")

    def test_remote_test_and_mute(self):
        twin = Twin()
        class Message: payload = b"TEST_ON"
        twin.command(None, None, Message())
        self.assertEqual(twin.sample(27, 65, 4095, False, now=10)["state"], "ALARM")
        Message.payload = b"MUTE"
        twin.command(None, None, Message())
        self.assertTrue(twin.sample(27, 65, 4095, False, now=11)["muted"])

if __name__ == "__main__": unittest.main()
