#!/bin/sh

XSOCK=/tmp/.X11-unix
XAUTH=/home/$USER/.Xauthority
SHARED_DIR=/home/$USER/shared_dir
HOST_DIR=/home/$USER/shared_dir

<<<<<<< HEAD
if [ "$1" = "kinetic" ] || [ "$1" = "indigo" ] || [ "$1" = "test" ]
then
    echo "Using $1"
else
    echo "Select distribution, kinetic|indigo|test"
=======
if [ "$1" = "kinetic" ] || [ "$1" = "indigo" ]
then
    echo "Use $1"
else
    echo "Select distribution, kinetic|indigo"
>>>>>>> b94c18bdb82cf29214a06941597a07b004c39dbb
    exit
fi

if [ "$2" = "" ]
then
    # Create Shared Folder
    mkdir -p $SHARED_DIR
else
    HOST_DIR=$2
fi
echo "Shared directory: ${HOST_DIR}"

nvidia-docker run \
    -it --rm \
    --volume=$XSOCK:$XSOCK:rw \
    --volume=$XAUTH:$XAUTH:rw \
    --volume=$HOST_DIR:$SHARED_DIR:rw \
    --env="XAUTHORITY=${XAUTH}" \
    --env="DISPLAY=${DISPLAY}" \
<<<<<<< HEAD
    --privileged -v /dev/bus/usb:/dev/bus/usb \
    --net=host \
autoware-$1


#     -u autoware \
=======
    -u autoware \
    --privileged -v /dev/bus/usb:/dev/bus/usb \
    --net=host \
    autoware-$1
>>>>>>> b94c18bdb82cf29214a06941597a07b004c39dbb
