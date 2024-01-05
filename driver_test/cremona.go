package main

import (
	"errors"
	"log"
	"os"

	"github.com/mdlayher/genetlink"
	"github.com/mdlayher/netlink"
)

const (
	familyName = "CREMONA"

	CREMONA_CMD_CONNECT         = 1
	CREMONA_CMD_NOTIFY_ADD_DATA = 2
	CREMONA_CMD_READ_BUFFER     = 3
	CREMONA_CMD_MOVE_NEXT       = 4
	CREMONA_CMD_DISSCONNECT     = 5

	CREMONA_ATTR_NAME    = 1
	CREMONA_ATTR_RESULT  = 2
	CREMONA_ATTR_CMD     = 3
	CREMONA_ATTR_TOOT_ID = 4
	CREMONA_ATTR_DATA    = 5

	CREMONA_REPERTER_DATA_TYPE_TOOT_CREATE     = 0
	CREMONA_REPERTER_DATA_TYPE_TOOT_ADD_STRING = 1
	CREMONA_REPERTER_DATA_TYPE_TOOT_SEND       = 2
)

type Cremona struct {
	link   *genetlink.Conn
	family genetlink.Family
}

func Connect(name string) (*Cremona, error) {
	c, err := genetlink.Dial(nil)
	if err != nil {
		return nil, err
	}

	family, err := c.GetFamily(name)
	if err != nil {
		if errors.Is(err, os.ErrNotExist) {
			log.Printf("%q family not available", familyName)
		}

		log.Printf("failed to query for family: %v", err)
		return nil, err
	}

	enq := netlink.NewAttributeEncoder()
	enq.String(CREMONA_ATTR_NAME, name)
	data, err := enq.Encode()
	if err != nil {
		c.Close()
		return nil, err
	}

	req := genetlink.Message{
		Header: genetlink.Header{
			Command: CREMONA_CMD_CONNECT,
			Version: family.Version,
		},
		Data: data,
	}

	msgs, err := c.Execute(req, family.ID, netlink.Request)
	if err != nil {
		log.Printf("failed to execute: %v", err)
		return nil, err
	}

	ad, err := netlink.NewAttributeDecoder(msgs[0].Data)
	if err != nil {
		log.Printf("failed to decode attributes: %v", err)
		return nil, err
	}
	ad.Next()
	if ad.Type() != CREMONA_ATTR_RESULT {
		log.Printf("failed to find result")
		return nil, errors.New("failed to find result")
	}
	result := ad.Uint32()
	if result != 0 {
		log.Printf("failed to connect %v", result)
		return nil, errors.New("failed to find result")
	}

	return &Cremona{link: c, family: family}, nil
}

func (cremona *Cremona) Disconnect() error {
	enq := netlink.NewAttributeEncoder()
	data, err := enq.Encode()
	if err != nil {
		return err
	}

	req := genetlink.Message{
		Header: genetlink.Header{
			Command: CREMONA_CMD_DISSCONNECT,
			Version: cremona.family.Version,
		},
		Data: data,
	}

	msgs, err := cremona.link.Execute(req, cremona.family.ID, netlink.Request)
	if err != nil {
		log.Printf("failed to execute: %v", err)
		return err
	}

	ad, err := netlink.NewAttributeDecoder(msgs[0].Data)
	if err != nil {
		log.Printf("failed to decode attributes: %v", err)
		return err
	}
	ad.Next()
	if ad.Type() != CREMONA_ATTR_RESULT {
		log.Printf("failed to find result")
		return errors.New("failed to find result")
	}
	result := ad.Uint32()
	if result != 0 {
		log.Printf("failed to connect %v", result)
		return errors.New("failed to connec")
	}

	return nil
}

func (cremona *Cremona) Close() error {
	err := cremona.link.Close()
	if err != nil {
		return err
	}
	return nil
}
