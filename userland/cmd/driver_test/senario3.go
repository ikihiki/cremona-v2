package main

import (
	"log"
	"os"
	"sync"
	"time"

	"github.com/ikihiki/cremona/userland/internal/cremona"
)

func Senario3() {
	log.Println("Senario3 start <=======================================")
	defer log.Println("Senario3 end <=========================================")

	crmna, err := cremona.Connect("senario3")
	if err != nil {
		log.Fatalf("failed to connect: %v", err)
	}
	defer crmna.Close()

	pid := os.Getpid()
	log.Printf("pid: %v", pid)

	var wg sync.WaitGroup
	wg.Add(1)
	go waitNotify(crmna, &wg)

	time.Sleep(1 * time.Second)

	err = os.WriteFile("/dev/senario3", []byte("hello"), 0644)
	if err != nil {
		log.Fatalf("failed to write: %v", err)
	}

	done := make(chan struct{})
	go func() {
		wg.Wait()
		close(done)
	}()

	select {
	case <-done:
		log.Println("success to wait notify")
	case <-time.After(500 * time.Millisecond):
		log.Fatal("failed to wait notify")
	}

	log.Println("disconnect")
	err = crmna.Disconnect()
	if err != nil {
		log.Fatalf("failed to disconnect: %v", err)
	}

	log.Println("Senario3 success <=====================================")
}

func waitNotify(crmna *cremona.Cremona, wg *sync.WaitGroup) {
	err := crmna.WaitNotify()
	if err != nil {
		log.Fatalf("failed to wait notify: %v", err)
	}
	wg.Done()
}
