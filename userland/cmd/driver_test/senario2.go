package main

import (
	"log"
	"os"
	"time"

	"github.com/ikihiki/cremona/userland/internal/cremona"
)

func Senario2() {
	log.Println("Senario2 start <=======================================")
	defer log.Println("Senario2 end <=========================================")

	crmna, err := cremona.Connect("senario2")
	if err != nil {
		log.Fatalf("failed to connect: %v", err)
	}
	defer crmna.Close()

	pid := os.Getpid()
	log.Printf("pid: %v", pid)

	time.Sleep(1 * time.Second)

	err = os.WriteFile("/dev/senario2", []byte("hello"), 0644)
	if err != nil {
		log.Fatalf("failed to write: %v", err)
	}

	time.Sleep(1 * time.Second)

	// add toot
	log.Println("add toot")
	cmd, err := crmna.ReadData()
	if err != nil {
		log.Fatalf("failed to receive command: %v", err)
	}
	log.Printf("cmd: %v", cmd)
	if cmd.Type != cremona.CREMONA_COMMAND_TYPE_CREATE_TOOT {
		log.Fatalf("invalid command type: %v", cmd.Type)
	}
	toot_id := cmd.TootId

	res, err := crmna.MoveNext()
	if err != nil {
		log.Fatalf("failed to move next: %v", err)
	}
	if !res {
		log.Fatalf("invalid result: %v", res)
	}

	// add string
	log.Println("add string")
	cmd, err = crmna.ReadData()
	if err != nil {
		log.Fatalf("failed to receive command: %v", err)
	}
	log.Printf("cmd: %v", cmd)
	if cmd.Type != cremona.CREMONA_COMMAND_TYPE_ADD_STRING_TOOT {
		log.Fatalf("invalid command type: %v", cmd.Type)
	}

	if string(cmd.Data) != "hello" {
		log.Fatalf("invalid data: %v str: %s", cmd.Data, string(cmd.Data))
	}
	log.Printf("str: %s", string(cmd.Data))

	if cmd.TootId != toot_id {
		log.Fatalf("invalid toot_id: %v", cmd.TootId)
	}

	res, err = crmna.MoveNext()
	if err != nil {
		log.Fatalf("failed to move next: %v", err)
	}
	if !res {
		log.Fatalf("invalid result: %v", res)
	}

	// send toot
	log.Println("send toot")
	cmd, err = crmna.ReadData()
	if err != nil {
		log.Fatalf("failed to receive command: %v", err)
	}
	log.Printf("cmd: %v", cmd)
	if cmd.Type != cremona.CREMONA_COMMAND_TYPE_SEND_TOOT {
		log.Fatalf("invalid command type: %v", cmd.Type)
	}

	if cmd.TootId != toot_id {
		log.Fatalf("invalid toot_id: %v", cmd.TootId)
	}

	res, err = crmna.MoveNext()
	if err != nil {
		log.Fatalf("failed to move next: %v", err)
	}
	if res {
		log.Fatalf("invalid result: %v", res)
	}

	log.Println("disconnect")
	err = crmna.Disconnect()
	if err != nil {
		log.Fatalf("failed to disconnect: %v", err)
	}

	log.Println("Senario2 success <=====================================")
}
