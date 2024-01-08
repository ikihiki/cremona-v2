package main

import (
	"flag"
	"fmt"
	"log"
	"runtime/debug"

	"github.com/ikihiki/cremona/userland/internal/cremona"
)

func main() {
	defer func() {
		if r := recover(); r != nil {
			fmt.Println("stacktrace from panic: \n" + string(debug.Stack()))
		}
	}()

	f := flag.Bool("gen_config", false, "only generate config file")
	flag.Parse()
	if *f {
		createConfigFile()
		return
	}

	config := readConfig()
	mastdon, err := NewMastdonCilent(config)
	if err != nil {
		log.Panicf("failed to connect mastdon: %v\n", err)
	}

	tootMap := make(map[uint32]string)
	crmna, err := cremona.Connect(config.DeviceName)
	if err != nil {
		log.Panicf("failed to connect: %v\n", err)
	}
	defer crmna.Close()

	for {

		for {
			data, err := crmna.ReadData()
			if err != nil {
				log.Panicf("failed to receive command: %v\n", err)
			}
			switch data.Type {
			case cremona.CREMONA_COMMAND_TYPE_CREATE_TOOT:
				tootMap[data.TootId] = ""
			case cremona.CREMONA_COMMAND_TYPE_ADD_STRING_TOOT:
				tootMap[data.TootId] += string(data.Data)
			case cremona.CREMONA_COMMAND_TYPE_SEND_TOOT:
				err := mastdon.SendToot(tootMap[data.TootId])
				if err != nil {
					log.Printf("failed to send toot: %v\n", err)
				}
			}

			res, err := crmna.MoveNext()
			if err != nil {
				log.Panicf("failed to move next: %v\n", err)
			}
			if !res {
				break
			}
		}

		err = crmna.WaitNotify()
		if err != nil {
			log.Panicf("failed to wait notify: %v\n", err)
		}
	}
}
