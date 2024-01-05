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
	readMeesages("return ", msgs)
	ad, err := netlink.NewAttributeDecoder(msgs[0].Data)
	if err != nil {
		log.Fatalf("failed to decode attributes: %v", err)
	}
	ad.Next()
	if ad.Type() != CREMONA_ATTR_RESULT {
		log.Fatalf("failed to find result")
	}
	result := ad.Uint32()
	if result != 0 {
		log.Fatalf("failed to connect")
	}

	for {
		readNotify(c)
		readBuffer(c, family)
		for moveNext(c, family) {
			readBuffer(c, family)
		}
	}

	println("success")
}

func readNotify(c *genetlink.Conn) {
	println("read notify")

	for {
		msgs, _, err := c.Receive()
		if err != nil {
			log.Fatalf("failed to receive: %v", err)
		}
		readMeesages("notify ", msgs)
		for _, msg := range msgs {
			ad, err := netlink.NewAttributeDecoder(msg.Data)
			if err != nil {
				log.Fatalf("failed to decode attributes: %v", err)
			}
			ad.Next()
			if ad.Type() == CREMONA_CMD_NOTIFY_ADD_DATA {
				return
			}
		}
	}
}

func readBuffer(c *genetlink.Conn, family genetlink.Family) {
	attrEnc := netlink.NewAttributeEncoder()
	data, err := attrEnc.Encode()
	if err != nil {
		log.Fatalf("failed to encode attributes: %v", err)
	}

	println("create netlink request")
	req := genetlink.Message{
		Header: genetlink.Header{
			Command: CREMONA_CMD_READ_BUFFER,
			Version: family.Version,
		},
		Data: data,
	}

	println("execute netlink request")
	msgs, err := c.Execute(req, family.ID, netlink.Request)
	if err != nil {
		log.Fatalf("failed to execute: %v", err)
	}
	readMeesages("return ", msgs)
	ad, err := netlink.NewAttributeDecoder(msgs[0].Data)
	if err != nil {
		log.Fatalf("failed to decode attributes: %v", err)
	}

	for ad.Next() {
		switch ad.Type() {
		case CREMONA_ATTR_RESULT:
			println("read buffer result:", ad.Uint32())
		case CREMONA_ATTR_TOOT_ID:
			println("read buffer toot id:", ad.Uint32())
		case CREMONA_ATTR_DATA:
			println("read buffer data:", string(ad.Bytes()))
		case CREMONA_ATTR_CMD:
			switch ad.Uint8() {
			case CREMONA_REPERTER_DATA_TYPE_TOOT_CREATE:
				println("read buffer data type: toot create")
			case CREMONA_REPERTER_DATA_TYPE_TOOT_ADD_STRING:
				println("read buffer data type: toot add string")
			case CREMONA_REPERTER_DATA_TYPE_TOOT_SEND:
				println("read buffer data type: toot send")
			}
		}
	}
}

func moveNext(c *genetlink.Conn, family genetlink.Family) bool {
	attrEnc := netlink.NewAttributeEncoder()
	data, err := attrEnc.Encode()
	if err != nil {
		log.Fatalf("failed to encode attributes: %v", err)
	}

	println("create netlink request")
	req := genetlink.Message{
		Header: genetlink.Header{
			Command: CREMONA_CMD_MOVE_NEXT,
			Version: family.Version,
		},
		Data: data,
	}

	println("execute netlink request")
	msgs, err := c.Execute(req, family.ID, netlink.Request)
	if err != nil {
		log.Fatalf("failed to execute: %v", err)
	}
	readMeesages("return ", msgs)
	ad, err := netlink.NewAttributeDecoder(msgs[0].Data)
	if err != nil {
		log.Fatalf("failed to decode attributes: %v", err)
	}

	ad.Next()
	if ad.Type() != CREMONA_ATTR_RESULT {
		log.Fatalf("failed to find result")
	}
	result := ad.Uint32()
	if result < 0 {
		log.Fatalf("failed to connect")
	}
	println("move next result:", result)

	return result > 0
}

func readMeesages(prefix string, msgs []genetlink.Message) {
	println(prefix, "message length:", len(msgs))

	for index, msg := range msgs {
		println(prefix, "message header [", index, "]:")
		println("\tcommand:", msg.Header.Command)
		println("\tversion:", msg.Header.Version)

		println(prefix, "message raw data [", index, "]:")
		for index, b := range msg.Data {
			fmt.Printf("\tdata\t%d:%x\n", index, b)
		}

		println("create netlink decoder")
		ad, err := netlink.NewAttributeDecoder(msgs[0].Data)
		if err != nil {
			log.Fatalf("failed to decode attributes: %v", err)
		}

		println(prefix, "message data [", index, "]:")
		for ad.Next() {
			println("\ttype:", ad.Type())
			println("\tlen:", ad.Len())
			for index, b := range ad.Bytes() {
				fmt.Printf("\tdata\t%d:%x\n", index, b)
			}
		}
	}
}
