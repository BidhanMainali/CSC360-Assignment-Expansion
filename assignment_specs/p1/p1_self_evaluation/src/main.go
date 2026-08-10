package main

import (
	"flag"
	"fmt"
	"io"
	"log"
	"os"
	"time"
)

var TAMarking = false
var markingOutputDir = "marking_result"

func main() {
	var vnumber *string
	var testDir string
	var logfileName string

	if TAMarking {
		vnumber = flag.String("v", "", "Student V#")
	} else {
		log.Println("CSC360 Fall 2025 | P1 Self Evaluation Tool")
	}

	flag.Parse()

	if TAMarking {
		if *vnumber == "" {
			log.Println("Please provide the student V# (e.g., -v 00001234)")
			return
		} else {
			log.Printf("Student V#: %s\n\n", *vnumber)
			testDir = fmt.Sprintf("V%s", *vnumber)
		}
		os.MkdirAll(fmt.Sprintf("%s/V%s", markingOutputDir, *vnumber), 0755)
		logfileName = fmt.Sprintf("%s/V%s/V%s.log", markingOutputDir, *vnumber, *vnumber)
	} else {
		testDir = "."
		logfileName = "self-marking.log"
	}

	logFile, err := os.OpenFile(logfileName, os.O_CREATE|os.O_WRONLY, 0666)
	if err != nil {
		log.Println("Failed to open log file")
		return
	}
	defer logFile.Close()
	log.SetOutput(io.MultiWriter(logFile, os.Stdout))

	if !checkDirectory(testDir) {
		log.Printf("Test directory %s does not exist", testDir)
		return
	}

	grade := 0.0

	tests := []func(string) (string, bool, string, float64){
		testMake,
		testREADME,
		testExecSSI,
		testExec,
		testExecParameters,
		testNotExistCommand,
		testExecLongRunning,
		testPwd,
		testChangeRelativeDir,
		testChangeAbsDir,
		testChangeHomeDir,
		testPromptCorrect,
		testBgPing,
	}

	for _, test := range tests {
		if name, succ, msg, score := test(testDir); succ {
			grade += score
			log.Printf("==== Test name: %s, Mark received: %.2f ====\n\n", name, score)
		} else {
			if name == "testMake" && msg == "Makefile not found" {
				log.Fatalf("Makefile not found. Exiting marking\n")
				break
			}
			log.Printf("==== Test name: %s, Mark received: %.2f ====\nMessage: %s\n\n", name, score, msg)
		}
		time.Sleep(1 * time.Second)
	}

	log.Printf("\n==== Final Grade: %f====\n", grade)

	cleanup(testDir)
}
