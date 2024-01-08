package main

import (
	"fmt"
	"log"
	"runtime/debug"
	"time"
)

func main() {

	defer func() {
		if r := recover(); r != nil {
			fmt.Println("stacktrace from panic: \n" + string(debug.Stack()))
		}
	}()

	log.Println("Test start <============================================================")
	defer log.Println("Test end <==============================================================")

	Senario1()
	time.Sleep(1 * time.Second)
	Senario2()
	time.Sleep(1 * time.Second)
	Senario3()

	log.Println("Test success <==========================================================")
}
