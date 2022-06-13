#!/usr/bin/env python3
import unittest

if __name__ == "__main__":
    tests = unittest.defaultTestLoader.discover(".")
    unittest.TextTestRunner(buffer=True).run(tests)
