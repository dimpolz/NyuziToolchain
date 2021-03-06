"""
Test that the lldb-mi driver understands MI command syntax.
"""

import lldbmi_testcase
from lldbtest import *
import unittest2

class MiSyntaxTestCase(lldbmi_testcase.MiTestCaseBase):

    @lldbmi_test
    @expectedFailureWindows("llvm.org/pr22274: need a pexpect replacement for windows")
    def test_lldbmi_tokens(self):
        """Test that 'lldb-mi --interpreter' echos command tokens."""

        self.spawnLldbMi(args = None)

        # Load executable
        self.runCmd("000-file-exec-and-symbols %s" % self.myexe)
        self.expect("000\^done")

        # Run to main
        self.runCmd("100000001-break-insert -f b_MyFunction")
        self.expect("100000001\^done,bkpt={number=\"1\"")
        self.runCmd("2-exec-run")
        self.expect("2\^running")
        self.expect("\*stopped,reason=\"breakpoint-hit\"")

        # Exit
        self.runCmd("0000000000000000000003-exec-continue")
        self.expect("0000000000000000000003\^running")
        self.expect("\*stopped,reason=\"exited-normally\"")

    @lldbmi_test
    @expectedFailureWindows("llvm.org/pr22274: need a pexpect replacement for windows")
    @unittest2.skip("lldb-mi doesn't handle special chars properly")
    def test_lldbmi_specialchars(self):
        """Test that 'lldb-mi --interpreter' handles complicated strings."""

        self.spawnLldbMi(args = None)

        # Create alias for myexe
        complicated_myexe = "C--mpl-x file's`s @#$%^&*()_+-={}[]| name"
        if os.path.exists(complicated_myexe):
            os.unlink(complicated_myexe)
        os.symlink(self.myexe, complicated_myexe)

        try:
            # Try to load executable with complicated filename
            self.runCmd("-file-exec-and-symbols \"%s\"" % complicated_myexe)
            self.expect("\^done")

            # Check that it was loaded correctly
            self.runCmd("-break-insert -f a_MyFunction")
            self.expect("\^done,bkpt={number=\"1\"")
            self.runCmd("-exec-run")
            self.expect("\^running")
            self.expect("\*stopped,reason=\"breakpoint-hit\"")

        finally:
            # Clean up
            os.unlink(complicated_myexe)

if __name__ == '__main__':
    unittest2.main()
