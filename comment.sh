#!/bin/bash

convert -comment "coucou" Full_00003_00002_00007_00005_00009.bmp toto.png
convert toto.png -format "%c" info:

