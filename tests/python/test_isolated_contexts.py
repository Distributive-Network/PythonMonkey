import pythonmonkey as pm
import unittest

class TestIsolatedContexts(unittest.TestCase):

    def test_create_context(self):
        context = pm.context()
        self.assertIsNotNone(context)

    def test_eval_in_context(self):
        context = pm.context()
        result = pm.evalInContext(context, "x = 4; x")
        self.assertEqual(result, 4)

    def test_isolation_between_contexts(self):
        context1 = pm.context()
        context2 = pm.context()

        pm.evalInContext(context1, "x = 4")
        pm.evalInContext(context2, "x = 5")

        result1 = pm.evalInContext(context1, "x")
        result2 = pm.evalInContext(context2, "x")

        self.assertEqual(result1, 4)
        self.assertEqual(result2, 5)

    def test_cleanup_contexts(self):
        context1 = pm.context()
        context2 = pm.context()

        pm.evalInContext(context1, "x = 4")
        pm.evalInContext(context2, "x = 5")

        pm.cleanupContexts()

        with self.assertRaises(ValueError):
            pm.evalInContext(context1, "x")

        with self.assertRaises(ValueError):
            pm.evalInContext(context2, "x")

    def test_super_global_context(self):
        context1 = pm.context()
        context2 = pm.context()

        pm.evalInContext(context1, "globalThis.sharedVar = 10")
        result = pm.evalInContext(context2, "globalThis.sharedVar")

        self.assertEqual(result, 10)

if __name__ == '__main__':
    unittest.main()
