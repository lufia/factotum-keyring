package main

import (
	"log"

	"github.com/zalando/go-keyring"
)

func main() {
	service := "my-app"
	user := "anon"
	password := "secret"

	err := keyring.Set(service, user, password)
	if err != nil {
		log.Fatal("Set:", err)
	}
	secret, err := keyring.Get(service, user)
	if err != nil {
		log.Fatal("Get:", err)
	}

	log.Println(secret)
}
