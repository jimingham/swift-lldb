# TestExclusivitySuppression.py
#
# This source file is part of the Swift.org open source project
#
# Copyright (c) 2014 - 2016 Apple Inc. and the Swift project authors
# Licensed under Apache License v2.0 with Runtime Library Exception
#
# See https://swift.org/LICENSE.txt for license information
# See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
#
# ------------------------------------------------------------------------------
"""
Test suppression of dynamic exclusivity enforcement
"""
import commands
import lldb
from lldbsuite.test.lldbtest import *
import lldbsuite.test.decorators as decorators
import lldbsuite.test.lldbutil as lldbutil
import os
import unittest2

def execute_command(command):
    (exit_status, output) = commands.getstatusoutput(command)
    return exit_status

class TestExclusivitySuppression(TestBase):

    mydir = TestBase.compute_mydir(__file__)

    # Test that we can evaluate w.s.i at Breakpoint 1 without triggering
    # a failure due to exclusivity
    @decorators.swiftTest
    @decorators.add_test_categories(["swiftpr"])
    def test_basic_exclusivity_suppression(self):
        """Test that exclusively owned values can still be accessed"""

        self.buildAll()

        target = self.create_target()

        # Set the breakpoints
        bp1 = target.BreakpointCreateBySourceRegex('Breakpoint 1',
                                                   self.main_source_spec)
        self.assertTrue(bp1.GetNumLocations() > 0, VALID_BREAKPOINT)

        # Launch the process, and do not stop at the entry point.
        process = target.LaunchSimple(None, None, os.getcwd())

        self.assertTrue(process, PROCESS_IS_VALID)

        # Frame #0 should be at our breakpoint.
        threads = lldbutil.get_threads_stopped_at_breakpoint(process, bp1)

        self.assertTrue(len(threads) == 1)
        thread = threads[0]
        frame = thread.frames[0]
        self.assertTrue(frame, "Frame 0 is valid.")

        self.check_expression(frame, "w.s.i", "8", use_summary=False)

    # Test that we properly handle nested expression evaluations by:
    # (1) Breaking at breakpoint 1
    # (2) Running 'expr get()' (which will hit breakpoint 2)
    # (3) Evaluating i at breakpoint 2 (this is a nested evaluation)
    # (4) Continuing the evaluation of 'expr get()' to return to bp 1
    # (5) Evaluating w.s.i again to check that finishing the nested expression
    #     did not prematurely re-enable exclusivity checks.
    @decorators.swiftTest
    @decorators.add_test_categories(["swiftpr"])
    def test_exclusivity_suppression_for_concurrent_expressions(self):
        """Test that exclusivity suppression works with concurrent expressions"""
        self.buildAll()

        target = self.create_target()

        bp1 = target.BreakpointCreateBySourceRegex('Breakpoint 1',
                                                   self.main_source_spec)
        self.assertTrue(bp1.GetNumLocations() > 0, VALID_BREAKPOINT)

        bp2 = target.BreakpointCreateBySourceRegex('Breakpoint 2',
                                                   self.main_source_spec)
        self.assertTrue(bp2.GetNumLocations() > 0, VALID_BREAKPOINT)

        # Launch the process, and do not stop at the entry point.
        process = target.LaunchSimple(None, None, os.getcwd())

        self.assertTrue(process, PROCESS_IS_VALID)

        # Break at Breakpoint 1, then evaluate 'get()' to hit breakpoint 2.
        threads = lldbutil.get_threads_stopped_at_breakpoint(process, bp1)

        self.assertTrue(len(threads) == 1)
        thread = threads[0]

        opts = lldb.SBExpressionOptions()
        opts.SetIgnoreBreakpoints(False)
        thread.frame[0].EvaluateExpression('get()', opts)

        # Evaluate w.s.i at breakpoint 2 to check that exclusivity checking
        # is suppressed inside the nested expression
        self.check_expression(thread.frames[0], "i", "8", use_summary=False)

        # Return to breakpoint 1 and evaluate w.s.i again to check that
        # exclusivity checking is still suppressed
        self.dbg.HandleCommand('thread ret -x')
        self.check_expression(thread.frame[0], "w.s.i", "8", use_summary=False)

    def setUp(self):
        TestBase.setUp(self)
        self.main_source = "main.swift"
        self.main_source_spec = lldb.SBFileSpec(self.main_source)

    def buildAll(self):
        execute_command("make everything")
        def cleanup():
            execute_command("make cleanup")

        self.addTearDownHook(cleanup)

    def check_expression(self, frame, expression, expected_result, use_summary=True):
        value = frame.EvaluateExpression(expression)
        self.assertTrue(value.IsValid(), expression + " returned a valid value")
        if self.TraceOn():
            print value.GetSummary()
            print value.GetValue()
        if use_summary:
            answer = value.GetSummary()
        else:
            answer = value.GetValue()
        report_str = "%s expected: %s got: %s" % (
            expression, expected_result, answer)
        self.assertTrue(answer == expected_result, report_str)

    def create_target(self):
        exe_name = "main"
        exe = os.path.join(os.getcwd(), exe_name)

        # Create the target
        target = self.dbg.CreateTarget(exe)
        self.assertTrue(target, VALID_TARGET)

        return target

if __name__ == '__main__':
    import atexit
    lldb.SBDebugger.Initialize()
    atexit.register(lldb.SBDebugger.Terminate)
    unittest2.main()
