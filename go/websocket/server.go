package main

import (
	"fmt"
	"github.com/go-chi/chi/v5"
	"github.com/go-chi/chi/v5/middleware"
	"github.com/olahol/melody"
	"net/http"
)

func main() {
	r := chi.NewRouter()
	m := melody.New()

	r.Use(middleware.RequestID)
	r.Use(middleware.RealIP)
	r.Use(middleware.Logger)
	r.Use(middleware.Recoverer)

	r.Get("/", func(w http.ResponseWriter, r *http.Request) {
		http.ServeFile(w, r, "index.html")
	})

	r.Get("/ws", func(w http.ResponseWriter, r *http.Request) {
		fmt.Println("connection requested, upgrading")
		m.HandleRequest(w, r)
	})

	m.HandleConnect(func(session *melody.Session) {
		fmt.Println("connected")
	})

	m.HandleDisconnect(func(session *melody.Session) {
		fmt.Println("disconnected")
	})

	m.HandleMessage(func(s *melody.Session, msg []byte) {
		m.Broadcast(msg)
	})

	http.ListenAndServe(":3000", r)
}
