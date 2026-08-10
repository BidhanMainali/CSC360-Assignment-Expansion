package main

import (
	"context"
	"fmt"
	"io"
	"log"
	"os"
	"os/exec"
	"path"
	"strconv"
	"strings"
	"time"
)

func checkDirectory(dir string) bool {
	if _, err := os.Stat(dir); os.IsNotExist(err) {
		return false
	}
	return true
}

func execCmd(cmd string, args ...string) (string, error) {
	command := exec.Command(cmd, args...)

	log.Printf("[COMMAND]: %s\n", command.String())
	if out, err := command.CombinedOutput(); err != nil {
		return string(out), err
	} else {
		// fmt.Print(string(out))
		log.Print(string(out))
		return string(out), nil
	}
}

func execCmdWithCustomStartupDirSubcommandInput(pwd bool, dir string, cmdDir string, cmd string, subcommand string, args ...string) (string, error) {
	command := exec.Command(fmt.Sprintf("%s/%s", cmdDir, cmd))
	command.Dir = dir

	subcommandArgs := append([]string{subcommand}, args...)
	_cmd := strings.Join(subcommandArgs, " ")
	if pwd {
		_cmd += "\npwd\n"
	} else {
		_cmd += "\n"
	}
	command.Stdin = strings.NewReader(_cmd)

	log.Printf("Sending command to SSI: \n%s\n", _cmd)
	var stdout, stderr strings.Builder
	command.Stdout = &stdout
	command.Stderr = &stderr

	err := command.Run()

	if stdout.Len() > 0 {
		// fmt.Println(stdout.String())
		log.Println(stdout.String())
	}
	if stderr.Len() > 0 {
		// fmt.Println(stderr.String())
		log.Println(stderr.String())
	}

	out := stdout.String() + stderr.String()
	if err != nil {
		return out, err
	}
	return out, nil
}

func execMultipleSubcommands(cmdDir string, cmd string, cmdList [][]string, timeout ...int) (string, error) {
	var _timeout int
	if len(timeout) > 0 {
		_timeout = timeout[0]
	} else {
		_timeout = 1
	}

	ctx, cancel := context.WithTimeout(context.Background(), time.Duration(_timeout)*time.Second)
	defer cancel()
	command := exec.CommandContext(ctx, fmt.Sprintf("%s/%s", cmdDir, cmd))
	command.Dir = cmdDir

	// Create a pipe to write to the child process's Stdin
	stdinPipe, err := command.StdinPipe()
	if err != nil {
		return fmt.Sprintf("Error creating Stdin pipe: %v\n", err), err
	}

	// Capture the output
	stdoutPipe, err := command.StdoutPipe()
	if err != nil {
		return fmt.Sprintf("Error creating Stdout pipe: %v\n", err), err
	}

	// Capture stderr
	// stderrPipe, err := command.StderrPipe()
	// if err != nil {
	// return fmt.Sprintf("Error creating Stderr pipe: %v\n", err), err
	// }

	log.Printf("[COMMAND]: %s\n", command.String())
	// Start the process
	if err := command.Start(); err != nil {
		return fmt.Sprintf("Error starting command: %v\n", err), err
	}

	outputChan := make(chan string)

	go func() {
		var output strings.Builder
		// var outputStderr strings.Builder
		buf := make([]byte, 1024)
		for {
			n, err := stdoutPipe.Read(buf)
			if n > 0 {
				output.Write(buf[:n])
			}
			if err != nil {
				if err == io.EOF {
					break
				}
				log.Print("Error reading stdout:", err)
				break
			}

			// n, err = stderrPipe.Read(buf)
			// if n > 0 {
			// 	outputStderr.Write(buf[:n])
			// }
			// if err != nil {
			// 	if err == io.EOF {
			// 		break
			// 	}
			// 	log.Print("Error reading stderr:", err)
			// }
		}
		outputChan <- output.String()
		close(outputChan)
	}()

	// Send each subcommand to the child process via Stdin
	for _, cmd := range cmdList {
		subcommandArgs := cmd
		subcommand := strings.Join(subcommandArgs, " ")

		if subcommand != "" {
			log.Printf("Sending command to SSI: %s\n", subcommand)
			_, err = io.WriteString(stdinPipe, subcommand+"\n")
			if err != nil {
				log.Print(err)
			}
		}
	}

	// _, err = io.WriteString(stdinPipe, "exit\n")
	// if err != nil {
	// 	log.Print(err)
	// }
	err = stdinPipe.Close()
	if err != nil {
		log.Print("Error closing stdin:", err)
	}

	err = command.Wait()
	if err != nil {
		log.Print("Error waiting for process to exit:", err)
	}

	output := <-outputChan
	// fmt.Println(output)
	log.Println(output)
	// flush stdout

	return output, nil
}

