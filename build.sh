#!/bin/bash

./doc.sh | ./pager.awk | ./tw && zathura output.pdf
