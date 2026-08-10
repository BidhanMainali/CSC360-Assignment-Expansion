package main

import (
	"fmt"
	"log"
	"path"
	"path/filepath"
)

func testBgPing(testDir string) (string, bool, string, float64) {
	testName := "testBgPing"
	testMark := 5.0
	log.Printf("[%s] Testing `bg` command with `ping`, %.2f\n", testName, testMark)
	log.Printf("Please wait for around 15 seconds")

	setupTestDirectories(testDir)

	mark := 0.0

	cmdList := [][]string{
		{"bg", "ping", "-c 5 1.1.1.1"},
		{"bg", "ping", "-c 10 8.8.8.8"},
		{"bglist", "", ""},
		{"sleep", "5", ""},
		{"bglist", "", ""},
		{"sleep", "5", ""},
		{"bglist", "", ""},
	}

	cmdDir, _ := filepath.Abs(path.Join(testDir))
	if _, err := execMultipleSubcommands(cmdDir, ssiExecName, cmdList, 20); err != nil {
		errMsg := fmt.Sprintf("ssi failed to execute: %s", err)
		log.Printf("[%s] %s\n", testName, errMsg)
	} else {
		mark = shouldAwardMark(mark, testMark)
	}

	return testName, true, "", mark
}
