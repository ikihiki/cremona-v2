package main

import (
	"bufio"
	"fmt"
	"log"
	"os"
	"time"

	"github.com/ikihiki/cremona/userland/internal/cremona"
)

func Senario1() {
	log.Println("Senario1 start <=======================================")
	defer log.Println("Senario1 end <=========================================")

	cremona, err := cremona.Connect("senario1")
	if err != nil {
		log.Fatalf("failed to connect: %v", err)
	}
	defer cremona.Close()

	pid := os.Getpid()
	log.Printf("pid: %v", pid)





	name, err := readFirstLine(fmt.Sprintf("/sys/kernel/cremona/repeaters/%d/name", pid))
	if err != nil {
		log.Fatalf("failed to read name: %v", err)
	}
	log.Printf("name: %v", name)
	if name != "senario1" {
		log.Fatalf("name is not senario1")
	}

	if(!Exists("/dev/senario1")) {
		log.Fatalf("device file is not exist")
	}

	log.Println("disconnect")
	err = cremona.Disconnect()
	if err != nil {
		log.Fatalf("failed to disconnect: %v", err)
	}

	time.Sleep(1 * time.Second) 

	if(Exists("/dev/senario1")) {
		log.Fatalf("device file is exist")
	}

	log.Println("Senario1 success <=====================================")
}

func readFirstLine(path string) (string, error) {
	f, err := os.Open(path)
	if err != nil {
		return "", err
	}

	defer f.Close()

	r := bufio.NewReader(f)
	line, _, err := r.ReadLine()
	if err != nil {
		return "", err
	}

	return string(line), nil
}

func Exists(filename string) bool {
    _, err := os.Stat(filename)
    return err == nil
}