func execCmdSubcommandInputOutputPipeSignal(dir string, cmd string, subcommand string, args ...string) (string, error) {
	ctx, cancel := context.WithTimeout(context.Background(), time.Duration(10)*time.Second)
	defer cancel()
	command := exec.CommandContext(ctx, fmt.Sprintf("./%s", cmd))
	command.Dir = dir

	subcommandArgs := append([]string{subcommand}, args...)

	_cmd := strings.Join(subcommandArgs, " ")
	// _cmd += "\n"

	command.Stdin = strings.NewReader(_cmd)

	out, err := command.StdoutPipe()
	if err != nil {
		return "", err
	}

	log.Printf("[COMMAND]: %s\n", command.String())
	if err := command.Start(); err != nil {
		return "", err
	}

	// wait 5 seconds
	log.Println("Waiting 5 seconds before sending Ctrl-C signal")
	time.Sleep(5 * time.Second)

	// send signal
	log.Println("Sending interrupt signal Ctrl-C")
	if err := command.Process.Signal(os.Interrupt); err != nil {
		return "", err
	}

	buf := make([]byte, 1024)
	for {
		n, err := out.Read(buf)
		if err != nil {
			break
		}
		// fmt.Print(string(buf[:n]))
		log.Print(string(buf[:n]))
	}

	if err := command.Wait(); err != nil {
		return "", err
	}

	return "", nil
}

func shouldAwardMark(currentMark float64, mark float64) float64 {
	var input string
	log.Printf("Do you want to award mark for this test? (y/n or input a floating number between [0, %.2f]): ", mark)
	fmt.Scanln(&input)
	log.Printf("Mark received: %s", input)
	if input == "y" {
		return currentMark + mark
	} else if input == "n" {
		return currentMark
	} else {
		if m, err := strconv.ParseFloat(input, 64); err == nil {
			return currentMark + m
		}
	}
	return currentMark
}

func setupTestDirectories(testDir string) {
	tempDir := path.Join(testDir, "temp")
	dirs := []string{
		path.Join(tempDir, "A"),
		path.Join(tempDir, "A/C"),
		path.Join(tempDir, "A/C/D"),
		path.Join(tempDir, "B"),
		path.Join(tempDir, "B/E"),
	}
	for _, d := range dirs {
		os.MkdirAll(d, 0755)
	}

	files := []string{
		path.Join(tempDir, "A/A.txt"),
		path.Join(tempDir, "A/C/AC.txt"),
		path.Join(tempDir, "A/C/D/ACD.txt"),
		path.Join(tempDir, "B/B.txt"),
		path.Join(tempDir, "B/E/BE.txt"),
	}
	for _, f := range files {
		os.Create(f)
		// populate contents with the file name
		str := "This is the content for " + path.Base(f) + "\n"
		os.WriteFile(f, []byte(str), 0644)
	}
}

func cleanup(testDir string) {
	tempDir := path.Join(testDir, "temp")
	os.RemoveAll(tempDir)
}
