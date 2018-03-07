#!/bin/sh

# Build Docker Image
<<<<<<< HEAD
if [ "$1" = "kinetic" ] || [ "$1" = "indigo" ] || [ "$1" = "test" ]
=======
if [ "$1" = "kinetic" ] || [ "$1" = "indigo" ]
>>>>>>> b94c18bdb82cf29214a06941597a07b004c39dbb
then
    echo "Use $1"
    nvidia-docker build -t autoware-$1 -f Dockerfile.$1 .
else
    echo "Select distribution, kinetic|indigo"
fi
