package main

import (
	"crypto/sha256"
	"crypto/subtle"
	"encoding/hex"

	"golang.org/x/crypto/pbkdf2"
)

// HashToken return the hashable salt
func HashToken(token, salt string) string {
	tempHash := pbkdf2.Key([]byte(token), []byte(salt), 10000, 50, sha256.New)
	return hex.EncodeToString(tempHash)
}

func main() {
	token := "fb1c430539c2d47e0b5991639e838b4367cab6a0"

	token_hash := "0bd29e825b675b6d7748146407033caa99104bce113e1c9350f91a08472f6544405f236a819258abe0180f3a445ac8b6fcbb"
	salt := "5sOZ3Vfngn"

	hash := HashToken(token, salt)

	if subtle.ConstantTimeCompare([]byte(token_hash), []byte(hash)) == 1 {
		println("token valid")
	}
}
