package main

import (
	"fmt"
	"log"
	"path"
	"path/filepath"
)

func testNotExistCommand(testDir string) (string, bool, string, float64) {
	testName := "testNotExistCommand"
	testMark := 1.0
	log.Printf("[%s] Testing non-existent command, %.2f\n", testName, testMark)

	mark := 0.0

	cmdList := [][]string{
		{path.Join(testDir), "nonexistent", "\n"},
	}

	cmdDir, _ := filepath.Abs(path.Join(testDir))
	for _, cmd := range cmdList {
		if _, err := execCmdWithCustomStartupDirSubcommandInput(false, cmd[0], cmdDir, ssiExecName, cmd[1], cmd[2]); err != nil {
			errMsg := fmt.Sprintf("ssi failed to execute: %s", err)
			log.Printf("[%s] %s\n", testName, errMsg)
		} else {
			mark = shouldAwardMark(mark, testMark/float64(len(cmdList)))
		}
	}
	return testName, true, "", mark
}

func testExec(testDir string) (string, bool, string, float64) {
	testName := "testExec"
	testMark := 1.0
	log.Printf("[%s] Testing exec commands without parameters, %.2f\n", testName, testMark)

	setupTestDirectories(testDir)

	cmdList := [][]string{
		{"ls", "", ""},
		{"whoami", "", ""},
		{"uname", "", ""},
		{"date", "", ""},
		{"hostname", "", ""},
	}

	mark := 0.0
	cmdDir, _ := filepath.Abs(path.Join(testDir))

	for _, cmd := range cmdList {
		if _, err := execMultipleSubcommands(cmdDir, ssiExecName, [][]string{cmd}); err != nil {
			errMsg := fmt.Sprintf("ssi failed to execute: %s", err)
			log.Printf("[%s] %s\n", testName, errMsg)
		} else {
			mark = shouldAwardMark(mark, testMark/float64(len(cmdList)))
		}
	}

	return testName, true, "", mark
}

func testExecParameters(testDir string) (string, bool, string, float64) {
	testName := "testExecWithParameters"
	testMark := 1.0
	log.Printf("[%s] Testing exec with parameters, %.2f\n", testName, testMark)

	setupTestDirectories(testDir)
	mark := 0.0

	cmdList := [][]string{
		{path.Join(testDir), "ls", "temp"},
		{path.Join(testDir), "ls", "-alh"},
		{path.Join(testDir), "ls", "temp/A/C"},
		{path.Join(testDir), "cat", "temp/A/C/AC.txt"},
		{path.Join(testDir), "echo", "hello"},
	}

	cmdDir, _ := filepath.Abs(path.Join(testDir))
	for _, cmd := range cmdList {
		if _, err := execCmdWithCustomStartupDirSubcommandInput(false, cmd[0], cmdDir, ssiExecName, cmd[1], cmd[2]); err != nil {
			errMsg := fmt.Sprintf("ssi failed to execute: %s", err)
			log.Printf("[%s] %s\n", testName, errMsg)
		} else {
			mark = shouldAwardMark(mark, testMark/float64(len(cmdList)))
		}
	}

	return testName, true, "", mark
}

func testExecLongRunning(testDir string) (string, bool, string, float64) {
	testName := "testExecLongRunning"
	testMark := 1.0
	log.Printf("[%s] Testing exec with long running command, %.2f\n", testName, testMark)
	mark := 0.0

	cmd := "ping"
	args := []string{"-c", "10", "1.1.1.1"}
	if _, err := execCmdSubcommandInputOutputPipeSignal(path.Join(testDir), ssiExecName, cmd, args...); err != nil {
		errMsg := fmt.Sprintf("ssi failed to execute: %s", err)
		log.Printf("[%s] %s\n", testName, errMsg)
	} else {
		mark = shouldAwardMark(mark, testMark)
	}

	return testName, true, "", mark
}
