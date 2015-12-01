import sys
import os

file_list = [
	"*.aux",
	"*.fdb*",
	"*.log",
	"*.out",
	"*.synctex*",
    "*.fls"
]

map( lambda x: os.system( "rm " + x ), file_list )
