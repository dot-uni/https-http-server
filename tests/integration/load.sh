#!/usr/bin/env zsh

i=0
while (( i < "$1" )); do 
    (
    printf "GET /uni HTTP/1.1\r\n"
    printf "Host: example.com\r\n"
    printf "Connection: close\r\n\r\n"
    ) | openssl s_client -connect localhost:8080 -quiet
    ((i++))
done
