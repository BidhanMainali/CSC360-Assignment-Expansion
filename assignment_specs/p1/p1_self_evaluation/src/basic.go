package main

import (
	"fmt"
	"log"
	"os"
	"path"
	"path/filepath"
)

var ssiNames = []string{"ssi", "SSI"}
var ssiExecName string

func testREADME(testDir string) (string, bool, string, float64) {
	testName := "testREADME"
	testMark := 0.25
	log.Printf("[%s] Testing README file exists, %.2f\n", testName, testMark)

	READMENames := []string{"README.txt", "readme.txt", "README.md", "readme.md", "readme", "README"}

	mark := 0.0
	for _, name := range READMENames {
		if _, err := os.Stat(fmt.Sprintf("%s/%s", testDir, name)); err == nil {
			if content, err := os.ReadFile(fmt.Sprintf("%s/%s", testDir, name)); err == nil {
				log.Printf("[%s] README content: \n%s\n", testName, string(content))
			}
			mark = shouldAwardMark(mark, testMark)
			return testName, true, "", mark
		}
	}
	return testName, false, "README file not found", 0
}

func testMake(testDir string) (string, bool, string, float64) {
	testName := "testMake"
	testMark := 0.25

	mark := 0.0
	log.Printf("[%s] Testing Makefile exists and can compile ssi, %.2f\n", testName, testMark)

	makefileNames := []string{"Makefile", "makefile", "GNUmakefile"}

	for _, name := range makefileNames {
		if _, err := os.Stat(fmt.Sprintf("%s/%s", testDir, name)); err == nil {
			makeClean(testDir)
			if out, err := execCmd("make", "-C", testDir); err != nil {
				errMsg := fmt.Sprintf("Makefile failed to execute: \n%s", out)
				log.Printf("[%s] %s\n", testName, errMsg)
				mark = shouldAwardMark(mark, testMark)
				return testName, true, "", mark
			} else {
				for _, ssiName := range ssiNames {
					if _, err := os.Stat(fmt.Sprintf("%s/%s", testDir, ssiName)); err == nil {
						ssiExecName = ssiName
						return testName, true, "", testMark
					}
				}
				return testName, false, "ssi executable not found", 0
			}
		}
	}

	return testName, false, "Makefile not found", 0
}

func testExecSSI(testDir string) (string, bool, string, float64) {
	testName := "testExecSSI"
	testMark := 0.5

	log.Printf("[%s] Testing your ssi can be executed without additional parameter, shows the correct prompt, and exit on EOF, %.2f\n", testName, testMark)

	cmdList := [][]string{
		{""},
	}
	mark := 0.0
	cmdDir, _ := filepath.Abs(path.Join(testDir))

	if _, err := execMultipleSubcommands(cmdDir, ssiExecName, cmdList); err != nil {
		errMsg := fmt.Sprintf("ssi failed to execute: %s", err)
		log.Printf("[%s] %s\n", testName, errMsg)
	} else {
		mark = shouldAwardMark(mark, testMark)
	}

	return testName, true, "", mark
}

func makeClean(testDir string) {
	if _, err := execCmd("make", "-C", testDir, "clean"); err != nil {
		for _, ssiName := range ssiNames {
			os.Remove(fmt.Sprintf("%s/%s", testDir, ssiName))
		}
	}
}
