package cremona

import (
	"errors"
	"fmt"
	"log"
	"os"
	"time"

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

	CREMONA_COMMAND_TYPE_CREATE_TOOT     = 0
	CREMONA_COMMAND_TYPE_ADD_STRING_TOOT = 1
	CREMONA_COMMAND_TYPE_SEND_TOOT       = 2
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

	family, err := c.GetFamily(familyName)
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

	msgs, err := cremona.execute(req, cremona.family.ID, netlink.Request)
	if err != nil {
		log.Printf("failed to execute: %v", err)
		return err
	}

	for _, msg := range msgs {
		if msg.Header.Command != CREMONA_CMD_DISSCONNECT {
			continue
		}

		ad, err := netlink.NewAttributeDecoder(msg.Data)
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
	return errors.New("failed to find command")
}

func (cremona *Cremona) Close() error {
	err := cremona.link.Close()
	if err != nil {
		return err
	}
	return nil
}

func (cremona *Cremona) WaitNotify() error {
	isRecived := false
	for {
		log.Println("start wait notify")

		done := make(chan error)
		go func() {
			for {
				msgs, _, err := cremona.link.Receive()
				if err != nil {
					log.Printf("failed to receive: %v", err)
					done <- err
				}
				for _, msg := range msgs {
					ad, err := netlink.NewAttributeDecoder(msg.Data)
					if err != nil {
						log.Printf("failed to decode attributes: %v", err)
						done <- err
					}
					ad.Next()
					if ad.Type() == CREMONA_CMD_NOTIFY_ADD_DATA {
						close(done)
						return
					}
				}
			}
		}()

		select {
		case err, ok := <-done:
			if ok {
				log.Printf("error: %v\n", err)
				return err
			} else {
				isRecived = true
				log.Println("recived notify")
				return nil
			}
		case <-time.After(5000 * time.Millisecond):
			log.Println("wait notify timeout")
			if isRecived {
				log.Println("return")
				return nil
			}
		}
	}
}

type CremonaCommand struct {
	Type   uint32
	TootId uint32
	Data   []byte
}

func (cremona *Cremona) ReadData() (*CremonaCommand, error) {
	enc := netlink.NewAttributeEncoder()
	data, err := enc.Encode()
	if err != nil {
		log.Printf("failed to encode attributes: %v", err)
		return nil, err
	}
	req := genetlink.Message{
		Header: genetlink.Header{
			Command: CREMONA_CMD_READ_BUFFER,
			Version: cremona.family.Version,
		},
		Data: data,
	}

	msgs, err := cremona.execute(req, cremona.family.ID, netlink.Request)
	if err != nil {
		log.Printf("failed to receive: %v", err)
		return nil, err
	}
	for _, msg := range msgs {
		if msg.Header.Command != CREMONA_CMD_READ_BUFFER {
			continue
		}

		ad, err := netlink.NewAttributeDecoder(msg.Data)
		if err != nil {
			log.Printf("failed to decode attributes: %v", err)
			return nil, err
		}
		cmd := &CremonaCommand{}
		for ad.Next() {
			switch ad.Type() {
			case CREMONA_ATTR_CMD:
				cmd.Type = uint32(ad.Uint8())
			case CREMONA_ATTR_TOOT_ID:
				cmd.TootId = ad.Uint32()
			case CREMONA_ATTR_DATA:
				cmd.Data = ad.Bytes()
			}
		}
		return cmd, nil
	}
	return nil, errors.New("failed to find command")
}

func (cremona *Cremona) MoveNext() (bool, error) {
	enc := netlink.NewAttributeEncoder()
	data, err := enc.Encode()
	if err != nil {
		log.Printf("failed to encode attributes: %v", err)
		return false, err
	}

	req := genetlink.Message{
		Header: genetlink.Header{
			Command: CREMONA_CMD_MOVE_NEXT,
			Version: cremona.family.Version,
		},
		Data: data,
	}

	msgs, err := cremona.execute(req, cremona.family.ID, netlink.Request)
	if err != nil {
		log.Printf("failed to execute: %v", err)
		return false, err
	}

	for _, msg := range msgs {
		if msg.Header.Command != CREMONA_CMD_MOVE_NEXT {
			continue
		}

		ad, err := netlink.NewAttributeDecoder(msg.Data)
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

		return result > 0, nil
	}
	return false, errors.New("failed to find command")
}

// func readMeesages(prefix string, msgs []genetlink.Message) {
// 	println(prefix, "message length:", len(msgs))

// 	for index, msg := range msgs {
// 		println(prefix, "message header [", index, "]:")
// 		println("\tcommand:", msg.Header.Command)
// 		println("\tversion:", msg.Header.Version)

// 		println(prefix, "message raw data [", index, "]:")
// 		for index, b := range msg.Data {
// 			fmt.Printf("\tdata\t%d:%x\n", index, b)
// 		}

// 		println("create netlink decoder")
// 		ad, err := netlink.NewAttributeDecoder(msgs[0].Data)
// 		if err != nil {
// 			log.Fatalf("failed to decode attributes: %v", err)
// 		}

// 		println(prefix, "message data [", index, "]:")
// 		for ad.Next() {
// 			println("\ttype:", ad.Type())
// 			println("\tlen:", ad.Len())
// 			for index, b := range ad.Bytes() {
// 				fmt.Printf("\tdata\t%d:%x\n", index, b)
// 			}
// 		}
// 	}
// }

func (cremona *Cremona) execute(m genetlink.Message, family uint16, flags netlink.HeaderFlags) ([]genetlink.Message, error) {
	request, err := cremona.link.Send(m, family, flags)
	if err != nil {
		log.Println("failed to send")
		return nil, err
	}

	for {
		msgs, replies, err := cremona.link.Receive()

		if err != nil {
			log.Println("failed to receive")
			return nil, err
		}
		for index, reply := range replies {
			msg := msgs[index]
			if msg.Header.Command != m.Header.Command {
				continue
			}

			// Check for mismatched sequence, unless:
			//   - request had no sequence, meaning we are probably validating
			//     a multicast reply
			if reply.Header.Sequence != request.Header.Sequence && request.Header.Sequence != 0 {
				if msg.Header.Command != CREMONA_CMD_NOTIFY_ADD_DATA {
					return nil, fmt.Errorf("validate: mismatched sequence in netlink reply. response sequence: %v request sequence: %v", reply.Header.Sequence, request.Header.Sequence)
				}
			}

			if reply.Header.PID != request.Header.PID && request.Header.PID != 0 && reply.Header.PID != 0 {
				return nil, fmt.Errorf("validate: mismatched PID in netlink reply. response pid: %v request pid: %v", reply.Header.PID, request.Header.PID)
			}

			for _, v := range msgs {
				log.Printf("msg: %v", v)
			}

			return msgs, nil
		}
	}

}
