#!/bin/bash

gcc -o von_neumann von_neumann.c
gcc -o von_neumann_kam von_neumann_kam.c -lssl -lcrypto
gcc -o amls amls.c
