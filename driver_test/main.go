package main

import (
	"errors"
	"fmt"
	"log"
	"os"
	"runtime/debug"

	"github.com/mdlayher/genetlink"
	"github.com/mdlayher/netlink"
)

func main() {

	defer func() {
		if r := recover(); r != nil {
			fmt.Println("stacktrace from panic: \n" + string(debug.Stack()))
		}
	}()

	// Create a new generic netlink socket, and automatically connect
	// to the NETLINK_GENERIC protocol.
	c, err := genetlink.Dial(nil)
	if err != nil {
		panic(err)
	}
	defer c.Close()

	families, err := c.ListFamilies()
	if err != nil {
		panic(err)
	}
	println("Families:")
	for _, family := range families {
		println("\t", family.Name)
	}

	const (
		name = "CREMONA"

		CREMONA_CMD_CONNECT = 1

		CREMONA_ATTR_NAME   = 1
		CREMONA_ATTR_RESULT = 2
		CREMONA_ATTR_DATA   = 3
	)

	// Get the family for the "nlctrl" controller.
	family, err := c.GetFamily(name)
	if err != nil {
		if errors.Is(err, os.ErrNotExist) {
			log.Printf("%q family not available", name)
			return
		}

		log.Fatalf("failed to query for family: %v", err)
	}

	attrEnc := netlink.NewAttributeEncoder()
	attrEnc.String(CREMONA_ATTR_NAME, "driver_test")
	data, err := attrEnc.Encode()
	if err != nil {
		log.Fatalf("failed to encode attributes: %v", err)
	}

	println("create netlink request")
	req := genetlink.Message{
		Header: genetlink.Header{
			Command: CREMONA_CMD_CONNECT,
			Version: family.Version,
		},
		Data: data,
	}

	println("execute netlink request")
	msgs, err := c.Execute(req, family.ID, netlink.Request)
	if err != nil {
		log.Fatalf("failed to execute: %v", err)
	}

	println("%return msg len:", len(msgs))

	println("return header:")
	println("\tcommand:", msgs[0].Header.Command)
	println("\tversion:", msgs[0].Header.Version)

	println("return raw data:")
	for index, b := range msgs[0].Data {
		fmt.Printf("\tdata\t%d:%x\n", index, b)
	}

	println("create netlink decoder")
	ad, err := netlink.NewAttributeDecoder(msgs[0].Data)
	if err != nil {
		log.Fatalf("failed to decode attributes: %v", err)
	}

	println("return message:")
	for ad.Next() {
		println("\ttype:", ad.Type())
		println("\tlen:", ad.Len())
		for index, b := range ad.Bytes() {
			fmt.Printf("\tdata\t%d:%x\n", index, b)
		}
	}

	ad, err = netlink.NewAttributeDecoder(msgs[0].Data)
	if err != nil {
		log.Fatalf("failed to decode attributes: %v", err)
	}
	ad.Next()
	if ad.Type() != CREMONA_ATTR_RESULT {
		log.Fatalf("failed to find result")
	}
	result := ad.Uint8()
	if result != 0 {
		log.Fatalf("failed to connect")
	}

	println("success")
}
