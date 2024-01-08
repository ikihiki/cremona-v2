package main

import (
	"context"

	"github.com/mattn/go-mastodon"
)


type Client struct {
	config Config
	client *mastodon.Client
}

func NewMastdonCilent(config Config) (*Client, error) {
	client := mastodon.NewClient(&mastodon.Config{
		Server:       config.Server,
		ClientID:     config.ClientId,
		ClientSecret: config.ClientSecret,
	})

	err := client.Authenticate(context.Background(), config.Username, config.Password)
	if err != nil {
		return nil, err
	}
	return &Client{config: config, client: client}, nil
}

func (this *Client) SendToot(status string) error {
	_, err := this.client.PostStatus(context.Background(), &mastodon.Toot{
		Status:     status,
		Visibility: "public",
	})
	return err
}
