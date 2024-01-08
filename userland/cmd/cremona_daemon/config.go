package main

import (
	"encoding/json"
	"fmt"
	"log"
	"os"
)

type Config struct {
	Server       string `yaml:"server"`
	ClientId     string `yaml:"client_id"`
	ClientSecret string `yaml:"client_secret"`
	Username     string `yaml:"username"`
	Password     string `yaml:"password"`
	DeviceName   string `yaml:"device_name"`
}

func getConfigFilePath() string {
	config_dir := os.Getenv("XDG_CONFIG_HOME")
	if config_dir == "" {
		config_dir = os.Getenv("HOME")
	}

	if config_dir == "" {
		config_dir = "/etc"
	}
	config_file := config_dir + "/cremona.json"
	return config_file
}

func readConfig() Config {
	config_file := getConfigFilePath()

	bytes, err := os.ReadFile(config_file)
	if err != nil {
		if os.IsNotExist(err) {
			createConfigFile()
		}

		log.Fatal(err)
	}

	var config Config
	if err := json.Unmarshal(bytes, &config); err != nil {
		log.Fatal(err)
	}
	return config
}

func createConfigFile() {
	config_file := getConfigFilePath()
	if _, err := os.Stat(config_file); err == nil {
		return
	}
	json, _ := json.Marshal(Config{
		DeviceName:   "device name",
		Server:       "mastdon server url",
		ClientId:     "api client id",
		ClientSecret: "api client secret",
		Username:     "username",
		Password:     "password",
	})
	fmt.Printf("create config file: %v\n", config_file)
	os.WriteFile(config_file, json, 0666)
}
