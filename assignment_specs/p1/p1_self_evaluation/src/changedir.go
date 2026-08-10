package main

import (
	"fmt"
	"log"
	"os"
	"path"
	"path/filepath"
)

func testPwd(testDir string) (string, bool, string, float64) {
	testName := "testPwd"
	testMark := 1.0
	log.Printf("[%s] Testing printing current working directory, %.2f\n", testName, testMark)

	mark := 0.0

	cmdList := [][]string{
		{path.Join(testDir), "pwd", ""},
	}
	cmdDir, _ := filepath.Abs(path.Join(testDir))
	for _, cmd := range cmdList {
		if _, err := execCmdWithCustomStartupDirSubcommandInput(false, cmd[0], cmdDir, ssiExecName, cmd[1], cmd[2]); err != nil {
			errMsg := fmt.Sprintf("ssi failed to execute: %s", err)
			log.Printf("[%s] %s\n", testName, errMsg)
		} else {
			cwd, _ := os.Getwd()
			cwd = path.Join(cwd, testDir)

			log.Printf("pwd should output: \n%s\n", cwd)
			mark = shouldAwardMark(mark, testMark)
		}
	}

	return testName, true, "", mark
}

func testChangeRelativeDir(testDir string) (string, bool, string, float64) {
	testName := "testChangeRelativeDir"
	testMark := 1.0
	log.Printf("[%s] Testing changing relative directory, %.2f\n", testName, testMark)

	setupTestDirectories(testDir)

	mark := 0.0

	cmdList := [][]string{
		{path.Join(testDir), "cd", "./temp"},
		// {path.Join(testDir, "temp"), "cd", "./A/"},
		{path.Join(testDir, "temp"), "cd", "./A"},
		{path.Join(testDir, "temp/A/C"), "cd", "../.."},
		{path.Join(testDir, "temp/A/C"), "cd", "./D/../.."},
	}

	cmdDir, _ := filepath.Abs(path.Join(testDir))
	for _, cmd := range cmdList {
		if _, err := execCmdWithCustomStartupDirSubcommandInput(true, cmd[0], cmdDir, ssiExecName, cmd[1], cmd[2]); err != nil {
			errMsg := fmt.Sprintf("ssi failed to execute: %s", err)
			log.Printf("[%s] %s\n", testName, errMsg)
		} else {
			mark = shouldAwardMark(mark, testMark/float64(len(cmdList)))
		}
	}

	return testName, true, "", mark
}

func testChangeAbsDir(testDir string) (string, bool, string, float64) {
	testName := "testChangeAbsDir"
	testMark := 1.0
	log.Printf("[%s] Testing changing absolute directory, %.2f\n", testName, testMark)

	setupTestDirectories(testDir)

	mark := 0.0

	cmdList := [][]string{
		{path.Join(testDir), "cd", "/tmp"},                // tmp
		{path.Join(testDir), "cd", "/tmp/../tmp/../tmp/"}, // tmp
		{path.Join(testDir), "cd", "/"},                   // /
		{path.Join(testDir), "cd", "/.././"},              // /
		{path.Join(testDir), "cd", "/tmp/nonexisit/"},     // cd: no such file or directory: /tmp/nonexisit/
	}

	cmdDir, _ := filepath.Abs(path.Join(testDir))
	for _, cmd := range cmdList {
		if _, err := execCmdWithCustomStartupDirSubcommandInput(true, cmd[0], cmdDir, ssiExecName, cmd[1], cmd[2]); err != nil {
			errMsg := fmt.Sprintf("ssi failed to execute: %s", err)
			log.Printf("[%s] %s\n", testName, errMsg)
		} else {
			mark = shouldAwardMark(mark, testMark/float64(len(cmdList)))
		}
	}

	return testName, true, "", mark
}

func testChangeHomeDir(testDir string) (string, bool, string, float64) {
	testName := "testChangeHomeDir"
	testMark := 1.0
	log.Printf("[%s] Testing changing home directory, %.2f\n", testName, testMark)

	setupTestDirectories(testDir)

	mark := 0.0

	cmdList := [][]string{
		{path.Join(testDir), "cd", "/tmp"},
		{path.Join(testDir), "cd", "~"},
		{path.Join(testDir), "cd", "~/"},
		{path.Join(testDir), "cd", "~/../../"},
		{path.Join(testDir), "cd", ""},
	}

	cmdDir, _ := filepath.Abs(path.Join(testDir))
	for _, cmd := range cmdList {
		if _, err := execCmdWithCustomStartupDirSubcommandInput(true, cmd[0], cmdDir, ssiExecName, cmd[1], cmd[2]); err != nil {
			errMsg := fmt.Sprintf("ssi failed to execute: %s", err)
			log.Printf("[%s] %s\n", testName, errMsg)
		} else {
			mark = shouldAwardMark(mark, testMark/float64(len(cmdList)))
		}
	}

	return testName, true, "", mark
}

func testPromptCorrect(testDir string) (string, bool, string, float64) {
	testName := "testPromptCorrect"
	testMark := 1.0

	log.Printf("[%s] Testing prompt is correct, %.2f\n", testName, testMark)

	mark := 0.0

	log.Println("Among all the above changing directory tests, does the prompt always show the correct cwd after each `cd` command?")
	mark = shouldAwardMark(mark, testMark)

	return testName, true, "", mark
}
